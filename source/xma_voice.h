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
    static constexpr size_t RING_BUFFER_SIZE = 16 * 1024 * 1024; // 16MiB

    size_t m_SamplesPosition = 0;
    BYTE m_pSamplesRing[RING_BUFFER_SIZE];
    std::vector<float> m_SamplesDecodeTemp;

    XMA2WAVEFORMATEX m_XMA2WaveFormat;
    WAVEFORMATIEEEFLOATEX  m_WaveFormat;

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
    void GetVoiceDetails(XAUDIO2_VOICE_DETAILS *pVoiceDetails) override
    {
        m_pSourceVoice->GetVoiceDetails(pVoiceDetails);
    }

    HRESULT SetOutputVoices(XAUDIO2_VOICE_SENDS const *pSendList) override
    {
        return m_pSourceVoice->SetOutputVoices(pSendList);
    }

    HRESULT SetEffectChain(XAUDIO2_EFFECT_CHAIN const *pEffectChain) override
    {
        return m_pSourceVoice->SetEffectChain(pEffectChain);
    }

    HRESULT EnableEffect(UINT32 EffectIndex, UINT32 OperationSet) override
    {
        return m_pSourceVoice->EnableEffect(EffectIndex, OperationSet);
    }

    HRESULT DisableEffect(UINT32 EffectIndex, UINT32 OperationSet) override
    {
        return m_pSourceVoice->DisableEffect(EffectIndex, OperationSet);
    }

    void GetEffectState(UINT32 EffectIndex, BOOL *pEnabled) override
    {
        m_pSourceVoice->GetEffectState(EffectIndex, pEnabled);
    }

    HRESULT SetEffectParameters(UINT32 EffectIndex, void const *pParameters, UINT32 ParametersByteSize, UINT32 OperationSet) override
    {
        return m_pSourceVoice->SetEffectParameters(EffectIndex, pParameters, ParametersByteSize, OperationSet);
    }

    HRESULT GetEffectParameters(UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize) override
    {
        return m_pSourceVoice->GetEffectParameters(EffectIndex, pParameters, ParametersByteSize);
    }

    HRESULT SetFilterParameters(XAUDIO2_FILTER_PARAMETERS const *pParameters, UINT32 OperationSet) override
    {
        return m_pSourceVoice->SetFilterParameters(pParameters, OperationSet);
    }

    void GetFilterParameters(XAUDIO2_FILTER_PARAMETERS *pParameters) override
    {
        return m_pSourceVoice->GetFilterParameters(pParameters);
    }

    HRESULT SetOutputFilterParameters(IXAudio2Voice *pDestinationVoice, XAUDIO2_FILTER_PARAMETERS const *pParameters, UINT32 OperationSet) override
    {
        return m_pSourceVoice->SetOutputFilterParameters(pDestinationVoice, pParameters, OperationSet);
    }

    void GetOutputFilterParameters(IXAudio2Voice *pDestinationVoice, XAUDIO2_FILTER_PARAMETERS *pParameters) override
    {
        m_pSourceVoice->GetOutputFilterParameters(pDestinationVoice, pParameters);
    }

    HRESULT SetVolume(float Volume, UINT32 OperationSet) override
    {
        return m_pSourceVoice->SetVolume(Volume, OperationSet);
    }

    void GetVolume(float *pVolume) override
    {
        m_pSourceVoice->GetVolume(pVolume);
    }

    HRESULT SetChannelVolumes(UINT32 Channels, float const *pVolumes, UINT32 OperationSet) override
    {
        return m_pSourceVoice->SetChannelVolumes(Channels, pVolumes, OperationSet);
    }

    void GetChannelVolumes(UINT32 Channels, float *pVolumes) override
    {
        m_pSourceVoice->GetChannelVolumes(Channels, pVolumes);
    }

    HRESULT SetOutputMatrix(IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels, UINT32 DestinationChannels, float const *pLevelMatrix, UINT32 OperationSet) override
    {
        return m_pSourceVoice->SetOutputMatrix(pDestinationVoice, SourceChannels, DestinationChannels, pLevelMatrix, OperationSet);
    }

    void GetOutputMatrix(IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels, UINT32 DestinationChannels, float *pLevelMatrix) override
    {
        m_pSourceVoice->GetOutputMatrix(pDestinationVoice, SourceChannels, DestinationChannels, pLevelMatrix);
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
        return m_pSourceVoice->Start(Flags, OperationSet);
    }

    HRESULT Stop(UINT32 Flags, UINT32 OperationSet) override
    {
        return m_pSourceVoice->Stop(Flags, OperationSet);
    }

    HRESULT SubmitSourceBuffer(XAUDIO2_BUFFER const *pBuffer, XAUDIO2_BUFFER_WMA const *pBufferWMA) override;

    HRESULT FlushSourceBuffers() override
    {
        return m_pSourceVoice->FlushSourceBuffers();
    }

    HRESULT Discontinuity() override
    {
        return m_pSourceVoice->Discontinuity();
    }

    HRESULT ExitLoop(UINT32 OperationSet) override
    {
        return m_pSourceVoice->ExitLoop(OperationSet);
    }

    void GetState(XAUDIO2_VOICE_STATE *pVoiceState, UINT32 Flags) override
    {
        m_pSourceVoice->GetState(pVoiceState, Flags);
    }

    HRESULT SetFrequencyRatio(float Ratio, UINT32 OperationSet) override
    {
        return m_pSourceVoice->SetFrequencyRatio(Ratio, OperationSet);
    }

    void GetFrequencyRatio(float *pRatio) override
    {
        m_pSourceVoice->GetFrequencyRatio(pRatio);
    }

    HRESULT SetSourceSampleRate(UINT32 NewSourceSampleRate) override
    {
        return m_pSourceVoice->SetSourceSampleRate(NewSourceSampleRate);
    }
};
