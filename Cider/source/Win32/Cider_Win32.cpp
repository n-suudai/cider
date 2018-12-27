#include "Cider.hpp"
#include "Win32Prerequisites.hpp"


namespace Cider {


void CIDER_APIENTRY Hello()
{
    OutputDebugStringA(
        "-------------------------------------\n"
        "            Hello, Cider!!\n"
        "-------------------------------------\n"
    );
}


} // namespace Cider

