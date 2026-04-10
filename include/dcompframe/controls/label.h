#pragma once

#include "dcompframe/controls/styled_element.h"

#include <string>

namespace dcompframe {

class Label : public StyledElement {
public:
    explicit Label(std::string text = "");

    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const;

private:
    std::string text_;
};

}  // namespace dcompframe
