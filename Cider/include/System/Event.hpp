
#pragma once

#include "System/Types.hpp"
#include "System/STL.hpp"
#include "System/Assert.hpp"
#include "System/Signals.hpp"
#include <type_traits>
#include <typeinfo>
#include <utility>


namespace Cider {
namespace System {
namespace Detail {


template<typename T>
struct EventHashCode final
{
    static_assert(!std::is_pointer_v<T>, "<T> is not pointer.");
    static_assert(std::is_object_v<T>, "<T> is object type.");

    static const SizeT Value;
};


template<typename T>
const SizeT EventHashCode<T>::Value = typeid(const T*).hash_code();


template<MEMORY_AREA AREA>
struct EventBody : public BaseAllocator<AREA>
{
    EventBody() = default;

    EventBody(const EventBody&) = delete;
    void operator=(const EventBody&) = delete;

    virtual ~EventBody() = default;

    virtual SizeT HashCode() const = 0;
};


template<typename T, MEMORY_AREA AREA>
struct EventBodyOverride final : public EventBody<AREA>
{
    static_assert(!std::is_reference_v<T>, "reference type is not supported.");
    static_assert(!std::is_pointer_v<T>, "pointer type is not supported.");
    static_assert(std::is_object_v<T>, "<T> is object type.");

    template<typename...Arguments>
    explicit EventBodyOverride(Arguments&&...arguments)
        : data(std::forward<Arguments>(arguments)...)
    {}

    SizeT HashCode() const override
    {
        return EventHashCode<T>::Value;
    }

    T data;
};


} // namespace Detail


template<MEMORY_AREA AREA>
class Event final
{
    template<typename T>
    using EventBodyOverrideType = Detail::EventBodyOverride<T, AREA>;
    typedef Detail::EventBody<AREA> EventBodyType;
public:
    Event() = delete;
    Event(const Event&) = delete;
    void operator=(const Event&) = delete;

    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    template<typename T>
    explicit Event(T&& value)
    {
        typedef std::remove_reference_t<T> ValueType;
        typedef EventBodyOverrideType<ValueType> ContainerType;

        static_assert(!std::is_same_v<Event, ValueType>, "");
        static_assert(std::is_object_v<ValueType>, "<T> is object type.");
        static_assert(!std::is_pointer_v<ValueType>, "pointer type is not supported.");
        static_assert(std::is_base_of_v<EventBodyType, ContainerType>, "Container is not a base class of 'EventBody'.");

        m_body = STL::make_unique<ContainerType>(std::forward<T>(value));
    }

    template<typename T>
    Bool Is() const
    {
        static_assert(!std::is_reference_v<T>, "reference type is not supported.");
        static_assert(!std::is_pointer_v<T>, "pointer type is not supported.");
        static_assert(std::is_object_v<T>, "<T> is object type.");

        CIDER_ASSERT(m_body, "");
        return m_body && (m_body->HashCode() == Detail::EventHashCode<T>::Value);
    }

    template<typename T>
    const T* As() const
    {
        typedef EventBodyOverrideType<T> ContainerType;

        static_assert(!std::is_reference_v<T>, "reference type is not supported.");
        static_assert(!std::is_pointer_v<T>, "pointer type is not supported.");
        static_assert(std::is_object_v<T>, "<T> is object type.");
        static_assert(std::is_base_of_v<EventBodyType, ContainerType>, "Container is not a base class of 'EventBody'.");

        CIDER_ASSERT(m_body, "");

        if (auto p = dynamic_cast<const ContainerType*>(m_body.get()))
        {
            CIDER_ASSERT(Is<T>(), "");
            return &p->data;
        }
        return nullptr;
    }

private:
    STL::unique_ptr<EventBodyType> m_body;
};


template<MEMORY_AREA AREA = MEMORY_AREA::SYSTEM>
class EventQueue final
{
public:
    typedef Event<AREA> EventType;

    EventQueue()
        : m_signalBody(STL::make_shared<SignalBody>())
    {}

    EventQueue(const EventQueue&) = delete;
    void operator=(const EventQueue&) = delete;

    EventQueue(EventQueue&&) = delete;
    void operator=(EventQueue&&) = delete;

    Connection Connect(const Slot<void(const EventType&)>& slot)
    {
        CIDER_ASSERT(slot, "");
        CIDER_ASSERT(m_signalBody, "");
        return Connection { m_signalBody->Connect(slot) };
    }

    Connection Connect(Slot<void(const EventType&)>&& slot)
    {
        CIDER_ASSERT(slot, "");
        CIDER_ASSERT(m_signalBody, "");
        return Connection { m_signalBody->Connect(slot) };
    }

    void Enqueue(EventType&& eventValue)
    {
        std::lock_guard<std::recursive_mutex> lock(m_notificationProtection);
        m_events.emplace_back(std::move(eventValue));
    }

    template<typename T, typename...Arguments>
    void Enqueue(Arguments&&...arguments)
    {
        Enqueue(EventType { T{std::forward<Arguments>(arguments)...} });
    }

    void Emit()
    {
        CIDER_ASSERT(m_signalBody, "");

        std::vector<EventType> notifications;
        {
            std::lock_guard<std::recursive_mutex> lock(m_notificationProtection);
            std::swap(notifications, m_events);
        }

        for (auto& notification : notifications)
        {
            m_signalBody->operator()(notification);
        }
    }

private:
    typedef Detail::SignalBody<void(const EventType&)> SignalBody;
    std::vector<EventType> m_events;
    std::shared_ptr<SignalBody> m_signalBody;
    std::recursive_mutex m_notificationProtection;
};


typedef EventQueue<MEMORY_AREA::SYSTEM> SystemEventQueue;
typedef SystemEventQueue::EventType     SystemEvent;


} // namespace System
} // namespace Cider

