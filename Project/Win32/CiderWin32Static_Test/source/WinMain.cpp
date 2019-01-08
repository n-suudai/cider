
#include "Cider.hpp"
#pragma comment(lib, "CiderWin32Static.lib")


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

    Cider::Hello();

    CIDER_ASSERT(false, "強制失敗！！");

    return EXIT_SUCCESS;
}

