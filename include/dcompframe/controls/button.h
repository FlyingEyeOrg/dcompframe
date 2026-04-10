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


class Button : public StyledElement {
public:
    explicit Button(std::string text = "");

    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const;

    void set_on_click(std::function<void()> callback);
    bool click();
    void bind_enabled(Observable<bool>& observable);

private:
    std::string text_;
    std::function<void()> on_click_ {};
    int enabled_binding_id_ = 0;
};

}  // namespace dcompframe
