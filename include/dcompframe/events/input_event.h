#pragma once

namespace dcompframe {

enum class EventType {
    MouseDown,
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
    bool handled = false;
};

}  // namespace dcompframe
