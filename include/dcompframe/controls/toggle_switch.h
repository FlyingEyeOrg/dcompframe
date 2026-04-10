#pragma once

#include "dcompframe/controls/styled_element.h"

#include <functional>

namespace dcompframe {

class ToggleSwitch : public StyledElement {
public:
    using CheckedChangedHandler = std::function<void(bool)>;

    ToggleSwitch();

    void set_checked(bool checked);
    [[nodiscard]] bool checked() const;
    bool toggle();
    void set_on_checked_changed(CheckedChangedHandler handler);

private:
    bool checked_ = false;
    CheckedChangedHandler on_checked_changed_ {};
};

}  // namespace dcompframe