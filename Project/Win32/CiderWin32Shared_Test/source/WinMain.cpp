

#include "CiderShared.hpp"


#include <tchar.h>
#include <Windows.h>


int APIENTRY _tWinMain(
    _In_        HINSTANCE hInstance,
    _In_opt_    HINSTANCE hPrevInstance,
    _In_        LPWSTR    lpCmdLine,
    _In_        int       nCmdShow
)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // DLLをロード
    ::HMODULE dll = ::LoadLibraryA("CiderWin32Shared.dll");

    if (dll != NULL)
    {
        // DLL関数を取得
        auto hello = reinterpret_cast<Cider_HelloProc>(::GetProcAddress(dll, "Cider_Hello"));

        if (hello != NULL)
        {
            hello();
        }

        auto log = reinterpret_cast<Cider_LogFormatProc>(::GetProcAddress(dll, "Cider_LogFormat"));

        if (log != NULL)
        {
            log(CIDER_LOG_D, "%s", "Test");
        }

        // DLLを解放
        ::FreeLibrary(dll);
    }

    return EXIT_SUCCESS;
}

