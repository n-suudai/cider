

#include "System/StackTrace.hpp"
#include "System/Log.hpp"

#include <Windows.h>
#include <dbghelp.h>
#include <mutex>

#pragma comment(lib, "imagehlp.lib")
#pragma comment(lib, "Kernel32.lib")


namespace {


using namespace Cider;


#pragma warning(push)
#pragma warning(disable: 4074)
#pragma init_seg(compiler)

static HANDLE g_Process = NULL;
static Bool   g_IsSymbolEngineReady = false;

#pragma warning(pop)


static Void AddressToTraceInfo(Void* address, Cider::System::StackTrace::TraceInfo& outInfo)
{
    outInfo.Clear();
    outInfo.address = address;

    if (!g_IsSymbolEngineReady)
    {
        return;
    }

    //モジュール名
    IMAGEHLP_MODULE64 imageModule = { sizeof(IMAGEHLP_MODULE64) };
    BOOL r = ::SymGetModuleInfo64(g_Process, (DWORD64)address, &imageModule);
    if (!r)
    {
        return;
    }

    // モジュール名コピー
    strcpy_s(outInfo.moduleName, imageModule.ModuleName);

    //シンボル情報格納バッファ.
    IMAGEHLP_SYMBOL64 * imageSymbol;
    char buffer[MAX_PATH + sizeof(IMAGEHLP_SYMBOL64)] = { 0 };
    imageSymbol = (IMAGEHLP_SYMBOL64*)buffer;
    imageSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
    imageSymbol->MaxNameLength = MAX_PATH;

    //関数名の取得...
    {
        DWORD64 disp = 0;
        r = ::SymGetSymFromAddr64(g_Process, (DWORD64)address, &disp, imageSymbol);
        if (!r)
        {
            return;
        }
    }

    // 関数名コピー
    strcpy_s(outInfo.function, imageSymbol->Name);

    {
        DWORD disp = 0;
        //行番号の取得
        IMAGEHLP_LINE64 line = { sizeof(IMAGEHLP_LINE64) };
        r = ::SymGetLineFromAddr64(g_Process, (DWORD64)address, &disp, &line);

        outInfo.lineAddress = (Void*)line.Address;
        if (!r)
        {
            return;
        }

        strcpy_s(outInfo.file, line.FileName);
        outInfo.line = (Int32)line.LineNumber;
    }
}


} // namespace /* unnamed */


namespace Cider {
namespace System {


StackTrace::TraceInfo::TraceInfo()
{
    Clear();
}

Void StackTrace::TraceInfo::Clear()
{
    strcpy_s(function, "???");
    strcpy_s(file, "???");
    strcpy_s(moduleName, "???");
    line = -1;
    lineAddress = nullptr;
    address = nullptr;
}

Void StackTrace::TraceInfo::Print()
{
    if (line == -1)
    {
        Log::Format(
            "0x%p @ %s @ %s + 0x%p\n",
            address,
            moduleName,
            function,
            address
        );
    }
    else
    {
        Log::Format(
            "0x%p @ %s @ %s @ %s(%d)\n",
            address,
            moduleName,
            function,
            file,
            line
        );
    }
}

Void StackTrace::Initialize()
{
    g_IsSymbolEngineReady = false;

    g_Process = ::GetCurrentProcess();

    ::SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    if (::SymInitialize(g_Process, NULL, TRUE))
    {
        g_IsSymbolEngineReady = true;
    }
}

Void StackTrace::Terminate()
{
    if (g_IsSymbolEngineReady)
    {
        ::SymCleanup(g_Process);
        g_IsSymbolEngineReady = false;
    }
}

UInt64 StackTrace::CaptureStackTraceHash()
{
    const ULONG bufferCount = 62;
    Void* buffer[bufferCount] = { nullptr };

    ULONG h = 0;
    ::RtlCaptureStackBackTrace(0, (ULONG)bufferCount, buffer, &h);

    return (UInt64)h;
}

UInt32 StackTrace::CaptureStackTrace(
    TraceInfo* infoBuffer,
    UInt32 bufferCount
)
{
    if (bufferCount >= 63)
    {
        bufferCount = 62;
    }

    Void* buffer[62] = { nullptr };

    ULONG h = 0;
    UInt32 captureCount = (UInt32)::RtlCaptureStackBackTrace(0, (ULONG)bufferCount, buffer, &h);

    for (UInt32 i = 0; i < captureCount; ++i)
    {
        if (i > bufferCount) { break; }

        ::AddressToTraceInfo(buffer[i], infoBuffer[i]);
    }

    return captureCount;
}


} // namespace System
} // namespace Cider

