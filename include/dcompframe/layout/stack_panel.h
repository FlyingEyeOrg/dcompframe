#pragma once

#include "dcompframe/ui_element.h"

namespace dcompframe {

enum class Orientation {
    Horizontal,
    Vertical
};

class StackPanel : public UIElement {
public:
    explicit StackPanel(Orientation orientation = Orientation::Vertical);

    void set_spacing(float spacing);
    void set_wrap_enabled(bool enabled);
    void arrange(const Size& available_size);

private:
    Orientation orientation_ = Orientation::Vertical;
    float spacing_ = 0.0F;
    bool wrap_enabled_ = false;
};

}  // namespace dcompframe
