#pragma once

#include "System/Event.hpp"


namespace Cider {
namespace GameSystem {


struct OnStart {};
struct OnDestroy {};
struct OnUpdate
{
    Double deltaTime;
};


class Entity;
class Component : public System::BaseAllocator<System::MEMORY_AREA::SYSTEM>
{
public:
    Component() = default;

    virtual ~Component() = default;

    template<typename T>
    Void PostEvent(UInt64 entityId, T&& eventData)
    {
        EntityManager::Instance()->PostEvent(entityId, eventData);
    }

    template<typename T>
    Void BroadcastEvent(T&& eventData)
    {
        EntityManager::Instance()->BroadcastEvent(eventData);
    }

    virtual Void HandleEvent(const System::SystemEvent&) {}

    virtual const Char* GetComponentName() const { return "Component"; }

protected:
    STL::weak_ptr<Entity> m_ownerEntity;
};


// for user
STL::shared_ptr<Component> CreateUserComponent(const Char* componentName);


class ComponentManager
{
public:
    static ComponentManager* Instance();

public:
    ComponentManager() = default;

    ~ComponentManager() = default;

    STL::shared_ptr<Component> CreateComponent(const Char* componentName);

    Void DestroyComponent(const STL::shared_ptr<Component>& destroyComponent);

private:
    typedef std::vector<STL::shared_ptr<Component>> ComponentArrayType;
    typedef std::map<const Char*, ComponentArrayType> ComponentTable;

    ComponentTable m_componentTable;
};


class Entity : public System::BaseAllocator<System::MEMORY_AREA::SYSTEM>
{
public:
    Entity();

    ~Entity() = default;

    template<typename T>
    Void PostEvent(T&& eventData)
    {
        m_eventQueue.Enqueue<T>(std::forward<T>(eventData));
    }

    Void DispatchEvent();

    Void RegisterComponent(const Char* componentName);

    Void UnregisterComponent(const Char* componentName);

private:
    System::ScopedConnection                m_eventConnection;
    System::SystemEventQueue         m_eventQueue;
    STL::list<STL::shared_ptr<Component>>   m_componentList;
};


class EntityManager
{
public:
    static EntityManager* Instance();

public:
    EntityManager();

    ~EntityManager() = default;

    template<typename T>
    Void PostEvent(UInt64 entityId, T&& eventData)
    {
        auto entityIt = m_entityTable.find(entityId);

        if (entityIt != std::end(m_entityTable))
        {
            CIDER_ASSERT((*entityIt).second, "");
            (*entityIt).second->PostEvent(eventData);
        }
    }

    template<typename T>
    Void BroadcastEvent(T&& eventData)
    {
        for (auto& entityPair : m_entityTable)
        {
            CIDER_ASSERT(entityPair.second, "");
            entityPair.second->PostEvent(eventData);
        }
    }

    Void DispatchEvent();

    UInt64 CreateEntity();

    void DestroyEntity(UInt64 entityId);

    Void RegisterComponent(UInt64 entityId, const Char* componentName);

    Void UnregisterComponent(UInt64 entityId, const Char* componentName);

private:
    void ApplyDestroyEntityIds();

private:
    typedef STL::map<UInt64, STL::shared_ptr<Entity>> EntityTableType;

    UInt64 m_nextEntityId;
    EntityTableType m_entityTable;
    STL::vector<UInt64> m_destroyEntityIds;
};


} // namespace GameSystem
} // namespace Cider

