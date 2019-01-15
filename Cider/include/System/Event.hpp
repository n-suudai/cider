
#pragma once

#include "System/Types.hpp"
#include "System/KeyCode.hpp"
#include "System/STL.hpp"
#include <functional>


namespace Cider {
namespace System {
namespace Event {


struct OnResizeWindow
{
    UInt32 width;
    UInt32 height;
};
typedef std::function<void(OnResizeWindow&)> OnResizeWindowHandler;


struct OnKey
{
    KEY_CODE keyCode;
    Bool     isDown;
};
typedef std::function<void(OnKey&)> OnKeyHandler;


struct OnMouseButton
{
    UInt32       x;
    UInt32       y;
    MOUSE_BUTTON button;
    Bool         isDown;
};
typedef std::function<void(OnMouseButton&)> OnMouseButtonHandler;


struct OnMouseWheel
{
    UInt32 x;
    UInt32 y;
    Int32  wheelDelta;
};
typedef std::function<void(OnMouseWheel&)> OnMouseWheelHandler;


struct OnIdle
{
    Double deltaTime;
};
typedef std::function<void(OnIdle&)> OnIdleHandler;


struct OnDestroy
{
    Void* dummy;
};
typedef std::function<void(OnDestroy&)> OnDestroyHandler;


} // namespace Event
} // namespace System
} // namespace Cider

