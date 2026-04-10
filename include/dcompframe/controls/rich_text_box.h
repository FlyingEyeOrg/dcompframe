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


class RichTextBox : public StyledElement {
public:
    RichTextBox();

    void set_rich_text(std::string rich_text);
    [[nodiscard]] const std::string& rich_text() const;

private:
    std::string rich_text_;
};

}  // namespace dcompframe
