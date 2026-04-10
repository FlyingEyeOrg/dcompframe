#include "dcompframe/layout/stack_panel.h"

namespace dcompframe {

StackPanel::StackPanel(Orientation orientation)
    : UIElement("stack_panel"), orientation_(orientation) {}

void StackPanel::set_spacing(float spacing) {
    spacing_ = spacing;
}

void StackPanel::set_wrap_enabled(bool enabled) {
    wrap_enabled_ = enabled;
}

void StackPanel::arrange(const Size& available_size) {
    set_bounds(Rect {.x = 0.0F, .y = 0.0F, .width = available_size.width, .height = available_size.height});

    float cursor_x = 0.0F;
    float cursor_y = 0.0F;
    float line_max_extent = 0.0F;

    for (const auto& child : children()) {
        const Size desired = child->desired_size();

        if (orientation_ == Orientation::Vertical) {
            const float child_height = desired.height > 0.0F ? desired.height : 0.0F;
            child->set_bounds(Rect {.x = 0.0F, .y = cursor_y, .width = available_size.width, .height = child_height});
            cursor_y += child_height + spacing_;
            continue;
        }

        if (wrap_enabled_ && cursor_x > 0.0F && (cursor_x + desired.width) > available_size.width) {
            cursor_x = 0.0F;
            cursor_y += line_max_extent + spacing_;
            line_max_extent = 0.0F;
        }

        const float child_width = desired.width > 0.0F ? desired.width : 0.0F;
        child->set_bounds(Rect {.x = cursor_x, .y = cursor_y, .width = child_width, .height = available_size.height});
        cursor_x += child_width + spacing_;
        if (available_size.height > line_max_extent) {
            line_max_extent = available_size.height;
        }
    }
}

}  // namespace dcompframe
