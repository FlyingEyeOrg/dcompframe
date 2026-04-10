#include "dcompframe/layout/stack_panel.h"

#include <algorithm>

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
    const Rect current_bounds = bounds();
    set_bounds(Rect {.x = current_bounds.x, .y = current_bounds.y, .width = available_size.width, .height = available_size.height});

    float cursor_x = 0.0F;
    float cursor_y = 0.0F;
    float line_max_extent = 0.0F;

    for (const auto& child : children()) {
        const Size desired = child->desired_size();
        const Thickness margin = child->margin();

        if (orientation_ == Orientation::Vertical) {
            const float child_height = desired.height > 0.0F ? desired.height : 0.0F;
            const float child_width = std::max(0.0F, available_size.width - margin.left - margin.right);
            child->set_bounds(Rect {
                .x = margin.left,
                .y = cursor_y + margin.top,
                .width = child_width,
                .height = child_height,
            });
            child->arrange(Size {.width = child_width, .height = child_height});
            cursor_y += margin.top + child_height + margin.bottom + spacing_;
            continue;
        }

        const float child_width = desired.width > 0.0F ? desired.width : 0.0F;
        const float child_height = desired.height > 0.0F
            ? desired.height
            : std::max(0.0F, available_size.height - margin.top - margin.bottom);
        const float total_width = margin.left + child_width + margin.right;
        const float total_height = margin.top + child_height + margin.bottom;

        if (wrap_enabled_ && cursor_x > 0.0F && (cursor_x + total_width) > available_size.width) {
            cursor_x = 0.0F;
            cursor_y += line_max_extent + spacing_;
            line_max_extent = 0.0F;
        }

        child->set_bounds(Rect {
            .x = cursor_x + margin.left,
            .y = cursor_y + margin.top,
            .width = child_width,
            .height = child_height,
        });
        child->arrange(Size {.width = child_width, .height = child_height});
        cursor_x += total_width + spacing_;
        if (total_height > line_max_extent) {
            line_max_extent = total_height;
        }
    }
}

}  // namespace dcompframe
