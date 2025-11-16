#pragma once
#include <xaudio2.h>
#include <xma2defs.h>
#include <wrl/client.h>
#include <wil/result_macros.h>
#include <wil/win32_helpers.h>

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <avcodec_raii.h>
#include <vector>

class CXAudio2SourceVoiceXMA2 final :
    public IXAudio2SourceVoice
{
    XMA2WAVEFORMATEX xbox_wave_format;
    WAVEFORMATIEEEFLOATEX  host_wave_format;

    AVFrame *av_frame_ = nullptr;
    AVPacket *packet = nullptr;

    //float *samples_buffer = nullptr;
    //size_t samples_buffer_size = 0;

    DWORD currentBufferIndex = 0;
    //static constexpr size_t AUDIO_BUFFER_SIZE = 0x480000;
    //static constexpr size_t MAX_AUDIO_BUFFER_COUNT = 8;
    static constexpr size_t BIG_BUFFER_SIZE = 0x480000 * 8;
    IXAudio2SourceVoice *m_pSource;
    AVCodec const *xma2_codec;
    AVCodecContext *av_context_;
    //wil::unique_handle m_hDecodeThread;
    BOOL m_bDecodeStopRequest = FALSE;

    //BYTE m_pAudioBuffers[MAX_AUDIO_BUFFER_COUNT][AUDIO_BUFFER_SIZE];
    size_t BufferPosition = 0;
    BYTE m_pAudioBuffer[BIG_BUFFER_SIZE];

    ~CXAudio2SourceVoiceXMA2();
    CXAudio2SourceVoiceXMA2(IXAudio2SourceVoice *pSource, XMA2WAVEFORMATEX const &xbox_wave_format, WAVEFORMATIEEEFLOATEX  const &host_wave_format);
public:
    static HRESULT Create(IXAudio2SourceVoice **ppSourceVoice, IXAudio2 *pXAudio2, XMA2WAVEFORMATEX const *pSourceFormat, UINT32 Flags, float MaxFrequencyRatio, IXAudio2VoiceCallback *pCallback, XAUDIO2_VOICE_SENDS const *pSendList, XAUDIO2_EFFECT_CHAIN const *pEffectChain);

    //
    // IXAudio2Voice
    //
    void GetVoiceDetails(XAUDIO2_VOICE_DETAILS *pVoiceDetails) override
    {
        m_pSource->GetVoiceDetails(pVoiceDetails);
    }

    HRESULT SetOutputVoices(XAUDIO2_VOICE_SENDS const *pSendList) override
    {
        return m_pSource->SetOutputVoices(pSendList);
    }

    HRESULT SetEffectChain(XAUDIO2_EFFECT_CHAIN const *pEffectChain) override
    {
        return m_pSource->SetEffectChain(pEffectChain);
    }

    HRESULT EnableEffect(UINT32 EffectIndex, UINT32 OperationSet) override
    {
        return m_pSource->EnableEffect(EffectIndex, OperationSet);
    }

    HRESULT DisableEffect(UINT32 EffectIndex, UINT32 OperationSet) override
    {
        return m_pSource->DisableEffect(EffectIndex, OperationSet);
    }

    void GetEffectState(UINT32 EffectIndex, BOOL *pEnabled) override
    {
        m_pSource->GetEffectState(EffectIndex, pEnabled);
    }

    HRESULT SetEffectParameters(UINT32 EffectIndex, void const *pParameters, UINT32 ParametersByteSize, UINT32 OperationSet) override
    {
        return m_pSource->SetEffectParameters(EffectIndex, pParameters, ParametersByteSize, OperationSet);
    }

    HRESULT GetEffectParameters(UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize) override
    {
        return m_pSource->GetEffectParameters(EffectIndex, pParameters, ParametersByteSize);
    }

    HRESULT SetFilterParameters(XAUDIO2_FILTER_PARAMETERS const *pParameters, UINT32 OperationSet) override
    {
        return m_pSource->SetFilterParameters(pParameters, OperationSet);
    }

    void GetFilterParameters(XAUDIO2_FILTER_PARAMETERS *pParameters) override
    {
        return m_pSource->GetFilterParameters(pParameters);
    }

    HRESULT SetOutputFilterParameters(IXAudio2Voice *pDestinationVoice, XAUDIO2_FILTER_PARAMETERS const *pParameters, UINT32 OperationSet) override
    {
        return m_pSource->SetOutputFilterParameters(pDestinationVoice, pParameters, OperationSet);
    }

    void GetOutputFilterParameters(IXAudio2Voice *pDestinationVoice, XAUDIO2_FILTER_PARAMETERS *pParameters) override
    {
        m_pSource->GetOutputFilterParameters(pDestinationVoice, pParameters);
    }

    HRESULT SetVolume(float Volume, UINT32 OperationSet) override
    {
        return m_pSource->SetVolume(Volume, OperationSet);
    }

    void GetVolume(float *pVolume) override
    {
        m_pSource->GetVolume(pVolume);
    }

    HRESULT SetChannelVolumes(UINT32 Channels, float const *pVolumes, UINT32 OperationSet) override
    {
        return m_pSource->SetChannelVolumes(Channels, pVolumes, OperationSet);
    }

    void GetChannelVolumes(UINT32 Channels, float *pVolumes) override
    {
        m_pSource->GetChannelVolumes(Channels, pVolumes);
    }

    HRESULT SetOutputMatrix(IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels, UINT32 DestinationChannels, float const *pLevelMatrix, UINT32 OperationSet) override
    {
        return m_pSource->SetOutputMatrix(pDestinationVoice, SourceChannels, DestinationChannels, pLevelMatrix, OperationSet);
    }

    void GetOutputMatrix(IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels, UINT32 DestinationChannels, float *pLevelMatrix) override
    {
        m_pSource->GetOutputMatrix(pDestinationVoice, SourceChannels, DestinationChannels, pLevelMatrix);
    }

    void DestroyVoice() override
    {
        delete this;
    }

    //
    // IXAudio2SourceVoice
    //
    HRESULT Start(UINT32 Flags, UINT32 OperationSet) override
    {
        return m_pSource->Start(Flags, OperationSet);
    }

    HRESULT Stop(UINT32 Flags, UINT32 OperationSet) override
    {
        return m_pSource->Stop(Flags, OperationSet);
    }

    HRESULT SubmitSourceBuffer(XAUDIO2_BUFFER const *pBuffer, XAUDIO2_BUFFER_WMA const *pBufferWMA) override;

    HRESULT FlushSourceBuffers() override
    {
        return m_pSource->FlushSourceBuffers();
    }

    HRESULT Discontinuity() override
    {
        return m_pSource->Discontinuity();
    }

    HRESULT ExitLoop(UINT32 OperationSet) override
    {
        return m_pSource->ExitLoop(OperationSet);
    }

    void GetState(XAUDIO2_VOICE_STATE *pVoiceState, UINT32 Flags) override
    {
        m_pSource->GetState(pVoiceState, Flags);
    }

    HRESULT SetFrequencyRatio(float Ratio, UINT32 OperationSet) override
    {
        return m_pSource->SetFrequencyRatio(Ratio, OperationSet);
    }

    void GetFrequencyRatio(float *pRatio) override
    {
        m_pSource->GetFrequencyRatio(pRatio);
    }

    HRESULT SetSourceSampleRate(UINT32 NewSourceSampleRate) override
    {
        return m_pSource->SetSourceSampleRate(NewSourceSampleRate);
    }
};
