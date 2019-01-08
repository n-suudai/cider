
#pragma once

#include "dlmalloc/malloc.h"
#include "System/Types.hpp"
#include <chrono>
#include <mutex>
#include <atomic>
#include <array>


namespace Cider {
namespace System {


enum class MEMORY_AREA
{
    UNKNOWN,
    DEBUG,
    STL,
    SYSTEM,
    APPLICATION,
    NUM,
};


class MemorySpace
{
public:
    MemorySpace();

    ~MemorySpace();

    Bool CreateMemorySpace(const Char* name, SizeT capacity);

    Void DestroyMemorySpace();

    Void* Malloc(SizeT bytes, SizeT alignment);

    Void* Realloc(Void *memory, SizeT newsize);

    Void Free(Void* memory);

private:
    mspace m_mspace;
    SizeT m_capacity;
    Char   m_name[128];
};


class MemoryManager
{
public:
    struct DebugInfo
    {
        Void*        address;
        SizeT       bytes;
        const Char*  file;
        Int32 line;
        MEMORY_AREA  area;
        std::chrono::system_clock::time_point date;
        UInt64 stackTraceHash;
        UInt64 bookmark;

        DebugInfo();

        Void Clear();

        Void PrintInfo(Bool newLine = false);
    };

    static Bool Initialize();

    static Void Terminate();

    static Void* MallocDebug(const Char* file, Int32 line, MEMORY_AREA area, SizeT bytes, SizeT alignment = DEFAULT_ALIGNMENT_SIZE);

    static Void* Malloc(MEMORY_AREA area, SizeT bytes, SizeT alignment = DEFAULT_ALIGNMENT_SIZE);

    static Void Free(MEMORY_AREA area, Void* memory);

    static Void PrintDebugInfo();

    static Void ReportLeaks(UInt64 bookmark);
    static Void ReportLeaks(UInt64 bookmark1, UInt64 bookmark2);

    static Void CheckTrap(UInt64 bookmark);
    static Void CheckTrap(UInt64 bookmark1, UInt64 bookmark2);

    static UInt64 GetBookmark();

private:
    static DebugInfo * FindInfo(Void* address);
    static Void SetInfo(const DebugInfo& info);


private:
    static const SizeT DEFAULT_ALIGNMENT_SIZE;
    static const SizeT MEMORY_TRAP_SIZE;
    static const UInt32 MEMORY_TRAP;

    static std::mutex  m_memoryLock;
    static MemorySpace m_memorySpace[static_cast<Int32>(MEMORY_AREA::NUM)];

    static std::mutex m_infoLock;
    static std::array<DebugInfo, 1024> m_memoryInfo;

    static std::atomic<UInt64> m_allocCount;
    static std::atomic<UInt64> m_instanceCount;

    static Bool m_initialized;
};


template<MEMORY_AREA AREA>
struct BaseAllocator
{
    static constexpr MEMORY_AREA AREA_TYPE = AREA;

    BaseAllocator() = default;

    virtual ~BaseAllocator() = default;

    Void* operator new(SizeT bytes)
    {
        return MemoryManager::MallocDebug(__FILE__, __LINE__, AREA_TYPE, bytes);
    }

    Void* operator new(SizeT bytes, const Char* file, Int32 line)
    {
        return MemoryManager::MallocDebug(file, line, AREA_TYPE, bytes);
    }

    Void* operator new[](SizeT bytes)
    {
        return BaseAllocator::operator new(bytes);
    }

    Void* operator new[](SizeT bytes, const Char* file, Int32 line)
    {
        return BaseAllocator::operator new(bytes, file, line);
    }

    Void operator delete(Void* memory)
    {
        MemoryManager::Free(AREA_TYPE, memory);
    }

    Void operator delete(Void* memory, const Char*, Int32)
    {
        MemoryManager::Free(AREA_TYPE, memory);
    }

    Void operator delete[](Void* memory)
    {
        BaseAllocator::operator delete(memory);
    }

    Void operator delete[](Void* memory, const Char* file, Int32 line)
    {
        BaseAllocator::operator delete(memory, file, line);
    }
};


} // namespace System
} // namespace Cider


Cider::Void* operator new(Cider::SizeT bytes);
Cider::Void* operator new(Cider::SizeT bytes, const Cider::Char* file, Cider::Int32 line);
Cider::Void* operator new[](Cider::SizeT bytes);
Cider::Void* operator new[](Cider::SizeT bytes, const Cider::Char* file, Cider::Int32 line);

Cider::Void operator delete(Cider::Void* memory);
Cider::Void operator delete(Cider::Void* memory, const Cider::Char* file, Cider::Int32 line);
Cider::Void operator delete[](Cider::Void* memory);
Cider::Void operator delete[](Cider::Void* memory, const Cider::Char* file, Cider::Int32 line);


#define CIDER_NEW new(__FILE__, __LINE__)
#define CIDER_DELETE delete

