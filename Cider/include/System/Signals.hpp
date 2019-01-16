
#pragma once


#include "System/Memory.hpp"
#include "System/STL.hpp"
#include "System/Assert.hpp"
#include <mutex>
#include <algorithm>



// TODO : メモリの確保領域を変更できるようにする


namespace Cider {
namespace System {


template<typename Function>
using Slot = std::function<Function>;

template<typename Result, typename... Arguments>
class Signal;

namespace Detail {

template<typename Result, typename... Arguments>
class SignalBody;


class ConnectionBody : public BaseAllocator<MEMORY_AREA::SYSTEM>
{
public:
    virtual ~ConnectionBody() = default;

    virtual Void Disconnect() = 0;

    virtual Bool Valid() const = 0;

    virtual STL::unique_ptr<ConnectionBody> DeepCopy() const = 0;
};


template<typename Function>
class ConnectionBodyOverride final : public ConnectionBody
{
private:
    typedef Slot<Function> SlotType;
    typedef STL::weak_ptr<SlotType> WeakSlot;

    typedef SignalBody<Function> SignalBodyType;
    typedef STL::weak_ptr<SignalBodyType> WeakSignalBody;

public:
    ConnectionBodyOverride(WeakSignalBody&& weakSignalBody, WeakSlot&& weakSlot)
        : m_weakSignalBody(std::forward<WeakSignalBody>(weakSignalBody))
        , m_weakSlot(std::forward<WeakSlot>(weakSlot))
    {}

    ConnectionBodyOverride(const WeakSignalBody& weakSignalBody, const WeakSlot& weakSlot)
        : m_weakSignalBody(weakSignalBody)
        , m_weakSlot(weakSlot)
    {}

    ~ConnectionBodyOverride() = default;

    Void Disconnect() override
    {
        if (!m_weakSlot.expired())
        {
            return;
        }

        auto lockedSlot = m_weakSlot.lock();

        if (auto lockedSignalBody = m_weakSignalBody.lock())
        {
            lockedSignalBody->Disconnect(lockedSlot.get());
            m_weakSignalBody.reset();
        }

        m_weakSlot.reset();
    }

    Bool Valid() const override
    {
        return !m_weakSlot.expired() && !m_weakSignalBody.expired();
    }

    STL::unique_ptr<ConnectionBody> DeepCopy() const override
    {
        return STL::make_unique<ConnectionBodyOverride>(m_weakSignalBody, m_weakSlot);
    }

private:
    WeakSlot        m_weakSlot;
    WeakSignalBody  m_weakSignalBody;
};


template<typename Result, typename... Arguments>
class SignalBody<Result(Arguments...)> final
    : public std::enable_shared_from_this< SignalBody<Result(Arguments...)>>
    , public BaseAllocator<MEMORY_AREA::SYSTEM>
{
private:
    typedef Slot<Result(Arguments...)> SlotType;
    typedef ConnectionBodyOverride<Result(Arguments...)> ConnectionBodyOverrideType;

    typedef std::vector<Result> ResultArray;

public:
    SignalBody() = default;

    SignalBody(const SignalBody&) = delete;
    Void operator=(const SignalBody&) = delete;

    SignalBody(SignalBody&&) = delete;
    Void operator=(SignalBody&&) = delete;

    template<typename Function>
    STL::unique_ptr<ConnectionBodyOverrideType> Connect(Function&& slot)
    {
        CIDER_ASSERT(slot, "");

        auto observer = STL::make_shared<SlotType, MEMORY_AREA::SYSTEM>(std::forward<Function>(slot));
        {
            std::lock_guard< std::recursive_mutex> lock(m_addingProtection);

            CIDER_ASSERT(
                std::end(m_addedObservers) ==
                std::find(std::begin(m_addedObservers), std::end(m_addedObservers), observer),
                ""
            );

            m_addedObservers.push_back(observer);
        }

        STL::weak_ptr<SignalBody> weakSignal = this->shared_from_this();

        CIDER_ASSERT(!weakSignal.expired(), "");

        return STL::make_unique<ConnectionBodyOverrideType>(std::move(weakSignal), observer);
    }

    Void Disconnect(const SlotType* observer)
    {
        CIDER_ASSERT(observer, "");

        {
            std::lock_guard<std::recursive_mutex> lock(m_addingProtection);

            m_addedObservers.erase(
                std::remove_if(
                    std::begin(m_addedObservers),
                    std::end(m_addedObservers),
                    [observer](const STL::shared_ptr<SlotType>& slot) -> Bool {
                return slot.get() == observer;
            }
                ),
                std::end(m_addedObservers)
                );
        }

        auto const it = std::find_if(
            std::begin(m_observers),
            std::end(m_observers),
            [observer](const STL::shared_ptr<SlotType>& slot) -> Bool {
            return slot.get() == observer;
        }
        );

        if (it != std::end(m_observers))
        {
            it->reset();
        }
    }

    ResultArray operator()(Arguments&&... arguments)
    {
        ResultArray resultArray;

        if (m_nestedMethodCallCount <= 0)
        {
            PushBackAddedListeners();
        }

        if (m_nestedMethodCallCount >= std::numeric_limits<std::int16_t>::max())
        {
            return resultArray;
        }

        CIDER_ASSERT(m_nestedMethodCallCount >= 0, "");
        ++m_nestedMethodCallCount;

        try
        {
            for (auto& observer : m_observers)
            {
                if (auto scoped = observer)
                {
                    resultArray.push_back(
                        scoped->operator()(std::forward<Arguments>(arguments)...)
                    );
                }
            }
        }
        catch (const std::exception& e)
        {
            --m_nestedMethodCallCount;
            throw e;
        }

        CIDER_ASSERT(m_nestedMethodCallCount > 0, "");
        --m_nestedMethodCallCount;

        if (m_nestedMethodCallCount <= 0)
        {
            EraseRemovedListeners();
        }

        return resultArray;
    }

    std::size_t InvocationCount() const
    {
        return m_observers.size();
    }

private:
    Void PushBackAddedListeners()
    {
        std::vector<STL::shared_ptr<SlotType>> temporarySlots;
        {
            std::lock_guard<std::recursive_mutex> lock(m_addingProtection);
            std::swap(temporarySlots, m_addedObservers);
        }
        {
            std::lock_guard<std::recursive_mutex> lock(m_slotsProtection);

            for (auto& slot : temporarySlots)
            {
                CIDER_ASSERT(
                    std::end(m_observers) ==
                    std::find(std::begin(m_observers), std::end(m_observers), slot),
                    ""
                );

                m_observers.push_back(slot);
            }
        }
    }

    Void EraseRemovedListeners()
    {
        std::lock_guard<std::recursive_mutex> lock(m_slotsProtection);

        m_observers.erase(
            std::remove_if(
                std::begin(m_observers),
                std::end(m_observers),
                [](const STL::shared_ptr<SlotType>& slot) -> Bool {
            return !slot;
        }
            ),
            std::end(m_observers)
            );
    }

private:
    std::vector<STL::shared_ptr<SlotType>> m_observers;
    std::vector<STL::shared_ptr<SlotType>> m_addedObservers;

    std::recursive_mutex m_addingProtection;
    std::recursive_mutex m_slotsProtection;

    std::int32_t m_nestedMethodCallCount = 0;

};


// 戻り値 Void 特殊化
template<typename... Arguments>
class SignalBody<Void(Arguments...)> final
    : public std::enable_shared_from_this< SignalBody<Void(Arguments...)>>
    , public BaseAllocator<MEMORY_AREA::SYSTEM>
{
private:
    typedef Slot<Void(Arguments...)> SlotType;
    typedef ConnectionBodyOverride<Void(Arguments...)> ConnectionBodyOverrideType;

public:
    SignalBody() = default;

    SignalBody(const SignalBody&) = delete;
    Void operator=(const SignalBody&) = delete;

    SignalBody(SignalBody&&) = delete;
    Void operator=(SignalBody&&) = delete;

    template<typename Function>
    STL::unique_ptr<ConnectionBodyOverrideType> Connect(Function&& slot)
    {
        CIDER_ASSERT(slot, "");

        auto observer = STL::make_shared<SlotType, MEMORY_AREA::SYSTEM>(std::forward<Function>(slot));
        {
            std::lock_guard< std::recursive_mutex> lock(m_addingProtection);

            CIDER_ASSERT(
                std::end(m_addedObservers) ==
                std::find(std::begin(m_addedObservers), std::end(m_addedObservers), observer),
                ""
            );

            m_addedObservers.push_back(observer);
        }

        STL::weak_ptr<SignalBody> weakSignal = this->shared_from_this();

        CIDER_ASSERT(!weakSignal.expired(), "");

        return STL::make_unique<ConnectionBodyOverrideType>(std::move(weakSignal), observer);
    }

    Void Disconnect(const SlotType* observer)
    {
        CIDER_ASSERT(observer, "");

        {
            std::lock_guard<std::recursive_mutex> lock(m_addingProtection);

            m_addedObservers.erase(
                std::remove_if(
                    std::begin(m_addedObservers),
                    std::end(m_addedObservers),
                    [observer](const STL::shared_ptr<SlotType>& slot) -> Bool {
                return slot.get() == observer;
            }
                ),
                std::end(m_addedObservers)
                );
        }

        auto const it = std::find_if(
            std::begin(m_observers),
            std::end(m_observers),
            [observer](const STL::shared_ptr<SlotType>& slot) -> Bool {
            return slot.get() == observer;
        }
        );

        if (it != std::end(m_observers))
        {
            it->reset();
        }
    }

    Void operator()(Arguments&&... arguments)
    {
        if (m_nestedMethodCallCount <= 0)
        {
            PushBackAddedListeners();
        }

        if (m_nestedMethodCallCount >= std::numeric_limits<std::int16_t>::max())
        {
            return;
        }

        CIDER_ASSERT(m_nestedMethodCallCount >= 0, "");
        ++m_nestedMethodCallCount;

        try
        {
            for (auto& observer : m_observers)
            {
                if (auto scoped = observer)
                {
                    scoped->operator()(std::forward<Arguments>(arguments)...);
                }
            }
        }
        catch (const std::exception& e)
        {
            --m_nestedMethodCallCount;
            throw e;
        }

        CIDER_ASSERT(m_nestedMethodCallCount > 0, "");
        --m_nestedMethodCallCount;

        if (m_nestedMethodCallCount <= 0)
        {
            EraseRemovedListeners();
        }
    }

    std::size_t InvocationCount() const
    {
        return m_observers.size();
    }

private:
    Void PushBackAddedListeners()
    {
        std::vector<STL::shared_ptr<SlotType>> temporarySlots;
        {
            std::lock_guard<std::recursive_mutex> lock(m_addingProtection);
            std::swap(temporarySlots, m_addedObservers);
        }
        {
            std::lock_guard<std::recursive_mutex> lock(m_slotsProtection);

            for (auto& slot : temporarySlots)
            {
                CIDER_ASSERT(
                    std::end(m_observers) ==
                    std::find(std::begin(m_observers), std::end(m_observers), slot),
                    ""
                );

                m_observers.push_back(slot);
            }
        }
    }

    Void EraseRemovedListeners()
    {
        std::lock_guard<std::recursive_mutex> lock(m_slotsProtection);

        m_observers.erase(
            std::remove_if(
                std::begin(m_observers),
                std::end(m_observers),
                [](const STL::shared_ptr<SlotType>& slot) -> Bool {
            return !slot;
        }
            ),
            std::end(m_observers)
            );
    }

private:
    std::vector<STL::shared_ptr<SlotType>> m_observers;
    std::vector<STL::shared_ptr<SlotType>> m_addedObservers;

    std::recursive_mutex m_addingProtection;
    std::recursive_mutex m_slotsProtection;

    std::int32_t m_nestedMethodCallCount = 0;

};


} // namespace Detail



class Connection final
{
    typedef Detail::ConnectionBody ConnectionBody;
public:
    Connection() = default;
    ~Connection() = default;

    explicit Connection(STL::unique_ptr<ConnectionBody>&& body)
        : m_body(std::forward<STL::unique_ptr<ConnectionBody>>(body))
    {}

    Connection(const Connection& connection)
    {
        if (connection.m_body)
        {
            m_body = connection.m_body->DeepCopy();
        }
    }

    Connection& operator=(const Connection& connection)
    {
        if (connection.m_body)
        {
            m_body = connection.m_body->DeepCopy();
        }

        return *this;
    }

    Connection(Connection&&) = default;
    Connection& operator=(Connection&&) = default;

    operator Bool() const
    {
        return m_body && m_body->Valid();
    }

    Void Disconnect()
    {
        if (m_body)
        {
            m_body->Disconnect();
            m_body.reset();
        }
    }

private:
    STL::unique_ptr<ConnectionBody> m_body;
};


class ScopedConnection final
{
public:
    ScopedConnection() = default;

    ScopedConnection(const ScopedConnection&) = delete;
    Void operator=(const ScopedConnection&) = delete;

    ScopedConnection(ScopedConnection&&) = default;
    ScopedConnection& operator=(ScopedConnection&&) = default;

    ~ScopedConnection()
    {
        m_connection.Disconnect();
    }

    ScopedConnection& operator=(const Connection& connection)
    {
        m_connection.Disconnect();
        m_connection = connection;
        return *this;
    }

    ScopedConnection& operator=(Connection&& connection)
    {
        m_connection.Disconnect();
        m_connection = std::move(connection);
        return *this;
    }

    Void Disconnect()
    {
        m_connection.Disconnect();
    }

private:
    Connection m_connection;
};


template<typename Result, typename... Arguments>
class Signal<Result(Arguments...)> final
{
private:
    typedef Detail::SignalBody<Result(Arguments...)> SignalBodyType;

public:
    typedef std::vector<Result> ResultArray;

public:
    Signal()
        : m_body(STL::make_shared<SignalBodyType>())
    {}

    Signal(const Signal&) = delete;
    Void operator=(const Signal&) = delete;

    Signal(Signal&&) = default;
    Signal& operator=(Signal&&) = default;

    Connection Connect(const Slot<Result(Arguments...)>& slot)
    {
        CIDER_ASSERT(slot, "");
        CIDER_ASSERT(m_body, "");
        return Connection { m_body->Connect(slot) };
    }

    Connection Connect(Slot<Result(Arguments...)>&& slot)
    {
        CIDER_ASSERT(slot, "");
        CIDER_ASSERT(m_body, "");
        return Connection { m_body->Connect(std::move(slot)) };
    }

    ResultArray operator()(Arguments&&...arguments)
    {
        CIDER_ASSERT(m_body, "");
        return m_body->operator()(std::forward<Arguments>(arguments)...);
    }

    std::size_t InvocationCount() const
    {
        CIDER_ASSERT(m_body, "");
        return m_body->InvocationCount();
    }

private:
    STL::shared_ptr<SignalBodyType> m_body;
};


// 戻り値 Void 特殊化
template<typename... Arguments>
class Signal<Void(Arguments...)> final
{
private:
    typedef Detail::SignalBody<Void(Arguments...)> SignalBodyType;

public:
    Signal()
        : m_body(STL::make_shared<SignalBodyType>())
    {}

    Signal(const Signal&) = delete;
    Void operator=(const Signal&) = delete;

    Signal(Signal&&) = default;
    Signal& operator=(Signal&&) = default;

    Connection Connect(const Slot<Void(Arguments...)>& slot)
    {
        CIDER_ASSERT(slot, "");
        CIDER_ASSERT(m_body, "");
        return Connection { m_body->Connect(slot) };
    }

    Connection Connect(Slot<Void(Arguments...)>&& slot)
    {
        CIDER_ASSERT(slot, "");
        CIDER_ASSERT(m_body, "");
        return Connection { m_body->Connect(std::move(slot)) };
    }

    Void operator()(Arguments&&...arguments)
    {
        CIDER_ASSERT(m_body, "");
        m_body->operator()(std::forward<Arguments>(arguments)...);
    }

    std::size_t InvocationCount() const
    {
        CIDER_ASSERT(m_body, "");
        return m_body->InvocationCount();
    }

private:
    STL::shared_ptr<SignalBodyType> m_body;
};


} // namespace System
} // namespace Cider

