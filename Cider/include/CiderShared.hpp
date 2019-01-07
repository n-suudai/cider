
#pragma once


#include "Api.hpp"


void CIDER_APIENTRY Cider_Hello();
typedef void (CIDER_APIENTRY *Cider_HelloProc)();



#define CIDER_LOG_V 0
#define CIDER_LOG_D 1
#define CIDER_LOG_I 2
#define CIDER_LOG_W 3
#define CIDER_LOG_E 4
#define CIDER_LOG_A 5

void CIDER_APIENTRY Cider_LogFormat(int level, const char* format, ...);
typedef void (CIDER_APIENTRY *Cider_LogFormatProc)(int level, const char* format, ...);

void CIDER_APIENTRY Cider_LogMessage(int level, const char* message);
typedef void (CIDER_APIENTRY *Cider_LogMessageProc)(int level, const char* message);

