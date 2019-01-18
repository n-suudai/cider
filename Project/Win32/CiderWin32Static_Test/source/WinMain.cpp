
#include "Cider.hpp"
#pragma comment(lib, "CiderWin32Static.lib")


#include <tchar.h>
#include <Windows.h>



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

protected:
    STL::weak_ptr<Entity> m_ownerEntity;
};


class TestComponentA : public Component
{
public:
    TestComponentA() = default;
    ~TestComponentA() = default;

    virtual Void HandleEvent(const System::SystemEvent& eventObject) override
    {
        if (eventObject.Is<OnStart>())
        {
            System::Log::Message(
                System::Log::Verbose,
                "TestComponentA => OnStart"
            );
        }
        else if (eventObject.Is<OnDestroy>())
        {
            System::Log::Message(
                System::Log::Verbose,
                "TestComponentA => OnDestroy"
            );
        }
        else if (auto onUpdate = eventObject.As<OnUpdate>())
        {
            System::Log::Format(
                System::Log::Verbose,
                "TestComponentA => OnUpdate{ deltaTime=%lf }",
                onUpdate->deltaTime
            );
        }
    }
};


class ComponentManager
{
public:
    static ComponentManager* Instance()
    {
        static ComponentManager instance;
        return &instance;
    }

public:
    ComponentManager() = default;

    ~ComponentManager() = default;

    STL::shared_ptr<Component> CreateComponent()
    {
        auto component = STL::make_shared<TestComponentA>();

        m_components.push_back(component);

        return component;
    }

    Void DestroyComponent(const STL::shared_ptr<Component>& destroyComponent)
    {
        CIDER_ASSERT(destroyComponent, "");

        m_components.erase(
            std::find_if(
                std::begin(m_components),
                std::end(m_components),
                [&destroyComponent](const STL::shared_ptr<Component>& component)->Bool {
                    return component.get() == destroyComponent.get();
                }
            ),
            std::end(m_components)
        );
    }

private:
    std::vector<STL::shared_ptr<Component>> m_components;
};


class Entity : public System::BaseAllocator<System::MEMORY_AREA::SYSTEM>
{
public:
    Entity()
    {
        m_eventConnection = m_eventQueue.Connect(
            [this](const System::SystemEvent& eventObject) {
            for (auto& component : m_componentList)
            {
                CIDER_ASSERT(component, "");
                component->HandleEvent(eventObject);
            }
        }
        );
    }

    ~Entity() = default;

    template<typename T>
    Void PostEvent(T&& eventData)
    {
        m_eventQueue.Enqueue<T>(std::forward<T>(eventData));
    }

    Void DispatchEvent()
    {
        m_eventQueue.Emit();
    }

    Void RegisterComponent(const Char*)
    {
        m_componentList.push_back(
            ComponentManager::Instance()->CreateComponent()
        );
    }

    Void UnregisterComponent(const Char*)
    {
        if (!m_componentList.empty())
        {
            auto destroyComponent = m_componentList.front();
            
            m_componentList.pop_front();

            ComponentManager::Instance()->DestroyComponent(destroyComponent);
        }
    }

private:
    System::ScopedConnection                m_eventConnection;
    System::SystemEventQueue         m_eventQueue;
    STL::list<STL::shared_ptr<Component>>   m_componentList;
};


class EntityManager
{
public:
    static EntityManager* Instance()
    {
        static EntityManager instance;
        return &instance;
    }

public:
    EntityManager()
        : m_nextEntityId(1)
    {}

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

    Void DispatchEvent()
    {
        for (auto& entityPair : m_entityTable)
        {
            CIDER_ASSERT(entityPair.second, "");
            entityPair.second->DispatchEvent();
        }

        ApplyDestroyEntityIds();
    }

    UInt64 CreateEntity()
    {
        UInt64 entityId = m_nextEntityId;
        m_nextEntityId++;

        auto entity = STL::make_shared<Entity>();

        m_entityTable.insert(
            EntityTableType::value_type(
                entityId,
                entity
            )
        );

        entity->PostEvent(OnStart {});

        return entityId;
    }

    void DestroyEntity(UInt64 entityId)
    {
        auto entityIt = m_entityTable.find(entityId);

        if (entityIt != std::end(m_entityTable))
        {
            CIDER_ASSERT((*entityIt).second, "");
            (*entityIt).second->PostEvent(OnDestroy {});

            m_destroyEntityIds.push_back((*entityIt).first);
        }
    }

    Void RegisterComponent(UInt64 entityId, const Char* componentName)
    {
        auto entityIt = m_entityTable.find(entityId);

        if (entityIt != std::end(m_entityTable))
        {
            CIDER_ASSERT((*entityIt).second, "");
            (*entityIt).second->RegisterComponent(componentName);
        }
    }

    Void UnregisterComponent(UInt64 entityId, const Char* componentName)
    {
        auto entityIt = m_entityTable.find(entityId);

        if (entityIt != std::end(m_entityTable))
        {
            CIDER_ASSERT((*entityIt).second, "");
            (*entityIt).second->UnregisterComponent(componentName);
        }
    }

private:
    void ApplyDestroyEntityIds()
    {
        for (auto destroyEntityId : m_destroyEntityIds)
        {
            m_entityTable.erase(destroyEntityId);
        }

        m_destroyEntityIds.clear();
    }

private:
    typedef STL::map<UInt64, STL::shared_ptr<Entity>> EntityTableType;

    UInt64 m_nextEntityId;
    EntityTableType m_entityTable;
    STL::vector<UInt64> m_destroyEntityIds;
};


} // namespace GameSystem
} // namespace Cider




int APIENTRY _tWinMain(
    _In_        HINSTANCE hInstance,
    _In_opt_    HINSTANCE hPrevInstance,
    _In_        LPWSTR    lpCmdLine,
    _In_        int       nCmdShow
)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    //Cider::System::Signal<void()> signal;

    //signal.Connect(Cider::Hello);

    //signal();

    auto entityManager = Cider::GameSystem::EntityManager::Instance();

    // エンティティの生成
    auto entityId = entityManager->CreateEntity();

    // コンポーネントの登録
    entityManager->RegisterComponent(entityId, "");

    // 更新イベントの発行
    entityManager->BroadcastEvent(Cider::GameSystem::OnUpdate { 0.0 });

    // コンポーネントの削除
    entityManager->DestroyEntity(entityId);

    // イベントを実行
    entityManager->DispatchEvent();

    CIDER_ASSERT(false, "強制失敗！！");

    return EXIT_SUCCESS;
}

