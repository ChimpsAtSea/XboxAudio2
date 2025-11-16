#include "xma_voice.h"
#include <vector>

#define assert(expression) if(!(expression)) { __debugbreak(); throw 0; }

static void av_log_callback(void *ptr, int level, char const *fmt, va_list args)
{
    static char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    OutputDebugStringA(buffer);
}

CXAudio2SourceVoiceXMA2::~CXAudio2SourceVoiceXMA2()
{
    av_packet_free(&m_pPacket);
    av_frame_free(&m_pFrame);
    m_pSourceVoice->DestroyVoice();
}

CXAudio2SourceVoiceXMA2::CXAudio2SourceVoiceXMA2(IXAudio2SourceVoice *pSource, XMA2WAVEFORMATEX const& _xbox_wave_format, WAVEFORMATIEEEFLOATEX  const &_host_wave_format) :
    m_SamplesPosition(),
    m_pSamplesRing(),
    m_XMA2WaveFormat(_xbox_wave_format),
    m_WaveFormat(_host_wave_format),
    m_pFrame(),
    m_pPacket(),
    m_pXMA2Codec(),
    m_pXMA2CodecContext(),
    m_pSourceVoice(pSource),
    m_SamplesDecodeTemp()
{
    // reserve 1MiB
    m_SamplesDecodeTemp.reserve(1024 * 1024 / sizeof(float));

    m_pFrame = av_frame_alloc();
    m_pPacket = av_packet_alloc();
#if 0
    av_log_set_level(AV_LOG_DEBUG);
    av_log_set_callback(av_log_callback);
#endif
    m_pXMA2Codec = avcodec_find_decoder(AV_CODEC_ID_XMA2);
    m_pXMA2CodecContext = avcodec_alloc_context3(m_pXMA2Codec);
    av_channel_layout_default(&m_pXMA2CodecContext->ch_layout, m_XMA2WaveFormat.wfx.nChannels);

    m_pXMA2CodecContext->sample_rate = m_XMA2WaveFormat.wfx.nSamplesPerSec;
    m_pXMA2CodecContext->block_align = m_XMA2WaveFormat.wfx.nBlockAlign;
    m_pXMA2CodecContext->bits_per_raw_sample = m_XMA2WaveFormat.wfx.wBitsPerSample;
    m_pXMA2CodecContext->extradata_size = sizeof(m_XMA2WaveFormat) - sizeof(m_XMA2WaveFormat.wfx);
    m_pXMA2CodecContext->extradata = (uint8_t *)av_malloc(m_pXMA2CodecContext->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pXMA2CodecContext->extradata, &m_XMA2WaveFormat.wfx + 1, m_pXMA2CodecContext->extradata_size);

    if (avcodec_open2(m_pXMA2CodecContext, nullptr, nullptr) < 0)
    {
        av_free(m_pXMA2CodecContext->extradata);
        MessageBoxW(nullptr, L"Failed to open XMA2 codec", __FUNCTIONW__, MB_OK);
    }
}

HRESULT CXAudio2SourceVoiceXMA2::Create(IXAudio2SourceVoice **ppSourceVoice, IXAudio2 *pXAudio2, XMA2WAVEFORMATEX const *pSourceFormat, UINT32 Flags, float MaxFrequencyRatio, IXAudio2VoiceCallback *pCallback, XAUDIO2_VOICE_SENDS const *pSendList, XAUDIO2_EFFECT_CHAIN const *pEffectChain)
{
    assert(ppSourceVoice != nullptr);
    assert(pXAudio2 != nullptr);
    assert(pSourceFormat != nullptr);

    WAVEFORMATIEEEFLOATEX host_wave_format;
    host_wave_format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    host_wave_format.Format.nChannels = pSourceFormat->wfx.nChannels;
    host_wave_format.Format.nSamplesPerSec = pSourceFormat->wfx.nSamplesPerSec;
    host_wave_format.Format.wBitsPerSample = 32;
    host_wave_format.Format.nBlockAlign =   (host_wave_format.Format.nChannels * host_wave_format.Format.wBitsPerSample) / 8;
    host_wave_format.Format.nAvgBytesPerSec =  host_wave_format.Format.nSamplesPerSec * host_wave_format.Format.nBlockAlign;
    host_wave_format.Format.cbSize = sizeof(WAVEFORMATIEEEFLOATEX) - sizeof(WAVEFORMATEX);

    host_wave_format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    host_wave_format.Samples.wValidBitsPerSample = host_wave_format.Format.wBitsPerSample;
    static const DWORD kChannelMasks[] = {
        0,
        0,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
        0,
        0,
        0,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
        0,
    };
    host_wave_format.dwChannelMask = kChannelMasks[host_wave_format.Format.nChannels];

    IXAudio2SourceVoice *pSource;
    HRESULT hr;
    if (SUCCEEDED(hr = pXAudio2->CreateSourceVoice(&pSource, &host_wave_format.Format, Flags, MaxFrequencyRatio, pCallback, pSendList, pEffectChain)))
    {
        *ppSourceVoice = new CXAudio2SourceVoiceXMA2(pSource, *pSourceFormat, host_wave_format);
    }
    return hr;
}

struct DecodeThreadParams
{
    CXAudio2SourceVoiceXMA2 *pSource;
    XAUDIO2_BUFFER Buffer;

    DecodeThreadParams(CXAudio2SourceVoiceXMA2 *pSource, XAUDIO2_BUFFER const *pBuffer) :
        pSource(pSource),
        Buffer(*pBuffer)
    {
    }
};

HRESULT CXAudio2SourceVoiceXMA2::SubmitSourceBuffer(XAUDIO2_BUFFER const *pBuffer, XAUDIO2_BUFFER_WMA const *pBufferWMA)
{
    av_init_packet(m_pPacket);
    m_pPacket->data = (uint8_t *)pBuffer->pAudioData;
    m_pPacket->size = pBuffer->AudioBytes;
    avcodec_send_packet(m_pXMA2CodecContext, m_pPacket);

    m_SamplesDecodeTemp.clear();
    while (avcodec_receive_frame(m_pXMA2CodecContext, m_pFrame) >= 0)
    {
        for (int i = 0; i < m_pFrame->nb_samples; i++)
        {
            for (int j = 0; j < m_pFrame->ch_layout.nb_channels; j++)
            {
                m_SamplesDecodeTemp.push_back(((float *)m_pFrame->data[j])[i]);
            }
        }
    }

    if(!m_SamplesDecodeTemp.empty())
    {
        UINT32 numSamples = UINT32(m_SamplesDecodeTemp.size());
        UINT32 audioBytes = sizeof(float) * numSamples;

        if (audioBytes > RING_BUFFER_SIZE)
        {
            throw 0;
        }

        size_t bytes_remaining = RING_BUFFER_SIZE - m_SamplesPosition;
        if(bytes_remaining < audioBytes)
        {
            m_SamplesPosition = 0;
        }

        BYTE *pSamples = &m_pSamplesRing[m_SamplesPosition];
        memcpy(pSamples, m_SamplesDecodeTemp.data(), audioBytes);
        m_SamplesPosition += audioBytes;

        UINT32 maxPlayLength = UINT32(numSamples / m_pXMA2CodecContext->ch_layout.nb_channels);

        XAUDIO2_BUFFER Buffer = {};
        Buffer.Flags = pBuffer->Flags;
        Buffer.AudioBytes = audioBytes;
        Buffer.pAudioData = pSamples;
        Buffer.PlayBegin = pBuffer->PlayBegin;
        Buffer.PlayLength = __min(pBuffer->PlayLength, maxPlayLength);
        Buffer.LoopBegin = pBuffer->LoopBegin;
        Buffer.LoopLength = pBuffer->LoopLength;
        Buffer.LoopCount = pBuffer->LoopCount;
        Buffer.pContext = pBuffer->pContext;

        RETURN_IF_FAILED(m_pSourceVoice->SubmitSourceBuffer(&Buffer, nullptr));
    }

    RETURN_HR(S_OK);
}
