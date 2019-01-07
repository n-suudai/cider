#include "Cider.hpp"
#include "Win32Prerequisites.hpp"


namespace Cider {


Void CIDER_APIENTRY Hello()
{
    Log::Message(
        Log::Level::Verbose,
        "-------------------------------------\n"
        "            Hello, Cider!!\n"
        "-------------------------------------\n"
    );
}


} // namespace Cider

