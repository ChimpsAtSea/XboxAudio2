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
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, _COM_Outptr_ void **ppvInterface) override
    {
        RETURN_HR_IF_NULL(E_POINTER, ppvInterface);

        if (riid == __uuidof(IUnknown) ||
            riid == __uuidof(IXAudio2))
        {
            AddRef();
            *ppvInterface = this;
            RETURN_HR(S_OK);
        }

        *ppvInterface = nullptr;
        RETURN_HR(E_NOINTERFACE);
    }

    STDMETHOD_(ULONG, AddRef) (THIS) override
    {
        return InterlockedIncrement(&m_RefCount);
    }

    STDMETHOD_(ULONG, Release) (THIS) override
    {
        ULONG uCount = InterlockedDecrement(&m_RefCount);

        if (uCount == 0)
            delete this;

        return uCount;
    }

    //
    // IXAudio2
    //
    STDMETHOD(RegisterForCallbacks) (THIS_ _In_ IXAudio2EngineCallback *pCallback) override
    {
        return m_pXAudio2->RegisterForCallbacks(pCallback);
    }

    STDMETHOD_(void, UnregisterForCallbacks) (THIS_ _In_ IXAudio2EngineCallback *pCallback) override
    {
        m_pXAudio2->UnregisterForCallbacks(pCallback);
    }

    STDMETHOD(CreateSourceVoice) (THIS_ _Outptr_ IXAudio2SourceVoice **ppSourceVoice,
        _In_ const WAVEFORMATEX *pSourceFormat,
        UINT32 Flags X2DEFAULT(0),
        float MaxFrequencyRatio X2DEFAULT(XAUDIO2_DEFAULT_FREQ_RATIO),
        _In_opt_ IXAudio2VoiceCallback *pCallback X2DEFAULT(NULL),
        _In_opt_ const XAUDIO2_VOICE_SENDS *pSendList X2DEFAULT(NULL),
        _In_opt_ const XAUDIO2_EFFECT_CHAIN *pEffectChain X2DEFAULT(NULL)) override
    {
        if (pSourceFormat->wFormatTag == WAVE_FORMAT_XMA2)
            return CXAudio2SourceVoiceXMA2::Create(ppSourceVoice, this, (XMA2WAVEFORMATEX const *)pSourceFormat, Flags, MaxFrequencyRatio, pCallback, pSendList, pEffectChain);

        return m_pXAudio2->CreateSourceVoice(ppSourceVoice, pSourceFormat, Flags, MaxFrequencyRatio, pCallback, pSendList, pEffectChain);
    }

    STDMETHOD(CreateSubmixVoice) (THIS_ _Outptr_ IXAudio2SubmixVoice **ppSubmixVoice,
        UINT32 InputChannels, UINT32 InputSampleRate,
        UINT32 Flags X2DEFAULT(0), UINT32 ProcessingStage X2DEFAULT(0),
        _In_opt_ const XAUDIO2_VOICE_SENDS *pSendList X2DEFAULT(NULL),
        _In_opt_ const XAUDIO2_EFFECT_CHAIN *pEffectChain X2DEFAULT(NULL)) override
    {
        return m_pXAudio2->CreateSubmixVoice(ppSubmixVoice, InputChannels, InputSampleRate, Flags, ProcessingStage, pSendList, pEffectChain);
    }

    STDMETHOD(CreateMasteringVoice) (THIS_ _Outptr_ IXAudio2MasteringVoice **ppMasteringVoice,
        UINT32 InputChannels X2DEFAULT(XAUDIO2_DEFAULT_CHANNELS),
        UINT32 InputSampleRate X2DEFAULT(XAUDIO2_DEFAULT_SAMPLERATE),
        UINT32 Flags X2DEFAULT(0), _In_opt_z_ LPCWSTR szDeviceId X2DEFAULT(NULL),
        _In_opt_ const XAUDIO2_EFFECT_CHAIN *pEffectChain X2DEFAULT(NULL),
        _In_ AUDIO_STREAM_CATEGORY StreamCategory X2DEFAULT(AudioCategory_GameEffects)) override
    {
        return m_pXAudio2->CreateMasteringVoice(ppMasteringVoice, InputChannels, InputSampleRate, Flags, szDeviceId, pEffectChain, StreamCategory);
    }

    STDMETHOD(StartEngine) (THIS) override
    {
        return m_pXAudio2->StartEngine();
    }

    STDMETHOD_(void, StopEngine) (THIS) override
    {
        m_pXAudio2->StopEngine();
    }

    STDMETHOD(CommitChanges) (THIS_ UINT32 OperationSet) override
    {
        return m_pXAudio2->CommitChanges(OperationSet);
    }

    STDMETHOD_(void, GetPerformanceData) (THIS_ _Out_ XAUDIO2_PERFORMANCE_DATA *pPerfData) override
    {
        m_pXAudio2->GetPerformanceData(pPerfData);
    }
    STDMETHOD_(void, SetDebugConfiguration) (THIS_ _In_opt_ const XAUDIO2_DEBUG_CONFIGURATION *pDebugConfiguration,
        _Reserved_ void *pReserved X2DEFAULT(NULL)) override
    {
        m_pXAudio2->SetDebugConfiguration(pDebugConfiguration, pReserved);
    }
};

extern "C" HRESULT WINAPI CreateXAudio2Object(
    IXAudio2 **ppXAudio2,
    UINT32 Flags,
    XAUDIO2_PROCESSOR XAudio2Processor,
    void *pSharedShapeContexts);
