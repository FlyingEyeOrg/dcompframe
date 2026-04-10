#pragma once

#include "dcompframe/controls/styled_element.h"

namespace dcompframe {

enum class DividerOrientation {
    Horizontal,
    Vertical,
};

class Divider : public StyledElement {
public:
    Divider();

    void set_orientation(DividerOrientation orientation);
    [[nodiscard]] DividerOrientation orientation() const;

private:
    DividerOrientation orientation_ = DividerOrientation::Horizontal;
};

}  // namespace dcompframe