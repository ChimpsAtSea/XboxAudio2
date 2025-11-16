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
    av_packet_free(&packet);
    av_frame_free(&av_frame_);
    m_bDecodeStopRequest = TRUE;
    //WaitForSingleObject(m_hDecodeThread.get(), INFINITE);
    m_pSource->DestroyVoice();
}

CXAudio2SourceVoiceXMA2::CXAudio2SourceVoiceXMA2(IXAudio2SourceVoice *pSource, XMA2WAVEFORMATEX const& _xbox_wave_format, WAVEFORMATIEEEFLOATEX  const &_host_wave_format) :
    xbox_wave_format(_xbox_wave_format),
    host_wave_format(_host_wave_format),
    m_pSource(pSource),
    //m_hDecodeThread(nullptr),
    m_pAudioBuffer{}
{
    av_frame_ = av_frame_alloc();
    packet = av_packet_alloc();
#if 0
    av_log_set_level(AV_LOG_DEBUG);
    av_log_set_callback(av_log_callback);
#endif
    xma2_codec = avcodec_find_decoder(AV_CODEC_ID_XMA2);
    av_context_ = avcodec_alloc_context3(xma2_codec);
    av_channel_layout_default(&av_context_->ch_layout, xbox_wave_format.wfx.nChannels);
    av_context_->sample_rate = xbox_wave_format.wfx.nSamplesPerSec;
    av_context_->extradata_size = sizeof(xbox_wave_format) - sizeof(xbox_wave_format.wfx);
    av_context_->extradata = (uint8_t *)av_malloc(av_context_->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
    memcpy(av_context_->extradata, &xbox_wave_format.wfx + 1, av_context_->extradata_size);

    if (avcodec_open2(av_context_, nullptr, nullptr) < 0)
    {
        av_free(av_context_->extradata);
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
    host_wave_format.Format.nSamplesPerSec = 48000;
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
    av_init_packet(packet);
    packet->data = (uint8_t *)pBuffer->pAudioData;
    packet->size = pBuffer->AudioBytes;
    avcodec_send_packet(av_context_, packet);


    av_frame_ = av_frame_alloc();

    int index = -1;

    std::vector<float> samples{};
    //UINT32 buffer_size = 0;
    int last = 0;
    while ((last = avcodec_receive_frame(av_context_, av_frame_)) >= 0)
    {
        index++;

        for (int i = 0; i < av_frame_->nb_samples; i++)
        {
            for (int j = 0; j < av_frame_->ch_layout.nb_channels; j++)
            {
                samples.push_back(((float *)av_frame_->data[j])[i]);
            }
        }

        //UINT32 decode_size = UINT32(av_frame_->nb_samples) * UINT32(av_frame_->ch_layout.nb_channels);
        //UINT32 decode_start = buffer_size;
        //buffer_size += decode_size;
        //if (samples_buffer_size < buffer_size)
        //{
        //    delete[] samples_buffer;
        //    samples_buffer = new float[buffer_size];
        //    samples_buffer_size = buffer_size;
        //}
        //float *buffer = samples_buffer + decode_start;
        //
        //int nb_channels = av_frame_->ch_layout.nb_channels;
        //int nb_samples = av_frame_->nb_samples;
        //for (int j = 0; j < nb_channels; j++)
        //{
        //    float *channel = (float *)av_frame_->data[j];
        //
        //    for (int i = 0; i < nb_samples; i++)
        //    {
        //        buffer[i * nb_channels + j] = channel[i];
        //    }
        //}
    }

    if(!samples.empty())
    //if (buffer_size > 0)
    {
        //XAUDIO2_VOICE_STATE state;
        //m_pSource->GetState(&state);
        //if (state.BuffersQueued >= MAX_AUDIO_BUFFER_COUNT)
        //{
        //    // TODO: Surely there is a better way of doing this that I am not thinking of right now
        //    Sleep(1);
        //}

        // Copy the sample buffer
        //UINT32 AudioBytes = buffer_size * sizeof(float);
        UINT64 AudioBytes = samples.size() * sizeof(float);
        if (AudioBytes > BIG_BUFFER_SIZE)
        {
            throw 0;
        }

        size_t bytes_remaining = BIG_BUFFER_SIZE - BufferPosition;
        if(bytes_remaining < AudioBytes)
        {
            BufferPosition = 0;
        }

        void *buf = &m_pAudioBuffer[BufferPosition];
        memcpy(buf, samples.data(), AudioBytes);
        BufferPosition += AudioBytes;

        //memcpy(m_pAudioBuffers[currentBufferIndex], samples_buffer, AudioBytes);
        //memcpy(m_pAudioBuffers[currentBufferIndex], samples.data(), AudioBytes);

        XAUDIO2_BUFFER Buffer
        {
            // Only pass Flags for the last chunk
            AudioBytes <= XAUDIO2_MAX_BUFFER_BYTES ? pBuffer->Flags : 0,
            //pBuffer->Flags,
        
            // Cannot pass more than XAUDIO2_MAX_BUFFER_BYTES
            UINT(min(AudioBytes, XAUDIO2_MAX_BUFFER_BYTES)),
            (BYTE*)buf,//m_pAudioBuffers[currentBufferIndex],
        
            // TODO: Handle these
            pBuffer->PlayBegin, // pBuffer->PlayBegin,
            UINT(AudioBytes / sizeof(float) / av_context_->ch_layout.nb_channels),
            /*UINT32(min(AudioBytes, XAUDIO2_MAX_BUFFER_BYTES) / sizeof(float) / av_context_->ch_layout.nb_channels)*///, // pBuffer->PlayLength,
            pBuffer->LoopBegin, // pBuffer->LoopBegin,
            pBuffer->LoopLength, // pBuffer->LoopLength,
            pBuffer->LoopCount, // pBuffer->LoopCount,
            pBuffer->pContext,
        };

        //currentBufferIndex++;
        //currentBufferIndex %= MAX_AUDIO_BUFFER_COUNT;


        RETURN_IF_FAILED(m_pSource->SubmitSourceBuffer(&Buffer, nullptr));
    }

    av_frame_free(&av_frame_);

    RETURN_HR(S_OK);
}
