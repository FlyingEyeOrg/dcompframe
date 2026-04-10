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


class Panel : public StyledElement {
public:
    Panel();

    Size measure(const Size& available_size) override;
    void arrange(const Size& available_size) override;
};

}  // namespace dcompframe
