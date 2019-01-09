

#include "System/Memory.hpp"
#include "System/StackTrace.hpp"
#include "System/Log.hpp"
#include "System/Assert.hpp"
#include <ctime>
#include <iomanip>


namespace {

tm GetLocalTime(const std::chrono::system_clock::time_point &p)
{
    time_t t = std::chrono::system_clock::to_time_t(p);

    tm localTime;
    localtime_s(&localTime, &t);
    return localTime;
}

} // namespace /* unnamed */


namespace Cider {
namespace System {


#pragma warning(push)
#pragma warning(disable: 4074)
#pragma init_seg(compiler)


std::mutex          MemoryManager::m_memoryLock;
MemorySpace         MemoryManager::m_memorySpace[static_cast<Int32>(MEMORY_AREA::NUM)];
std::mutex          MemoryManager::m_infoLock;
Bool                MemoryManager::m_initialized = false;
std::array<MemoryManager::DebugInfo, 1024> MemoryManager::m_memoryInfo;
std::atomic<UInt64>  MemoryManager::m_allocCount = 0;
std::atomic<UInt64>  MemoryManager::m_instanceCount = 0;

class Initialize
{
public:
    Initialize()
    {
        MemoryManager::Initialize();
        StackTrace::Initialize();
    }

    ~Initialize()
    {
        StackTrace::Terminate();
        MemoryManager::Terminate();
    }
};

Initialize init;

#pragma warning(pop)


MemorySpace::MemorySpace()
    : m_mspace(nullptr)
    , m_capacity(0)
    , m_name("")
{

}

MemorySpace::~MemorySpace()
{

}

Bool MemorySpace::CreateMemorySpace(const Char* name, SizeT capacity)
{
    m_capacity = capacity;
    strcpy_s(m_name, name);
    m_mspace = create_mspace(capacity, 0);

    CIDER_ASSERT(m_mspace != nullptr, "メモリ領域の作成に失敗しました。");

    return m_mspace != nullptr;
}

Void MemorySpace::DestroyMemorySpace()
{
    destroy_mspace(m_mspace);
    m_mspace = nullptr;
}

Void* MemorySpace::Malloc(SizeT bytes, SizeT alignment)
{
    return mspace_memalign(m_mspace, alignment, bytes);
}

Void* MemorySpace::Realloc(Void *memory, SizeT newsize)
{
    return mspace_realloc(m_mspace, memory, newsize);
}

Void MemorySpace::Free(Void* memory)
{
    mspace_free(m_mspace, memory);
}


Bool MemoryManager::Initialize()
{
    constexpr SizeT KB = 1024;
    constexpr SizeT MB = KB * 1024;
    constexpr SizeT GB = MB * 1024;

    struct {
        const Char* name;
        SizeT       capacity;
    }
    const initInfos[static_cast<Int32>(MEMORY_AREA::NUM)] = {
        { "UNKNOWN",        512     },
        { "DEBUG",          512     },
        { "STL",            1  * KB },
        { "SYSTEM",         10 * KB },
        { "GRAPHICS",       10 * KB },
        { "APPLICATION",    10 * KB },
    };

    for (Int32 i = 0; i < static_cast<Int32>(MEMORY_AREA::NUM); ++i)
    {
        m_memorySpace[i].CreateMemorySpace(initInfos[i].name, initInfos[i].capacity);
    }

    m_initialized = true;
    return true;
}

Void MemoryManager::Terminate()
{
    for (int i = 0; i < static_cast<Int32>(MEMORY_AREA::NUM); ++i)
    {
        m_memorySpace[i].DestroyMemorySpace();
    }
    m_initialized = false;
}

Void* MemoryManager::MallocDebug(const Char* file, Int32 line, MEMORY_AREA area, SizeT bytes, SizeT alignment)
{
    // メモリトラップのサイズをプラス
    SizeT allocSize = bytes + MEMORY_TRAP_SIZE;

    Void* address = MemoryManager::Malloc(area, allocSize, alignment);

    // デバッグ情報を保存
    if (address)
    {
        UInt32* trap = (UInt32*)((PtrDiff)address + bytes);
        (*trap) = MEMORY_TRAP;

        std::lock_guard<std::mutex> lock(m_infoLock);

        DebugInfo info;
        info.address = address;
        info.file = file;
        info.area = area;
        info.line = line;
        info.bytes = bytes;
        info.date = std::chrono::system_clock::now();
        info.stackTraceHash = StackTrace::CaptureStackTraceHash();
        info.bookmark = m_allocCount;

        SetInfo(info);
    }

    m_instanceCount++;
    m_allocCount++;

    return address;
}

Void* MemoryManager::Malloc(MEMORY_AREA area, SizeT bytes, SizeT alignment)
{
    if (!m_initialized)
    {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_memoryLock);

    return m_memorySpace[static_cast<Int32>(area)].Malloc(bytes, alignment);
}

Void MemoryManager::Free(MEMORY_AREA area, Void* memory)
{
    if (memory)
    {
        std::lock_guard<std::mutex> lock(m_infoLock);

        auto info = FindInfo(memory);

        if (info)
        {
            info->Clear();
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_memoryLock);

        m_memorySpace[static_cast<Int32>(area)].Free(memory);
    }

    m_instanceCount--;
}

Void MemoryManager::PrintDebugInfo()
{
    std::lock_guard<std::mutex> lock(m_infoLock);

    // 確保順にソート
    std::sort(
        m_memoryInfo.begin(),
        m_memoryInfo.end(),
        [](const DebugInfo& v1, const DebugInfo& v2) -> Bool {
        return v1.date < v2.date;
    }
    );

    Log::Message("----------------------------------------\n");

    for (auto& info : m_memoryInfo)
    {
        info.PrintInfo();
    }

    Log::Message("----------------------------------------\n");
}

UInt64 MemoryManager::GetBookmark()
{
    return m_allocCount;
}

Void MemoryManager::ReportLeaks(UInt64 bookmark)
{
    ReportLeaks(bookmark, GetBookmark());
}

Void MemoryManager::ReportLeaks(UInt64 bookmark1, UInt64 bookmark2)
{
    std::lock_guard<std::mutex> lock(m_infoLock);

    Int32 leakCount = 0;

    Log::Message("========================================\n");

    Log::Format("【 メモリリークチェック [%llX - %llX] 】\n", bookmark1, bookmark2);

    for (auto& info : m_memoryInfo)
    {
        if (info.address == nullptr) { continue; }

        if (info.bookmark >= bookmark1 && info.bookmark < bookmark2)
        {
            Log::Message("----------------------------------------\n");
            info.PrintInfo(true);

            leakCount++;
        }
    }

    Log::Message("----------------------------------------\n");

    if (leakCount > 0)
    {
        Log::Format("【 %d件のメモリリークが検出されました 】\n", leakCount);
    }
    else
    {
        Log::Format("【 メモリリークは検出されませんでした 】\n");
    }

    Log::Message("========================================\n");
}

Void MemoryManager::CheckTrap(UInt64 bookmark)
{
    CheckTrap(bookmark, GetBookmark());
}

Void MemoryManager::CheckTrap(UInt64 bookmark1, UInt64 bookmark2)
{
    std::lock_guard<std::mutex> lock(m_infoLock);

    Int32 brokenCount = 0;

    Log::Message("========================================\n");

    Log::Format("【 メモリ破壊チェック [%llX - %llX] 】\n", bookmark1, bookmark2);

    for (auto& info : m_memoryInfo)
    {
        if (info.address == nullptr) { continue; }

        if (info.bookmark >= bookmark1 && info.bookmark < bookmark2)
        {
            UInt32* trap = (UInt32*)((PtrDiff)info.address + info.bytes);

            if ((*trap) != MEMORY_TRAP)
            {
                Log::Message("----------------------------------------\n");

                info.PrintInfo(true);

                brokenCount++;
            }
        }
    }

    Log::Message("----------------------------------------\n");

    if (brokenCount > 0)
    {
        Log::Format("【 %d件のメモリ破壊を検出しました 】\n", brokenCount);
    }
    else
    {
        Log::Format("【 メモリ破壊は検出されませんでした 】\n");
    }

    Log::Message("========================================\n");
}

MemoryManager::DebugInfo * MemoryManager::FindInfo(Void* address)
{
    auto result = std::find_if(
        m_memoryInfo.begin(),
        m_memoryInfo.end(),
        [address](const DebugInfo& info) -> Bool {
        return info.address == address;
    }
    );

    if (result != m_memoryInfo.end())
    {
        return &(*result);
    }

    return nullptr;
}

Void MemoryManager::SetInfo(const DebugInfo& info)
{
    auto p = FindInfo(info.address);

    if (p)
    {
        (*p) = info;
    }
    else
    {
        auto it = std::find_if(
            m_memoryInfo.begin(),
            m_memoryInfo.end(),
            [](const DebugInfo& v) -> Bool {
            return v.address == nullptr;
        }
        );

        (*it) = info;
    }
}


MemoryManager::DebugInfo::DebugInfo()
{
    Clear();
}

Void MemoryManager::DebugInfo::Clear()
{
    address = nullptr;
    bytes = 0;
    file = "";
    line = 0;
    area = MEMORY_AREA::UNKNOWN;
    date = std::chrono::system_clock::time_point();
    stackTraceHash = 0;
    bookmark = 0;
}

Void MemoryManager::DebugInfo::PrintInfo(Bool newLine)
{
    if (address == nullptr) { return; }

    UInt32* trap = (UInt32*)((PtrDiff)address + bytes);

    static const Char* s_memoryAreaName[static_cast<Int32>(MEMORY_AREA::NUM)] = {
        "UNKNOWN",
        "DEBUG",
        "STL",
        "SYSTEM",
        "APPLICATION",
    };

    static Char dateBuffer[256];
    {
        tm localTime = GetLocalTime(date);
        const Char* format = "%Y-%m-%d %H:%M:%S";
        struct tm*  tm = &localTime;
        strftime(dateBuffer, sizeof(dateBuffer), format, tm);
    }

    Log::Format(
        newLine ?
        "%s(%d)\n{ area=\"%s\" address=0x%p size=%zubyte time=%s backTraceHash=0x%016llX }\n[ %08X ]\n" :
        "%s(%d) : { area=\"%s\" address=0x%p size=%zubyte time=%s backTraceHash=0x%016llX } [ %08X ]\n",
        file,
        line,
        s_memoryAreaName[static_cast<Int32>(area)],
        address,
        bytes,
        dateBuffer,
        stackTraceHash,
        (*trap)
    );
}


} // namespace System
} // namespace Cider


using namespace Cider;
using namespace Cider::System;

Void* operator new(SizeT bytes)
{
    return MemoryManager::MallocDebug(__FILE__, __LINE__, MEMORY_AREA::UNKNOWN, bytes);
}

Void* operator new(SizeT bytes, const Char* file, Int32 line)
{
    return MemoryManager::MallocDebug(file, line, MEMORY_AREA::UNKNOWN, bytes);
}

Void* operator new[](SizeT bytes)
{
    return ::operator new(bytes);
}

Void* operator new[](SizeT bytes, const Char* file, Int32 line)
{
    return ::operator new(bytes, file, line);
}


Void operator delete(Void* memory)
{
    MemoryManager::Free(MEMORY_AREA::UNKNOWN, memory);
}

Void operator delete(Void* memory, const Char*, Int32)
{
    MemoryManager::Free(MEMORY_AREA::UNKNOWN, memory);
}

Void operator delete[](Void* memory)
{
    ::operator delete(memory);
}

Void operator delete[](Void* memory, const Char* file, Int32 line)
{
    ::operator delete(memory, file, line);
}

