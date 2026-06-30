#pragma once
#include <xaudio2.h>
#include <xma2defs.h>
#include <wrl/client.h>

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <avcodec_raii.h>
#include <vector>

class CXAudio2SourceVoiceXMA2 final :
    public IXAudio2SourceVoice
{
    static constexpr size_t RING_BUFFER_SIZE = 16 * 1024 * 1024; // 16MiB

    size_t m_SamplesPosition = 0;
    BYTE m_pSamplesRing[RING_BUFFER_SIZE];
    std::vector<float> m_SamplesDecodeTemp;

    XMA2WAVEFORMATEX m_XMA2WaveFormat;

    AVFrame *m_pFrame = nullptr;
    AVPacket *m_pPacket = nullptr;
    AVCodec const *m_pXMA2Codec;
    AVCodecContext *m_pXMA2CodecContext;

    IXAudio2SourceVoice *m_pSourceVoice;

    ~CXAudio2SourceVoiceXMA2();
    CXAudio2SourceVoiceXMA2(IXAudio2SourceVoice *pSource, XMA2WAVEFORMATEX const &xbox_wave_format, WAVEFORMATIEEEFLOATEX  const &host_wave_format);
public:
    static HRESULT Create(IXAudio2SourceVoice **ppSourceVoice, IXAudio2 *pXAudio2, XMA2WAVEFORMATEX const *pSourceFormat, UINT32 Flags, float MaxFrequencyRatio, IXAudio2VoiceCallback *pCallback, XAUDIO2_VOICE_SENDS const *pSendList, XAUDIO2_EFFECT_CHAIN const *pEffectChain);

    //
    // IXAudio2Voice
    //
    STDMETHOD_(void, GetVoiceDetails) (THIS_ _Out_ XAUDIO2_VOICE_DETAILS *pVoiceDetails) override
    {
        m_pSourceVoice->GetVoiceDetails(pVoiceDetails);
    }

    STDMETHOD(SetOutputVoices) (THIS_ _In_opt_ const XAUDIO2_VOICE_SENDS *pSendList) override
    {
        return m_pSourceVoice->SetOutputVoices(pSendList);
    }

    STDMETHOD(SetEffectChain) (THIS_ _In_opt_ const XAUDIO2_EFFECT_CHAIN *pEffectChain) override
    {
        return m_pSourceVoice->SetEffectChain(pEffectChain);
    }

    STDMETHOD(EnableEffect) (THIS_ UINT32 EffectIndex,
        UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->EnableEffect(EffectIndex, OperationSet);
    }

    STDMETHOD(DisableEffect) (THIS_ UINT32 EffectIndex,
        UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->DisableEffect(EffectIndex, OperationSet);
    }

    STDMETHOD_(void, GetEffectState) (THIS_ UINT32 EffectIndex, _Out_ BOOL *pEnabled) override
    {
        m_pSourceVoice->GetEffectState(EffectIndex, pEnabled);
    }

    STDMETHOD(SetEffectParameters) (THIS_ UINT32 EffectIndex,
        _In_reads_bytes_(ParametersByteSize) const void *pParameters,
        UINT32 ParametersByteSize,
        UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->SetEffectParameters(EffectIndex, pParameters, ParametersByteSize, OperationSet);
    }

    STDMETHOD(GetEffectParameters) (THIS_ UINT32 EffectIndex,
        _Out_writes_bytes_(ParametersByteSize) void *pParameters,
        UINT32 ParametersByteSize) override
    {
        return m_pSourceVoice->GetEffectParameters(EffectIndex, pParameters, ParametersByteSize);
    }

    STDMETHOD(SetFilterParameters) (THIS_ _In_ const XAUDIO2_FILTER_PARAMETERS *pParameters,
        UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->SetFilterParameters(pParameters, OperationSet);
    }

    STDMETHOD_(void, GetFilterParameters) (THIS_ _Out_ XAUDIO2_FILTER_PARAMETERS *pParameters) override
    {
        return m_pSourceVoice->GetFilterParameters(pParameters);
    }

    STDMETHOD(SetOutputFilterParameters) (THIS_ _In_opt_ IXAudio2Voice *pDestinationVoice,
        _In_ const XAUDIO2_FILTER_PARAMETERS *pParameters,
        UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->SetOutputFilterParameters(pDestinationVoice, pParameters, OperationSet);
    }

    STDMETHOD_(void, GetOutputFilterParameters) (THIS_ _In_opt_ IXAudio2Voice *pDestinationVoice,
        _Out_ XAUDIO2_FILTER_PARAMETERS *pParameters) override
    {
        m_pSourceVoice->GetOutputFilterParameters(pDestinationVoice, pParameters);
    }

    STDMETHOD(SetVolume) (THIS_ float Volume,
        UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->SetVolume(Volume, OperationSet);
    }

    STDMETHOD_(void, GetVolume) (THIS_ _Out_ float *pVolume) override
    {
        m_pSourceVoice->GetVolume(pVolume);
    }

    STDMETHOD(SetChannelVolumes) (THIS_ UINT32 Channels, _In_reads_(Channels) const float *pVolumes,
        UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->SetChannelVolumes(Channels, pVolumes, OperationSet);
    }

    STDMETHOD_(void, GetChannelVolumes) (THIS_ UINT32 Channels, _Out_writes_(Channels) float *pVolumes) override
    {
        m_pSourceVoice->GetChannelVolumes(Channels, pVolumes);
    }

    STDMETHOD(SetOutputMatrix) (THIS_ _In_opt_ IXAudio2Voice *pDestinationVoice,
        UINT32 SourceChannels, UINT32 DestinationChannels,
        _In_reads_(SourceChannels *DestinationChannels) const float *pLevelMatrix,
        UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->SetOutputMatrix(pDestinationVoice, SourceChannels, DestinationChannels, pLevelMatrix, OperationSet);
    }

    STDMETHOD_(void, GetOutputMatrix) (THIS_ _In_opt_ IXAudio2Voice *pDestinationVoice,
        UINT32 SourceChannels, UINT32 DestinationChannels,
        _Out_writes_(SourceChannels *DestinationChannels) float *pLevelMatrix) override
    {
        m_pSourceVoice->GetOutputMatrix(pDestinationVoice, SourceChannels, DestinationChannels, pLevelMatrix);
    }

    STDMETHOD_(void, DestroyVoice) (THIS) override
    {
        delete this;
    }

    //
    // IXAudio2SourceVoice
    //

    STDMETHOD(Start) (THIS_ UINT32 Flags X2DEFAULT(0), UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->Start(Flags, OperationSet);
    }

    STDMETHOD(Stop) (THIS_ UINT32 Flags X2DEFAULT(0), UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->Stop(Flags, OperationSet);
    }

    STDMETHOD(SubmitSourceBuffer) (THIS_ _In_ const XAUDIO2_BUFFER *pBuffer, _In_opt_ const XAUDIO2_BUFFER_WMA *pBufferWMA X2DEFAULT(NULL)) override;

    STDMETHOD(FlushSourceBuffers) (THIS) override
    {
        return m_pSourceVoice->FlushSourceBuffers();
    }

    STDMETHOD(Discontinuity) (THIS) override
    {
        return m_pSourceVoice->Discontinuity();
    }

    STDMETHOD(ExitLoop) (THIS_ UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->ExitLoop(OperationSet);
    }

    STDMETHOD_(void, GetState) (THIS_ _Out_ XAUDIO2_VOICE_STATE *pVoiceState, UINT32 Flags X2DEFAULT(0)) override
    {
        m_pSourceVoice->GetState(pVoiceState, Flags);
    }

    STDMETHOD(SetFrequencyRatio) (THIS_ float Ratio,
        UINT32 OperationSet X2DEFAULT(XAUDIO2_COMMIT_NOW)) override
    {
        return m_pSourceVoice->SetFrequencyRatio(Ratio, OperationSet);
    }

    STDMETHOD_(void, GetFrequencyRatio) (THIS_ _Out_ float *pRatio) override
    {
        m_pSourceVoice->GetFrequencyRatio(pRatio);
    }

    STDMETHOD(SetSourceSampleRate) (THIS_ UINT32 NewSourceSampleRate) override
    {
        return m_pSourceVoice->SetSourceSampleRate(NewSourceSampleRate);
    }
};
