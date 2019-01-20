
#include "GameSystem.hpp"
#include "System/Log.hpp"


namespace Cider {
namespace GameSystem {



ComponentManager* ComponentManager::Instance()
{
    static ComponentManager instance;
    return &instance;
}

STL::shared_ptr<Component> ComponentManager::CreateComponent(const Char* componentName)
{
    auto component = CreateUserComponent(componentName);

    CIDER_ASSERT(component, "");
    CIDER_ASSERT(strcmp(componentName, component->GetComponentName()) == 0, "");

    auto component_table_it = m_componentTable.find(componentName);

    if (component_table_it != std::end(m_componentTable))
    {
        component_table_it->second.push_back(
            component
        );
    }
    else
    {
        m_componentTable.insert(
            ComponentTable::value_type(
                componentName, ComponentArrayType{ component }
            )
        );
    }

    return component;
}

Void ComponentManager::DestroyComponent(const STL::shared_ptr<Component>& destroyComponent)
{
    CIDER_ASSERT(destroyComponent, "");

    auto component_table_it = m_componentTable.find(destroyComponent->GetComponentName());

    CIDER_ASSERT(component_table_it != std::end(m_componentTable), "");

    auto& components = component_table_it->second;

    components.erase(
        std::find_if(
            std::begin(components),
            std::end(components),
            [&destroyComponent](const STL::shared_ptr<Component>& component)->Bool {
        return component.get() == destroyComponent.get();
    }
        ),
        std::end(components)
        );
}



Entity::Entity()
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

Void Entity::DispatchEvent()
{
    m_eventQueue.Emit();
}

Void Entity::RegisterComponent(const Char* componentName)
{
    m_componentList.push_back(
        ComponentManager::Instance()->CreateComponent(componentName)
    );
}

Void Entity::UnregisterComponent(const Char* componentName)
{
    if (!m_componentList.empty())
    {
        auto component_list_it = std::find_if(
            std::begin(m_componentList),
            std::end(m_componentList),
            [componentName](const STL::shared_ptr<Component>& component) -> Bool {
            return STL::string(componentName) == component->GetComponentName();
        }
        );

        if (component_list_it != std::end(m_componentList))
        {
            auto destroyComponent = *component_list_it;

            m_componentList.erase(component_list_it);

            ComponentManager::Instance()->DestroyComponent(destroyComponent);
        }
    }
}



EntityManager* EntityManager::Instance()
{
    static EntityManager instance;
    return &instance;
}

EntityManager::EntityManager()
    : m_nextEntityId(1)
{}

Void EntityManager::DispatchEvent()
{
    for (auto& entityPair : m_entityTable)
    {
        CIDER_ASSERT(entityPair.second, "");
        entityPair.second->DispatchEvent();
    }

    ApplyDestroyEntityIds();
}

UInt64 EntityManager::CreateEntity()
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

    entity->PostEvent(OnStart{});

    return entityId;
}

void EntityManager::DestroyEntity(UInt64 entityId)
{
    auto entityIt = m_entityTable.find(entityId);

    if (entityIt != std::end(m_entityTable))
    {
        CIDER_ASSERT((*entityIt).second, "");
        (*entityIt).second->PostEvent(OnDestroy{});

        m_destroyEntityIds.push_back((*entityIt).first);
    }
}

Void EntityManager::RegisterComponent(UInt64 entityId, const Char* componentName)
{
    auto entityIt = m_entityTable.find(entityId);

    if (entityIt != std::end(m_entityTable))
    {
        CIDER_ASSERT((*entityIt).second, "");
        (*entityIt).second->RegisterComponent(componentName);
    }
}

Void EntityManager::UnregisterComponent(UInt64 entityId, const Char* componentName)
{
    auto entityIt = m_entityTable.find(entityId);

    if (entityIt != std::end(m_entityTable))
    {
        CIDER_ASSERT((*entityIt).second, "");
        (*entityIt).second->UnregisterComponent(componentName);
    }
}

void EntityManager::ApplyDestroyEntityIds()
{
    for (auto destroyEntityId : m_destroyEntityIds)
    {
        m_entityTable.erase(destroyEntityId);
    }

    m_destroyEntityIds.clear();
}


} // namespace GameSystem
} // namespace Cider

