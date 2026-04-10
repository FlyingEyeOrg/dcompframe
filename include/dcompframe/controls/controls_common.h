#pragma once
#include <optional>
#include <cstddef>
#include <chrono>

#include "dcompframe/ui_element.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "dcompframe/binding/observable.h"

namespace dcompframe {


enum class ControlState {
    Normal,
    Hovered,
    Pressed,
    Disabled,
    Selected
};

enum class TextHorizontalAlignment {
    Left,
    Center,
    Right
};

enum class TextVerticalAlignment {
    Top,
    Center,
    Bottom
};

}  // namespace dcompframe
