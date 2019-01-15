
#pragma once

#include "System/Memory.hpp"

#include <memory>

#include <string>
#include <sstream>

#include <vector>
#include <list>
#include <forward_list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <deque>
#include <queue>
#include <stack>


namespace Cider {
namespace STL {
namespace Detail {


// カスタムデリータ unique_ptr用
template<typename T>
struct CustomDeleter
{
    CustomDeleter() = default;

    template<
        typename T2,
        std::enable_if_t<std::is_convertible_v<T2 *, T *>, Int32> = 0>
        CustomDeleter(const CustomDeleter<T2>&)
    {// construct from another default_delete
    }

    Void operator()(T* ptr) const
    {
        static_assert(
            0 < sizeof(T),
            "can't delete an incomplete type"
            );

        CIDER_DELETE ptr;
    }
};

// カスタムデリータ 配列型のunique_ptr用
template<typename T>
struct CustomDeleter<T[]>
{
    CustomDeleter() = default;

    template<typename T2,
        std::enable_if_t<std::is_convertible_v<T2(*)[], T(*)[]>, Int32> = 0>
        CustomDeleter(const CustomDeleter<T2[]>&)
    {// construct from another default_delete
    }

    template<typename T2,
        std::enable_if_t<std::is_convertible_v<T2(*)[], T(*)[]>, Int32> = 0>
        Void operator()(T2 * ptr) const
    {// delete a pointer
        static_assert(
            0 < sizeof(T2),
            "can't delete an incomplete type"
            );

        CIDER_DELETE[] ptr;
    }
};


} // namespace Detail


// allocator
template<typename T, System::MEMORY_AREA AREA = System::MEMORY_AREA::STL>
struct StdAllocator
{
    using value_type = T;

    template<typename U>
    struct rebind {
        typedef StdAllocator<U, AREA> other;
    };

    StdAllocator()
    { /* DO_NOTHING */
    }

    template<typename U>
    StdAllocator(const StdAllocator<U, AREA>&)
    { /* DO_NOTHING */
    }

    T* allocate(SizeT count)
    {
        return reinterpret_cast<T*>(System::MemoryManager::MallocDebug(
            __FILE__,
            __LINE__,
            AREA,
            sizeof(T) * count,
            alignof(T)
        ));
    }

    Void deallocate(T* ptr, SizeT)
    {
        
        System::MemoryManager::Free(
            AREA,
            reinterpret_cast<Void*>(ptr)
        );
    }
};

template<typename T, typename U, System::MEMORY_AREA AREA = System::MEMORY_AREA::STL>
Bool operator == (const StdAllocator<T, AREA>&, const StdAllocator<U, AREA>&)
{
    return true;
}

template<typename T, typename U, System::MEMORY_AREA AREA = System::MEMORY_AREA::STL>
Bool operator != (const StdAllocator<T, AREA>&, const StdAllocator<U, AREA>&)
{
    return false;
}


// shared_ptr
template<typename T>
using shared_ptr = std::shared_ptr<T>;

// weak_ptr
template<typename T>
using weak_ptr = std::weak_ptr<T>;

// unique_ptr
template<
    typename T,
    typename Deleter = Detail::CustomDeleter<T>
>
using unique_ptr = std::unique_ptr<T, Deleter>;


// ※ T::AREA_TYPE が無い場合は、直接指定する
template<typename T, System::MEMORY_AREA AREA = T::AREA_TYPE, typename ... Arguments>
inline shared_ptr<T> make_shared(Arguments && ... arguments)
{
    static StdAllocator<T, AREA> allocator = StdAllocator<T, AREA>();
    return std::allocate_shared<T>(allocator, std::forward<Arguments>(arguments)...);
}

template<
    typename T,
    typename... Arguments,
    std::enable_if_t<!std::is_array_v<T>, Int32> = 0
>
inline unique_ptr<T> make_unique(Arguments && ... arguments)
{
    static Detail::CustomDeleter<T> deleter = Detail::CustomDeleter<T>();

    return unique_ptr<T>(CIDER_NEW T(std::forward<Arguments>(arguments)...), deleter);
}

template<
    typename T,
    std::enable_if_t<std::is_array_v<T> && std::extent<T>::value == 0, Int32> = 0
>
inline unique_ptr<T> make_unique(SizeT size)
{
    typedef std::remove_extent_t<T> Element;

    static Detail::CustomDeleter<T> deleter = Detail::CustomDeleter<T>();

    return (unique_ptr<T>(CIDER_NEW Element[size](), deleter));
}

template<typename T,
    class... Arguments,
    std::enable_if_t<std::extent<T>::value != 0, Int32> = 0
>
Void make_unique(Arguments&&...) = delete;


// basic_string
template<
    typename Element,
    typename Traits = std::char_traits<Element>,
    typename Allocator = StdAllocator<Element, System::MEMORY_AREA::STL>
>
using  basic_string = std::basic_string<Element, Traits, Allocator>;

// string
typedef basic_string<Char> string;
typedef basic_string<WChar> wstring;

// basic_stringstream
template<
    typename Element,
    typename Traits = std::char_traits<Element>,
    typename Allocator = StdAllocator<Element, System::MEMORY_AREA::STL>
>
using basic_stringstream = std::basic_stringstream<Element, Traits, Allocator>;

// stringstream
typedef basic_stringstream<Char> stringstream;
typedef basic_stringstream<WChar> wstringstream;

// vector
template<
    typename T,
    typename Allocator = StdAllocator<T, System::MEMORY_AREA::STL>
>
using vector = std::vector<T, Allocator>;

// list
template<
    typename T,
    typename Allocator = StdAllocator<T, System::MEMORY_AREA::STL>
>
using list = std::list<T, Allocator>;

// forward_list
template<
    typename T,
    typename Allocator = StdAllocator<T, System::MEMORY_AREA::STL>
>
using forward_list = std::forward_list<T, Allocator>;

// map
template<
    typename Key,
    typename T,
    typename Compare = std::less<Key>,
    typename Allocator = StdAllocator<std::pair<const Key, T>, System::MEMORY_AREA::STL>
>
using map = std::map<Key, T, Compare, Allocator>;

// multimap
template<
    typename Key,
    typename T,
    typename Compare = std::less<Key>,
    typename Allocator = StdAllocator<std::pair<const Key, T>, System::MEMORY_AREA::STL>
>
using multimap = std::multimap<Key, T, Compare, Allocator>;

// unordered_map
template<
    typename Key,
    typename T,
    typename Hasher = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = StdAllocator<std::pair<const Key, T>, System::MEMORY_AREA::STL>
>
using unordered_map = std::unordered_map<Key, T, Hasher, KeyEqual, Allocator>;

// unordered_multimap
template<
    typename Key,
    typename T,
    typename Hasher = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = StdAllocator<std::pair<const Key, T>, System::MEMORY_AREA::STL>
>
using unordered_multimap = std::unordered_multimap<Key, T, Hasher, KeyEqual, Allocator>;

// set
template<
    typename Key,
    typename Compare = std::less<Key>,
    typename Allocator = StdAllocator<Key, System::MEMORY_AREA::STL>
>
using set = std::set<Key, Compare, Allocator>;

// multiset
template<
    typename Key,
    typename Compare = std::less<Key>,
    typename Allocator = StdAllocator<Key, System::MEMORY_AREA::STL>
>
using multiset = std::multiset<Key, Compare, Allocator>;

// unordered_set
template<
    typename Key,
    typename Hasher = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = StdAllocator<Key, System::MEMORY_AREA::STL>
>
using unordered_set = std::unordered_set<Key, Hasher, KeyEqual, Allocator>;

// unordered_set
template<
    typename Key,
    typename Hasher = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = StdAllocator<Key, System::MEMORY_AREA::STL>
>
using unordered_multiset = std::unordered_multiset<Key, Hasher, KeyEqual, Allocator>;

// deque
template<
    typename T,
    typename Allocator = StdAllocator<T, System::MEMORY_AREA::STL>
>
using deque = std::deque<T, Allocator>;

// queue
template<
    typename T,
    typename Container = deque<T>
>
using queue = std::queue<T, Container>;

// priority_queue
template<
    typename T,
    typename Container = vector<T>,
    typename Compare = std::less<typename Container::value_type>
>
using priority_queue = std::priority_queue<T, Container, Compare>;

// stack
template<
    typename T,
    typename Container = deque<T>
>
using stack = std::stack<T, Container>;


} // namespace STL
} // namespace Cider

