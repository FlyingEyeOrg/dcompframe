#pragma once

#include "dcompframe/layout/flex_panel.h"

namespace dcompframe {

enum class Orientation {
    Horizontal,
    Vertical
};

class StackPanel : public FlexPanel {
public:
    explicit StackPanel(Orientation orientation = Orientation::Vertical);

    void set_spacing(float spacing);
    void set_wrap_enabled(bool enabled);
    void set_main_axis_alignment(LayoutAxisAlignment alignment);
    void set_cross_axis_alignment(LayoutAxisAlignment alignment);
};

}  // namespace dcompframe
