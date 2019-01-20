#include <tchar.h>
#include "Cider.hpp"
#pragma comment(lib, "CiderWin32Static.lib")

#include <string>



namespace Cider {
namespace GameSystem {


class TestComponentA : public GameSystem::Component
{
public:
    TestComponentA() = default;

    ~TestComponentA() = default;

    virtual Void HandleEvent(const System::SystemEvent& eventObject) override
    {
        using namespace GameSystem;

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

    virtual const Char* GetComponentName() const override
    {
        return "TestComponentA";
    }
};


STL::shared_ptr<Component> CreateUserComponent(const Char* componentName)
{
    typedef STL::map<const Char*, std::function<STL::shared_ptr<Component>()>> CreateComponentFunctionTableType;

    static CreateComponentFunctionTableType createComponentFunctionTable = {
        { "TestComponentA", []() { return STL::make_shared<TestComponentA>(); } }
    };

    auto table_it = createComponentFunctionTable.find(componentName);

    if (table_it != std::end(createComponentFunctionTable))
    {
        return table_it->second();
    }

    return nullptr;
}


} // namespace GameSystem
} // namespace Cider

