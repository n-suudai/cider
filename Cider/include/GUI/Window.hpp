
#pragma once

#include "System/Types.hpp"
#include "System/KeyCode.hpp"


namespace Cider {
namespace GUI {
namespace Events {


struct OnWindowWillClose
{
    Void* dummy;
};


struct OnResizeWindow
{
    UInt32 width;
    UInt32 height;
};


struct OnKey
{
    System::KEY_CODE keyCode;
    Bool             isDown;
};


struct OnMouseButton
{
    UInt32               x;
    UInt32               y;
    System::MOUSE_BUTTON button;
    Bool                 isDown;
};


struct OnMouseWheel
{
    UInt32 x;
    UInt32 y;
    Int32  wheelDelta;
};


struct OnIdle
{
    Double deltaTime;
};


struct OnDestroy
{
    Void* dummy;
};


} // namespace Events
} // namespace GUI
} // namespace Cider

