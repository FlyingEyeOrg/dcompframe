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


class Image : public StyledElement {
public:
    Image();

    void set_source(std::string source);
    [[nodiscard]] const std::string& source() const;

private:
    std::string source_;
};

}  // namespace dcompframe
