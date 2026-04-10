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
    void set_main_axis_alignment(LayoutAxisAlignment alignment);
    void set_cross_axis_alignment(LayoutAxisAlignment alignment);
    Size measure(const Size& available_size) override;
    void arrange(const Size& available_size) override;

private:
    Orientation orientation_ = Orientation::Vertical;
    float spacing_ = 0.0F;
    bool wrap_enabled_ = false;
    LayoutAxisAlignment main_axis_alignment_ = LayoutAxisAlignment::Start;
    LayoutAxisAlignment cross_axis_alignment_ = LayoutAxisAlignment::Stretch;
};

}  // namespace dcompframe
