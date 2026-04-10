#pragma once
#include <optional>
#include <cstddef>
#include <chrono>

#include "dcompframe/ui_element.h"
#include "dcompframe/controls/controls_common.h"
#include "dcompframe/controls/styled_element.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "dcompframe/binding/observable.h"

namespace dcompframe {


class ScrollViewer : public StyledElement {
public:
    ScrollViewer();

    void set_scroll_offset(float x, float y);
    [[nodiscard]] Point scroll_offset() const;
    void set_inertia_velocity(float x, float y);
    void tick_inertia(std::chrono::milliseconds delta_time, float deceleration = 0.0015F);
    [[nodiscard]] Point inertia_velocity() const;

private:
    Point offset_ {};
    Point velocity_ {};
};

}  // namespace dcompframe
