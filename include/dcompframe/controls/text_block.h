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


class TextBlock : public StyledElement {
public:
    explicit TextBlock(std::string text = "");

    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const;

private:
    std::string text_;
};

}  // namespace dcompframe
