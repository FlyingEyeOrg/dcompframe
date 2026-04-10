#pragma once
#include <optional>
#include <cstddef>
#include <chrono>

#include "dcompframe/ui_element.h"
#include "dcompframe/controls/controls_common.h"
#include "dcompframe/controls/style.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "dcompframe/binding/observable.h"

namespace dcompframe {


class StyledElement : public UIElement {
public:
    explicit StyledElement(std::string name);

    void set_style(Style style);
    [[nodiscard]] Style style() const;

    void set_state(ControlState state);
    [[nodiscard]] ControlState state() const;

    void set_text_alignment(TextHorizontalAlignment horizontal, TextVerticalAlignment vertical);
    [[nodiscard]] TextHorizontalAlignment text_horizontal_alignment() const;
    [[nodiscard]] TextVerticalAlignment text_vertical_alignment() const;

private:
    Style style_ {};
    ControlState state_ = ControlState::Normal;
    TextHorizontalAlignment text_horizontal_alignment_ = TextHorizontalAlignment::Center;
    TextVerticalAlignment text_vertical_alignment_ = TextVerticalAlignment::Center;
};

}  // namespace dcompframe
