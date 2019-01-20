
#pragma once


#include "System.hpp"
#include "GameSystem.hpp"


namespace Cider {


struct SystemObject : public System::BaseAllocator<System::MEMORY_AREA::SYSTEM, 16>
{

};

class Entity : public System::BaseAllocator<System::MEMORY_AREA::APPLICATION>
{
public:
    Entity()
    {
        System::Log::Format(
            System::Log::Debug,
            "Hello!! %s!!",
            "Cider"
        );
    }

    ~Entity()
    {
        System::Log::Format(
            System::Log::Debug,
            "Good Bye!! %s!!",
            "Cider"
        );
    }
};

Void CIDER_APIENTRY Hello();


} // namespace Cider

