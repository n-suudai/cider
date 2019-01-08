#include "Cider.hpp"


namespace Cider {


Void CIDER_APIENTRY Hello()
{
    Entity* pEntity = CIDER_NEW Entity();

    Cider::System::MemoryManager::ReportLeaks(0);

    CIDER_DELETE pEntity;

    Cider::System::MemoryManager::ReportLeaks(0);
}


} // namespace Cider

