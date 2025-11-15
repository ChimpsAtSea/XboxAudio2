#include "xma_voice.h"
#include <vector>

static void av_log_callback(void *ptr, int level, char const *fmt, va_list args)
{
    static char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    OutputDebugStringA(buffer);
}

CXAudio2SourceVoiceXMA2::~CXAudio2SourceVoiceXMA2()
{
    m_bDecodeStopRequest = TRUE;
    WaitForSingleObject(m_hDecodeThread.get(), INFINITE);
    m_pSource->DestroyVoice();
}

CXAudio2SourceVoiceXMA2::CXAudio2SourceVoiceXMA2(IXAudio2SourceVoice *pSource, XMA2WAVEFORMATEX const *pSourceFormat) :
    m_pSource(pSource),
    m_hDecodeThread(nullptr),
    m_pAudioBuffers{}
{
#if 0
    av_log_set_level(AV_LOG_DEBUG);
    av_log_set_callback(av_log_callback);
#endif
    m_pCodec = avcodec_find_decoder(AV_CODEC_ID_XMA2);
    m_pContext = unique_avcodec_context(avcodec_alloc_context3(m_pCodec));
    av_channel_layout_default(&m_pContext->ch_layout, pSourceFormat->wfx.nChannels);
    m_pContext->sample_rate = pSourceFormat->wfx.nSamplesPerSec;
    m_pContext->extradata_size = sizeof(*pSourceFormat) - sizeof(pSourceFormat->wfx);
    m_pContext->extradata = (uint8_t *)av_malloc(m_pContext->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pContext->extradata, &pSourceFormat->wfx + 1, m_pContext->extradata_size);

    if (avcodec_open2(m_pContext.get(), nullptr, nullptr) < 0)
    {
        av_free(m_pContext->extradata);
        MessageBoxW(nullptr, L"Failed to open XMA2 codec", __FUNCTIONW__, MB_OK);
    }
}

HRESULT CXAudio2SourceVoiceXMA2::Create(IXAudio2SourceVoice **ppSourceVoice, IXAudio2 *pXAudio2, XMA2WAVEFORMATEX const *pSourceFormat, UINT32 Flags, float MaxFrequencyRatio, IXAudio2VoiceCallback *pCallback, XAUDIO2_VOICE_SENDS const *pSendList, XAUDIO2_EFFECT_CHAIN const *pEffectChain)
{
    RETURN_HR_IF_NULL(E_POINTER, ppSourceVoice);
    RETURN_HR_IF_NULL(E_INVALIDARG, pXAudio2);
    RETURN_HR_IF_NULL(E_INVALIDARG, pSourceFormat);

    HRESULT hr;
    WORD nChannels = pSourceFormat->wfx.nChannels;

    // TODO
    WAVEFORMATEX DestFormat
    {
        WAVE_FORMAT_IEEE_FLOAT,
        nChannels,
        pSourceFormat->wfx.nSamplesPerSec,
        pSourceFormat->wfx.nSamplesPerSec * (nChannels * 32 / 8),
        nChannels * 32 / 8,
        32,
        0
    };

    IXAudio2SourceVoice *pSource;
    RETURN_IF_FAILED(hr = pXAudio2->CreateSourceVoice(&pSource, &DestFormat, Flags, MaxFrequencyRatio, pCallback, pSendList, pEffectChain));
    RETURN_IF_NULL_ALLOC(*ppSourceVoice = new(std::nothrow) CXAudio2SourceVoiceXMA2(pSource, pSourceFormat));
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

DWORD CXAudio2SourceVoiceXMA2::DecodeThreadProc(LPVOID lpParam)
{
    auto params = *(DecodeThreadParams *)lpParam;
    delete lpParam;

    auto frame = unique_avframe(av_frame_alloc());
    RETURN_IF_NULL_ALLOC(frame);

    DWORD currentBufferIndex = 0;
    std::vector<float> samples{};

    while (!params.pSource->m_bDecodeStopRequest && avcodec_receive_frame(params.pSource->m_pContext.get(), frame.get()) >= 0)
    {
        samples.clear();

        for (int i = 0; i < frame->nb_samples; i++)
        {
            for (int j = 0; j < frame->ch_layout.nb_channels; j++)
            {
                samples.push_back(((float *)frame->data[j])[i]);
            }
        }

        for (XAUDIO2_VOICE_STATE state; params.pSource->m_pSource->GetState(&state), (!params.pSource->m_bDecodeStopRequest && state.BuffersQueued >= MAX_AUDIO_BUFFER_COUNT);)
        {
            // TODO: Surely there is a better way of doing this that I am not thinking of right now
            Sleep(1);
        }

        // Copy the sample buffer
        INT64 AudioBytes = samples.size() * sizeof(float);
        memcpy(params.pSource->m_pAudioBuffers[currentBufferIndex], samples.data(), AudioBytes);

        XAUDIO2_BUFFER Buffer
        {
            // Only pass Flags for the last chunk
            AudioBytes <= XAUDIO2_MAX_BUFFER_BYTES ? params.Buffer.Flags : 0,

            // Cannot pass more than XAUDIO2_MAX_BUFFER_BYTES
            min(AudioBytes, XAUDIO2_MAX_BUFFER_BYTES),
            params.pSource->m_pAudioBuffers[currentBufferIndex],

            // TODO: Handle these
            0, // pBuffer->PlayBegin,
            min(AudioBytes, XAUDIO2_MAX_BUFFER_BYTES) / sizeof(float) / params.pSource->m_pContext->ch_layout.nb_channels, // pBuffer->PlayLength,
            0, // pBuffer->LoopBegin,
            0, // pBuffer->LoopLength,
            0, // pBuffer->LoopCount,
            params.Buffer.pContext,
        };

        currentBufferIndex++;
        currentBufferIndex %= MAX_AUDIO_BUFFER_COUNT;
        RETURN_IF_FAILED(params.pSource->m_pSource->SubmitSourceBuffer(&Buffer, nullptr));
    }

    RETURN_HR(S_OK);
}

HRESULT CXAudio2SourceVoiceXMA2::SubmitSourceBuffer(XAUDIO2_BUFFER const *pBuffer, XAUDIO2_BUFFER_WMA const *pBufferWMA)
{
    auto packet = unique_avpacket(av_packet_alloc());
    RETURN_IF_NULL_ALLOC(packet);

    av_init_packet(packet.get());
    packet->data = (uint8_t *)pBuffer->pAudioData;
    packet->size = pBuffer->AudioBytes;
    avcodec_send_packet(m_pContext.get(), packet.get());

    DecodeThreadParams *params;
    RETURN_IF_NULL_ALLOC(params = new(std::nothrow) DecodeThreadParams(this, pBuffer));
    m_hDecodeThread = wil::unique_handle(CreateThread(nullptr, 0, &DecodeThreadProc, params, 0, nullptr));
    RETURN_HR(S_OK);
}
