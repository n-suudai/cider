
#pragma once

#include "System/Types.hpp"


namespace Cider {
namespace System {


class StackTrace
{
public:
    struct TraceInfo
    {
        Char            function[256];
        Char            file[256];
        Char            moduleName[128];
        Int32    line;
        Void*           lineAddress;
        Void*           address;

        TraceInfo();

        Void Clear();

        Void Print();
    };

    static Void Initialize();

    static Void Terminate();

    static UInt64 CaptureStackTraceHash();

    static UInt32 CaptureStackTrace(
        TraceInfo* infoBuffer,
        UInt32 bufferCount
    );
};




} // namespace System
} // namespace Cider

