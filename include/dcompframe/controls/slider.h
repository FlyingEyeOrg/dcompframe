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


class Slider : public StyledElement {
public:
    using ValueChangedHandler = std::function<void(float)>;

    Slider();

    void set_range(float min_value, float max_value);
    void set_value(float value);
    [[nodiscard]] float value() const;
    [[nodiscard]] float min_value() const;
    [[nodiscard]] float max_value() const;
    void set_step(float step);
    [[nodiscard]] float step() const;
    [[nodiscard]] float normalized_value() const;
    void set_value_from_ratio(float ratio);
    bool step_by(float delta_steps);
    void set_on_value_changed(ValueChangedHandler handler);

private:
    float min_ = 0.0F;
    float max_ = 100.0F;
    float value_ = 0.0F;
    float step_ = 1.0F;
    ValueChangedHandler on_value_changed_ {};
};

}  // namespace dcompframe
