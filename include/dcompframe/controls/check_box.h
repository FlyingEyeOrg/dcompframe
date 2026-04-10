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


class CheckBox : public StyledElement {
public:
    using CheckedChangedHandler = std::function<void(bool)>;

    CheckBox();

    void set_checked(bool checked);
    [[nodiscard]] bool checked() const;
    bool toggle();
    void set_on_checked_changed(CheckedChangedHandler handler);

private:
    bool checked_ = false;
    CheckedChangedHandler on_checked_changed_ {};
};

}  // namespace dcompframe
