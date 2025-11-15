#include <Windows.h>
#include "xaudio2_impl.h"
#define ORDINAL_PROC(n) extern "C" void *OrdinalProc##n; void *OrdinalProc##n; __pragma(comment(linker, "/export:OrdinalThunk" #n ",@" #n ",NONAME"))

//
// Variant 1 (older ERA titles)
// 1: XAudio2Create
// 2: CreateAudioReverb
// 3: CreateAudioVolumeMeter
// 4: CreateFX
// 5: X3DAudioCalculate
// 6: X3DAudioInitialize
//
// Variant 2 (newer ERA titles)
// 1: CreateAudioReverb
// 2: CreateAudioVolumeMeter
// 3: CreateFX
// 4: CreateXAudio2Object
// 5: X3DAudioCalculate
// 6: X3DAudioInitialize
//
ORDINAL_PROC(1);
ORDINAL_PROC(2);
ORDINAL_PROC(3);
ORDINAL_PROC(4);

#pragma comment(linker, "/export:CreateXAudio2Object")
#pragma comment(linker, "/export:XAudio2Create=CreateXAudio2Object")

// We need to export everything because GetModuleHandle("XAudio2_9") may resolve to this DLL.
#define XAUDIO2_FORWARD(Name) __pragma(comment(linker, "/export:" #Name "=C:\\WINDOWS\\System32\\XAudio2_9." #Name))
#define XAUDIO2_FORWARD_ORDINAL(Name, Ordinal) __pragma(comment(linker, "/export:" #Name "=C:\\WINDOWS\\System32\\XAudio2_9." #Name ",@" #Ordinal))
XAUDIO2_FORWARD(CreateAudioReverb)
XAUDIO2_FORWARD(CreateAudioVolumeMeter)
XAUDIO2_FORWARD(CreateFX)
XAUDIO2_FORWARD_ORDINAL(X3DAudioCalculate, 5)
XAUDIO2_FORWARD_ORDINAL(X3DAudioInitialize, 6)
XAUDIO2_FORWARD_ORDINAL(CreateAudioReverbV2_8, 7)
XAUDIO2_FORWARD_ORDINAL(XAudio2CreateV2_9, 8)
XAUDIO2_FORWARD_ORDINAL(XAudio2CreateWithVersionInfo, 9)
XAUDIO2_FORWARD_ORDINAL(XAudio2CreateWithSharedContexts, 10)

BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD fdwReason,
    _In_ LPVOID lpvReserved)
{
    OSVERSIONINFOW OsVersion = { sizeof(OsVersion) };

    if (fdwReason != DLL_PROCESS_ATTACH)
        return TRUE;

    // This should link to kernelx.dll!GetVersionExW.
#pragma warning(suppress : 28159)
    if (!GetVersionExW(&OsVersion))
        return FALSE;

    // 6.2.10698.0 is the oldest known OS version with CreateXAudio2Object.
    if (OsVersion.dwMajorVersion > 6 ||
        (OsVersion.dwMajorVersion == 6 && OsVersion.dwMinorVersion > 2) ||
        (OsVersion.dwMajorVersion == 6 && OsVersion.dwMinorVersion == 2 && OsVersion.dwBuildNumber >= 10698))
    {
        OrdinalProc1 = GetProcAddress(hinstDLL, "CreateAudioReverb");
        OrdinalProc2 = GetProcAddress(hinstDLL, "CreateAudioVolumeMeter");
        OrdinalProc3 = GetProcAddress(hinstDLL, "CreateFX");
        OrdinalProc4 = GetProcAddress(hinstDLL, "CreateXAudio2Object");
    }
    else
    {
        OrdinalProc1 = GetProcAddress(hinstDLL, "XAudio2Create");
        OrdinalProc2 = GetProcAddress(hinstDLL, "CreateAudioReverb");
        OrdinalProc3 = GetProcAddress(hinstDLL, "CreateAudioVolumeMeter");
        OrdinalProc4 = GetProcAddress(hinstDLL, "CreateFX");
    }

    return TRUE;
}

HRESULT CreateXAudio2Object(
    IXAudio2 **ppXAudio2,
    UINT32 Flags,
    XAUDIO2_PROCESSOR XAudio2Processor,
    void *pSharedShapeContexts)
{
    HRESULT hr;
    Microsoft::WRL::ComPtr<IXAudio2> pXAudio2;
    RETURN_IF_FAILED(hr = XAudio2Create(&pXAudio2, Flags, XAudio2Processor));
    *ppXAudio2 = new CXAudio2(pXAudio2.Get());
    return hr;
}
