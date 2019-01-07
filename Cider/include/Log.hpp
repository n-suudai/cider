
#pragma once

#include "Api.hpp"
#include "Types.hpp"


namespace Cider {
namespace Log {


enum class Level
{
    Verbose
    , Debug
    , Info
    , Warning
    , Error
    , Assert
    , Num
};


Void CIDER_APIENTRY Format(Level level, const Char* format, ...);
Void CIDER_APIENTRY Message(Level level, const Char* message);


} // namespace Log
} // namespace Cider

