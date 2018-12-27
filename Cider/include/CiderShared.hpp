
#pragma once


#ifndef CIDER_APIENTRY
#   define CIDER_APIENTRY __stdcall
#endif


void CIDER_APIENTRY Cider_Hello();
typedef void (CIDER_APIENTRY *Cider_HelloProc)();

