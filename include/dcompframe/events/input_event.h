#pragma once

#include "dcompframe/geometry.h"

namespace dcompframe {

class UIElement;

enum class EventType {
    MouseDown,
    MouseMove,
    MouseUp,
    KeyDown,
    FocusChanged
};

enum class EventPhase {
    Capture,
    Target,
    Bubble
};

struct InputEvent {
    EventType type = EventType::MouseDown;
    Point position {};
    UIElement* target = nullptr;
    UIElement* current_target = nullptr;
    EventPhase phase = EventPhase::Target;
    bool handled = false;
};

}  // namespace dcompframe
