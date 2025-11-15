#pragma once
#include <xaudio2.h>
#include <xma2defs.h>
#include <wrl/client.h>
#include <wil/result_macros.h>
#include "xma_voice.h"

class CXAudio2 final : public IXAudio2
{
    LONG m_RefCount;
    Microsoft::WRL::ComPtr<IXAudio2> m_pXAudio2;
public:
    CXAudio2(IXAudio2 *pXAudio2) :
        m_RefCount(1),
        m_pXAudio2(pXAudio2)
    {
    }

    //
    // IUnknown
    //
    HRESULT QueryInterface(REFIID riid, void **ppvObject) override
    {
        RETURN_HR_IF_NULL(E_POINTER, ppvObject);

        if (riid == __uuidof(IUnknown) ||
            riid == __uuidof(IXAudio2))
        {
            AddRef();
            *ppvObject = this;
            RETURN_HR(S_OK);
        }

        *ppvObject = nullptr;
        RETURN_HR(E_NOINTERFACE);
    }

    ULONG AddRef() override
    {
        return InterlockedIncrement(&m_RefCount);
    }

    ULONG Release() override
    {
        ULONG uCount = InterlockedDecrement(&m_RefCount);

        if (uCount == 0)
            delete this;

        return uCount;
    }

    //
    // IXAudio2
    //
    HRESULT RegisterForCallbacks(IXAudio2EngineCallback *pCallback) override
    {
        return m_pXAudio2->RegisterForCallbacks(pCallback);
    }

    void UnregisterForCallbacks(IXAudio2EngineCallback *pCallback) override
    {
        m_pXAudio2->UnregisterForCallbacks(pCallback);
    }

    HRESULT CreateSourceVoice(IXAudio2SourceVoice **ppSourceVoice, WAVEFORMATEX const *pSourceFormat, UINT32 Flags, float MaxFrequencyRatio, IXAudio2VoiceCallback *pCallback, XAUDIO2_VOICE_SENDS const *pSendList, XAUDIO2_EFFECT_CHAIN const *pEffectChain) override
    {
        if (pSourceFormat->wFormatTag == WAVE_FORMAT_XMA2)
            return CXAudio2SourceVoiceXMA2::Create(ppSourceVoice, this, (XMA2WAVEFORMATEX const *)pSourceFormat, Flags, MaxFrequencyRatio, pCallback, pSendList, pEffectChain);

        return m_pXAudio2->CreateSourceVoice(ppSourceVoice, pSourceFormat, Flags, MaxFrequencyRatio, pCallback, pSendList, pEffectChain);
    }

    HRESULT CreateSubmixVoice(IXAudio2SubmixVoice **ppSubmixVoice, UINT32 InputChannels, UINT32 InputSampleRate, UINT32 Flags, UINT32 ProcessingStage, XAUDIO2_VOICE_SENDS const *pSendList, XAUDIO2_EFFECT_CHAIN const *pEffectChain) override
    {
        return m_pXAudio2->CreateSubmixVoice(ppSubmixVoice, InputChannels, InputSampleRate, Flags, ProcessingStage, pSendList, pEffectChain);
    }

    HRESULT CreateMasteringVoice(IXAudio2MasteringVoice **ppMasteringVoice, UINT32 InputChannels, UINT32 InputSampleRate, UINT32 Flags, LPCWSTR szDeviceId, XAUDIO2_EFFECT_CHAIN const *pEffectChain, AUDIO_STREAM_CATEGORY StreamCategory) override
    {
        return m_pXAudio2->CreateMasteringVoice(ppMasteringVoice, InputChannels, InputSampleRate, Flags, szDeviceId, pEffectChain, StreamCategory);
    }

    HRESULT StartEngine() override
    {
        return m_pXAudio2->StartEngine();
    }

    void StopEngine() override
    {
        m_pXAudio2->StopEngine();
    }

    HRESULT CommitChanges(UINT32 OperationSet) override
    {
        return m_pXAudio2->CommitChanges(OperationSet);
    }

    void GetPerformanceData(XAUDIO2_PERFORMANCE_DATA *pPerfData) override
    {
        m_pXAudio2->GetPerformanceData(pPerfData);
    }

    void SetDebugConfiguration(XAUDIO2_DEBUG_CONFIGURATION const *pDebugConfiguration, void *pReserved)
    {
        m_pXAudio2->SetDebugConfiguration(pDebugConfiguration, pReserved);
    }
};

extern "C" HRESULT WINAPI CreateXAudio2Object(
    IXAudio2 **ppXAudio2,
    UINT32 Flags,
    XAUDIO2_PROCESSOR XAudio2Processor,
    void *pSharedShapeContexts);
