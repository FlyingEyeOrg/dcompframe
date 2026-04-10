#include "dcompframe/layout/stack_panel.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

float main_extent(dcompframe::Orientation orientation, const dcompframe::Size& size) {
    return orientation == dcompframe::Orientation::Vertical ? size.height : size.width;
}

float cross_extent(dcompframe::Orientation orientation, const dcompframe::Size& size) {
    return orientation == dcompframe::Orientation::Vertical ? size.width : size.height;
}

dcompframe::Size make_size(dcompframe::Orientation orientation, float main, float cross) {
    if (orientation == dcompframe::Orientation::Vertical) {
        return dcompframe::Size {.width = cross, .height = main};
    }

    return dcompframe::Size {.width = main, .height = cross};
}

float leading_margin(dcompframe::Orientation orientation, const dcompframe::Thickness& margin) {
    return orientation == dcompframe::Orientation::Vertical ? margin.top : margin.left;
}

float trailing_margin(dcompframe::Orientation orientation, const dcompframe::Thickness& margin) {
    return orientation == dcompframe::Orientation::Vertical ? margin.bottom : margin.right;
}

float cross_leading_margin(dcompframe::Orientation orientation, const dcompframe::Thickness& margin) {
    return orientation == dcompframe::Orientation::Vertical ? margin.left : margin.top;
}

float cross_trailing_margin(dcompframe::Orientation orientation, const dcompframe::Thickness& margin) {
    return orientation == dcompframe::Orientation::Vertical ? margin.right : margin.bottom;
}

}  // namespace

namespace dcompframe {

StackPanel::StackPanel(Orientation orientation)
    : UIElement("stack_panel"), orientation_(orientation) {}

void StackPanel::set_spacing(float spacing) {
    spacing_ = spacing;
}

void StackPanel::set_wrap_enabled(bool enabled) {
    wrap_enabled_ = enabled;
}

void StackPanel::set_main_axis_alignment(LayoutAxisAlignment alignment) {
    main_axis_alignment_ = alignment;
}

void StackPanel::set_cross_axis_alignment(LayoutAxisAlignment alignment) {
    cross_axis_alignment_ = alignment;
}

Size StackPanel::measure(const Size& available_size) {
    const float infinite = std::numeric_limits<float>::infinity();
    float total_main = 0.0F;
    float max_cross = 0.0F;
    float line_main = 0.0F;
    float line_cross = 0.0F;
    bool has_children = false;

    for (const auto& child : children()) {
        const Thickness margin = child->margin();
        const float available_cross = std::isfinite(cross_extent(orientation_, available_size))
            ? std::max(0.0F, cross_extent(orientation_, available_size) - cross_leading_margin(orientation_, margin)
                    - cross_trailing_margin(orientation_, margin))
            : infinite;
        const Size child_constraint = make_size(orientation_, infinite, available_cross);
        const Size child_size = child->measure(child_constraint);
        const float child_main = leading_margin(orientation_, margin) + main_extent(orientation_, child_size)
            + trailing_margin(orientation_, margin);
        const float child_cross = cross_leading_margin(orientation_, margin) + cross_extent(orientation_, child_size)
            + cross_trailing_margin(orientation_, margin);

        if (wrap_enabled_ && orientation_ == Orientation::Horizontal
            && std::isfinite(available_size.width) && line_main > 0.0F
            && (line_main + spacing_ + child_main) > available_size.width) {
            total_main += line_cross;
            if (has_children) {
                total_main += spacing_;
            }
            max_cross = std::max(max_cross, line_main);
            line_main = child_main;
            line_cross = child_cross;
            has_children = true;
            continue;
        }

        if (wrap_enabled_ && orientation_ == Orientation::Horizontal) {
            if (line_main > 0.0F) {
                line_main += spacing_;
            }
            line_main += child_main;
            line_cross = std::max(line_cross, child_cross);
            has_children = true;
            continue;
        }

        if (has_children) {
            total_main += spacing_;
        }
        total_main += child_main;
        max_cross = std::max(max_cross, child_cross);
        has_children = true;
    }

    if (wrap_enabled_ && orientation_ == Orientation::Horizontal) {
        total_main += line_cross;
        max_cross = std::max(max_cross, line_main);
        set_measured_size(clamp_size_to_available(make_size(orientation_, total_main, max_cross), available_size));
        return measured_size();
    }

    set_measured_size(clamp_size_to_available(make_size(orientation_, total_main, max_cross), available_size));
    return measured_size();
}

void StackPanel::arrange(const Size& available_size) {
    const Rect current_bounds = bounds();
    set_bounds(Rect {.x = current_bounds.x, .y = current_bounds.y, .width = available_size.width, .height = available_size.height});

    if (wrap_enabled_ && orientation_ == Orientation::Horizontal) {
        float cursor_x = 0.0F;
        float cursor_y = 0.0F;
        float line_height = 0.0F;
        for (const auto& child : children()) {
            const Thickness margin = child->margin();
            const float child_cross_available = std::max(0.0F, available_size.height - margin.top - margin.bottom);
            const Size child_size = child->measure(Size {
                .width = std::numeric_limits<float>::infinity(),
                .height = child_cross_available,
            });
            const float outer_width = margin.left + child_size.width + margin.right;
            const float outer_height = margin.top + child_size.height + margin.bottom;

            if (cursor_x > 0.0F && (cursor_x + outer_width) > available_size.width) {
                cursor_x = 0.0F;
                cursor_y += line_height + spacing_;
                line_height = 0.0F;
            }

            child->set_bounds(Rect {
                .x = cursor_x + margin.left,
                .y = cursor_y + margin.top,
                .width = child_size.width,
                .height = child_size.height,
            });
            child->arrange(child_size);
            cursor_x += outer_width + spacing_;
            line_height = std::max(line_height, outer_height);
        }
        return;
    }

    std::vector<Size> measured_children;
    measured_children.reserve(children().size());
    float occupied_main = 0.0F;
    float total_grow = 0.0F;
    float total_shrink_weight = 0.0F;

    for (const auto& child : children()) {
        const Thickness margin = child->margin();
        const float available_cross = std::max(
            0.0F,
            cross_extent(orientation_, available_size) - cross_leading_margin(orientation_, margin)
                - cross_trailing_margin(orientation_, margin));
        const Size child_size = child->measure(make_size(orientation_, std::numeric_limits<float>::infinity(), available_cross));
        measured_children.push_back(child_size);

        occupied_main += leading_margin(orientation_, margin) + main_extent(orientation_, child_size)
            + trailing_margin(orientation_, margin);
        total_grow += child->flex_grow();
        total_shrink_weight += child->flex_shrink() * main_extent(orientation_, child_size);
    }

    if (!children().empty()) {
        occupied_main += spacing_ * static_cast<float>(children().size() - 1U);
    }

    const float available_main = main_extent(orientation_, available_size);
    const float free_space = available_main - occupied_main;
    float cursor_main = 0.0F;
    if (free_space > 0.0F && total_grow <= 0.0F) {
        if (main_axis_alignment_ == LayoutAxisAlignment::Center) {
            cursor_main = free_space * 0.5F;
        } else if (main_axis_alignment_ == LayoutAxisAlignment::End) {
            cursor_main = free_space;
        }
    }

    for (std::size_t index = 0; index < children().size(); ++index) {
        const auto& child = children()[index];
        const Size child_size = measured_children[index];
        const Thickness margin = child->margin();

        float child_main = main_extent(orientation_, child_size);
        if (free_space > 0.0F && total_grow > 0.0F && child->flex_grow() > 0.0F) {
            child_main += free_space * (child->flex_grow() / total_grow);
        } else if (free_space < 0.0F && total_shrink_weight > 0.0F && child->flex_shrink() > 0.0F) {
            const float shrink_weight = child->flex_shrink() * main_extent(orientation_, child_size);
            child_main = std::max(0.0F, child_main + free_space * (shrink_weight / total_shrink_weight));
        }

        const float available_cross = std::max(
            0.0F,
            cross_extent(orientation_, available_size) - cross_leading_margin(orientation_, margin)
                - cross_trailing_margin(orientation_, margin));
        float child_cross = cross_extent(orientation_, child_size);
        if (cross_axis_alignment_ == LayoutAxisAlignment::Stretch) {
            child_cross = available_cross;
        } else {
            child_cross = std::min(child_cross, available_cross);
        }

        float cross_offset = cross_leading_margin(orientation_, margin);
        if (cross_axis_alignment_ == LayoutAxisAlignment::Center) {
            cross_offset += std::max(0.0F, (available_cross - child_cross) * 0.5F);
        } else if (cross_axis_alignment_ == LayoutAxisAlignment::End) {
            cross_offset += std::max(0.0F, available_cross - child_cross);
        }

        const Size arranged_size = make_size(orientation_, child_main, child_cross);
        const float origin_x = orientation_ == Orientation::Vertical ? cross_offset : cursor_main + margin.left;
        const float origin_y = orientation_ == Orientation::Vertical ? cursor_main + margin.top : cross_offset;

        child->set_bounds(Rect {
            .x = origin_x,
            .y = origin_y,
            .width = arranged_size.width,
            .height = arranged_size.height,
        });
        child->arrange(arranged_size);
        cursor_main += leading_margin(orientation_, margin) + child_main + trailing_margin(orientation_, margin);
        if (index + 1U < children().size()) {
            cursor_main += spacing_;
        }
    }
}

}  // namespace dcompframe
