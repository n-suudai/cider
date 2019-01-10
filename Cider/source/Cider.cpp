#include "Cider.hpp"


namespace Cider {


Void CIDER_APIENTRY Hello()
{
    Entity* pEntity = CIDER_NEW Entity();

    System::MemoryManager::ReportLeaks(0);

    CIDER_DELETE pEntity;
}


} // namespace Cider

