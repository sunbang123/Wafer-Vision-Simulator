// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // DLL이 프로세스에 로드될 때 실행되는 코드
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        // DLL이 프로세스에서 언로드될 때 실행되는 코드
        break;
    }
    return TRUE;
}

