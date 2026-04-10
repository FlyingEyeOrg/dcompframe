#include "dcompframe/layout/flex_panel.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

namespace dcompframe {

namespace {

struct FlexItemState {
    UIElement::Ptr child;
    Thickness margin {};
    Size measured_size {};
    float base_main = 0.0F;
    float target_main = 0.0F;
    float target_cross = 0.0F;
    float outer_base_main = 0.0F;
    float outer_cross = 0.0F;
};

struct FlexLineState {
    std::vector<FlexItemState> items;
    float used_main = 0.0F;
    float cross_size = 0.0F;
};

bool is_horizontal(FlexDirection direction) {
    return direction == FlexDirection::Row || direction == FlexDirection::RowReverse;
}

bool is_reverse(FlexDirection direction) {
    return direction == FlexDirection::RowReverse || direction == FlexDirection::ColumnReverse;
}

float main_extent(FlexDirection direction, const Size& size) {
    return is_horizontal(direction) ? size.width : size.height;
}

float cross_extent(FlexDirection direction, const Size& size) {
    return is_horizontal(direction) ? size.height : size.width;
}

Size make_size(FlexDirection direction, float main, float cross) {
    if (is_horizontal(direction)) {
        return Size {.width = main, .height = cross};
    }

    return Size {.width = cross, .height = main};
}

float leading_margin(FlexDirection direction, const Thickness& margin) {
    return is_horizontal(direction) ? margin.left : margin.top;
}

float trailing_margin(FlexDirection direction, const Thickness& margin) {
    return is_horizontal(direction) ? margin.right : margin.bottom;
}

float cross_leading_margin(FlexDirection direction, const Thickness& margin) {
    return is_horizontal(direction) ? margin.top : margin.left;
}

float cross_trailing_margin(FlexDirection direction, const Thickness& margin) {
    return is_horizontal(direction) ? margin.bottom : margin.right;
}

float main_gap(FlexDirection direction, float row_gap, float column_gap) {
    return is_horizontal(direction) ? column_gap : row_gap;
}

float cross_gap(FlexDirection direction, float row_gap, float column_gap) {
    return is_horizontal(direction) ? row_gap : column_gap;
}

FlexAlignSelf resolve_align_self(const UIElement& child, FlexAlignItems align_items) {
    if (child.align_self() != FlexAlignSelf::Auto) {
        return child.align_self();
    }

    switch (align_items) {
    case FlexAlignItems::Start:
        return FlexAlignSelf::Start;
    case FlexAlignItems::End:
        return FlexAlignSelf::End;
    case FlexAlignItems::Center:
        return FlexAlignSelf::Center;
    case FlexAlignItems::Stretch:
        return FlexAlignSelf::Stretch;
    }

    return FlexAlignSelf::Stretch;
}

FlexItemState measure_flex_item(
    FlexDirection direction,
    const UIElement::Ptr& child,
    const Size& available_size) {
    const float infinite = std::numeric_limits<float>::infinity();
    const Thickness margin = child->margin();
    const float available_cross = std::isfinite(cross_extent(direction, available_size))
        ? std::max(
            0.0F,
            cross_extent(direction, available_size) - cross_leading_margin(direction, margin)
                - cross_trailing_margin(direction, margin))
        : infinite;
    const float measured_main_limit = child->has_flex_basis() ? child->flex_basis() : infinite;
    const Size measured_size = child->measure(make_size(direction, measured_main_limit, available_cross));

    FlexItemState item {
        .child = child,
        .margin = margin,
        .measured_size = measured_size,
    };
    item.base_main = child->has_flex_basis() ? child->flex_basis() : main_extent(direction, measured_size);
    item.target_main = item.base_main;
    item.target_cross = cross_extent(direction, measured_size);
    item.outer_base_main = leading_margin(direction, margin) + item.base_main + trailing_margin(direction, margin);
    item.outer_cross = cross_leading_margin(direction, margin) + item.target_cross + cross_trailing_margin(direction, margin);
    return item;
}

void resolve_line_flex(
    FlexDirection direction,
    float available_main,
    FlexLineState& line,
    float gap) {
    if (line.items.empty()) {
        return;
    }

    float occupied_main = gap * static_cast<float>(line.items.size() > 1 ? line.items.size() - 1U : 0U);
    float total_grow = 0.0F;
    float total_shrink_weight = 0.0F;
    for (const auto& item : line.items) {
        occupied_main += item.outer_base_main;
        total_grow += item.child->flex_grow();
        total_shrink_weight += item.child->flex_shrink() * item.base_main;
    }

    const float free_space = std::isfinite(available_main) ? available_main - occupied_main : 0.0F;
    line.used_main = gap * static_cast<float>(line.items.size() > 1 ? line.items.size() - 1U : 0U);
    line.cross_size = 0.0F;

    for (auto& item : line.items) {
        item.target_main = item.base_main;
        if (free_space > 0.0F && total_grow > 0.0F && item.child->flex_grow() > 0.0F) {
            item.target_main += free_space * (item.child->flex_grow() / total_grow);
        } else if (free_space < 0.0F && total_shrink_weight > 0.0F && item.child->flex_shrink() > 0.0F) {
            const float shrink_weight = item.child->flex_shrink() * item.base_main;
            item.target_main = std::max(0.0F, item.base_main + free_space * (shrink_weight / total_shrink_weight));
        }

        item.target_cross = cross_extent(direction, item.measured_size);
        item.outer_cross = cross_leading_margin(direction, item.margin) + item.target_cross
            + cross_trailing_margin(direction, item.margin);
        line.used_main += leading_margin(direction, item.margin) + item.target_main
            + trailing_margin(direction, item.margin);
        line.cross_size = std::max(line.cross_size, item.outer_cross);
    }
}

std::vector<FlexItemState> ordered_items(FlexDirection direction, const std::vector<UIElement::Ptr>& children, const Size& available_size) {
    std::vector<std::pair<std::size_t, UIElement::Ptr>> sorted_children;
    sorted_children.reserve(children.size());
    for (std::size_t index = 0; index < children.size(); ++index) {
        sorted_children.push_back({index, children[index]});
    }

    std::stable_sort(
        sorted_children.begin(),
        sorted_children.end(),
        [](const auto& left, const auto& right) {
            if (left.second->order() != right.second->order()) {
                return left.second->order() < right.second->order();
            }
            return left.first < right.first;
        });

    std::vector<FlexItemState> items;
    items.reserve(sorted_children.size());
    for (const auto& entry : sorted_children) {
        items.push_back(measure_flex_item(direction, entry.second, available_size));
    }
    return items;
}

std::vector<FlexLineState> build_flex_lines(
    FlexDirection direction,
    FlexWrap wrap,
    const std::vector<FlexItemState>& items,
    const Size& available_size,
    float gap) {
    std::vector<FlexLineState> lines;
    if (items.empty()) {
        return lines;
    }

    const bool wraps = wrap != FlexWrap::NoWrap;
    const float available_main = main_extent(direction, available_size);
    FlexLineState current_line;
    float current_line_main = 0.0F;

    for (const auto& item : items) {
        const float next_main = current_line.items.empty()
            ? item.outer_base_main
            : current_line_main + gap + item.outer_base_main;
        if (wraps && std::isfinite(available_main) && !current_line.items.empty() && next_main > available_main) {
            resolve_line_flex(direction, available_main, current_line, gap);
            lines.push_back(std::move(current_line));
            current_line = FlexLineState {};
            current_line_main = 0.0F;
        }

        current_line.items.push_back(item);
        current_line_main = current_line.items.size() == 1U ? item.outer_base_main : current_line_main + gap + item.outer_base_main;
    }

    resolve_line_flex(direction, available_main, current_line, gap);
    lines.push_back(std::move(current_line));

    return lines;
}

float compute_leading_space(FlexJustifyContent justify_content, float remaining_space, std::size_t item_count) {
    if (remaining_space <= 0.0F) {
        return 0.0F;
    }

    switch (justify_content) {
    case FlexJustifyContent::Start:
        return 0.0F;
    case FlexJustifyContent::End:
        return remaining_space;
    case FlexJustifyContent::Center:
        return remaining_space * 0.5F;
    case FlexJustifyContent::SpaceBetween:
        return 0.0F;
    case FlexJustifyContent::SpaceAround:
        return item_count > 0U ? (remaining_space / static_cast<float>(item_count)) * 0.5F : 0.0F;
    }

    return 0.0F;
}

float compute_between_spacing(
    FlexJustifyContent justify_content,
    float remaining_space,
    std::size_t item_count,
    float gap) {
    if (remaining_space <= 0.0F) {
        return gap;
    }

    switch (justify_content) {
    case FlexJustifyContent::Start:
    case FlexJustifyContent::End:
    case FlexJustifyContent::Center:
        return gap;
    case FlexJustifyContent::SpaceBetween:
        return item_count > 1U ? gap + remaining_space / static_cast<float>(item_count - 1U) : gap;
    case FlexJustifyContent::SpaceAround:
        return item_count > 0U ? gap + remaining_space / static_cast<float>(item_count) : gap;
    }

    return gap;
}

float compute_cross_leading_space(FlexAlignContent align_content, float remaining_space, std::size_t line_count) {
    if (remaining_space <= 0.0F) {
        return 0.0F;
    }

    switch (align_content) {
    case FlexAlignContent::Start:
    case FlexAlignContent::Stretch:
        return 0.0F;
    case FlexAlignContent::End:
        return remaining_space;
    case FlexAlignContent::Center:
        return remaining_space * 0.5F;
    case FlexAlignContent::SpaceBetween:
        return 0.0F;
    case FlexAlignContent::SpaceAround:
        return line_count > 0U ? (remaining_space / static_cast<float>(line_count)) * 0.5F : 0.0F;
    }

    return 0.0F;
}

float compute_cross_between_spacing(
    FlexAlignContent align_content,
    float remaining_space,
    std::size_t line_count,
    float gap) {
    if (remaining_space <= 0.0F) {
        return gap;
    }

    switch (align_content) {
    case FlexAlignContent::Start:
    case FlexAlignContent::End:
    case FlexAlignContent::Center:
    case FlexAlignContent::Stretch:
        return gap;
    case FlexAlignContent::SpaceBetween:
        return line_count > 1U ? gap + remaining_space / static_cast<float>(line_count - 1U) : gap;
    case FlexAlignContent::SpaceAround:
        return line_count > 0U ? gap + remaining_space / static_cast<float>(line_count) : gap;
    }

    return gap;
}

Size apply_container_desired_size(const UIElement& element, Size measured_size) {
    const Size desired = element.desired_size();
    if (desired.width > 0.0F) {
        measured_size.width = desired.width;
    }
    if (desired.height > 0.0F) {
        measured_size.height = desired.height;
    }
    return measured_size;
}

}  // namespace

FlexPanel::FlexPanel(FlexDirection direction)
    : UIElement("flex_panel"), direction_(direction) {}

void FlexPanel::set_direction(FlexDirection direction) {
    direction_ = direction;
    mark_dirty();
}

FlexDirection FlexPanel::direction() const {
    return direction_;
}

void FlexPanel::set_wrap(FlexWrap wrap) {
    wrap_ = wrap;
    mark_dirty();
}

FlexWrap FlexPanel::wrap() const {
    return wrap_;
}

void FlexPanel::set_justify_content(FlexJustifyContent justify_content) {
    justify_content_ = justify_content;
    mark_dirty();
}

FlexJustifyContent FlexPanel::justify_content() const {
    return justify_content_;
}

void FlexPanel::set_align_items(FlexAlignItems align_items) {
    align_items_ = align_items;
    mark_dirty();
}

FlexAlignItems FlexPanel::align_items() const {
    return align_items_;
}

void FlexPanel::set_align_content(FlexAlignContent align_content) {
    align_content_ = align_content;
    mark_dirty();
}

FlexAlignContent FlexPanel::align_content() const {
    return align_content_;
}

void FlexPanel::set_row_gap(float row_gap) {
    row_gap_ = std::max(0.0F, row_gap);
    mark_dirty();
}

float FlexPanel::row_gap() const {
    return row_gap_;
}

void FlexPanel::set_column_gap(float column_gap) {
    column_gap_ = std::max(0.0F, column_gap);
    mark_dirty();
}

float FlexPanel::column_gap() const {
    return column_gap_;
}

Size FlexPanel::measure(const Size& available_size) {
    const auto items = ordered_items(direction_, children(), available_size);
    const auto lines = build_flex_lines(direction_, wrap_, items, available_size, main_gap(direction_, row_gap_, column_gap_));

    float measured_main = 0.0F;
    float measured_cross = 0.0F;
    const float line_gap = cross_gap(direction_, row_gap_, column_gap_);
    for (std::size_t index = 0; index < lines.size(); ++index) {
        measured_main = std::max(measured_main, lines[index].used_main);
        measured_cross += lines[index].cross_size;
        if (index + 1U < lines.size()) {
            measured_cross += line_gap;
        }
    }

    const Size desired_size = apply_container_desired_size(*this, make_size(direction_, measured_main, measured_cross));
    set_measured_size(clamp_size_to_available(desired_size, available_size));
    return measured_size();
}

void FlexPanel::arrange(const Size& available_size) {
    const Rect current_bounds = bounds();
    set_bounds(Rect {.x = current_bounds.x, .y = current_bounds.y, .width = available_size.width, .height = available_size.height});

    auto lines = build_flex_lines(direction_, wrap_, ordered_items(direction_, children(), available_size), available_size, main_gap(direction_, row_gap_, column_gap_));
    if (lines.empty()) {
        return;
    }

    const float available_main = main_extent(direction_, available_size);
    const float available_cross = cross_extent(direction_, available_size);
    const float line_gap = cross_gap(direction_, row_gap_, column_gap_);

    if (lines.size() == 1U && std::isfinite(available_cross)) {
        lines.front().cross_size = std::max(lines.front().cross_size, available_cross);
    }

    float content_cross = 0.0F;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        content_cross += lines[index].cross_size;
        if (index + 1U < lines.size()) {
            content_cross += line_gap;
        }
    }

    const float remaining_cross = std::isfinite(available_cross) ? available_cross - content_cross : 0.0F;
    if (lines.size() > 1U && remaining_cross > 0.0F && align_content_ == FlexAlignContent::Stretch) {
        const float stretch_delta = remaining_cross / static_cast<float>(lines.size());
        for (auto& line : lines) {
            line.cross_size += stretch_delta;
        }
    }

    const float cross_start_offset = compute_cross_leading_space(align_content_, remaining_cross, lines.size());
    const float between_lines = compute_cross_between_spacing(align_content_, remaining_cross, lines.size(), line_gap);

    float line_cursor = cross_start_offset;
    if (wrap_ == FlexWrap::WrapReverse && std::isfinite(available_cross)) {
        line_cursor = available_cross - cross_start_offset;
    }

    for (auto& line : lines) {
        const float remaining_main = std::isfinite(available_main) ? available_main - line.used_main : 0.0F;
        const float leading_space = compute_leading_space(justify_content_, remaining_main, line.items.size());
        const float between_items = compute_between_spacing(justify_content_, remaining_main, line.items.size(), main_gap(direction_, row_gap_, column_gap_));

        float main_cursor = 0.0F;
        if (is_reverse(direction_)) {
            main_cursor = std::isfinite(available_main) ? available_main - leading_space : line.used_main;
        } else {
            main_cursor = leading_space;
        }

        const float line_cross_origin = wrap_ == FlexWrap::WrapReverse && std::isfinite(available_cross)
            ? line_cursor - line.cross_size
            : line_cursor;

        for (std::size_t index = 0; index < line.items.size(); ++index) {
            auto& item = line.items[index];
            const FlexAlignSelf align_self = resolve_align_self(*item.child, align_items_);
            const float cross_available_for_item = std::max(
                0.0F,
                line.cross_size - cross_leading_margin(direction_, item.margin)
                    - cross_trailing_margin(direction_, item.margin));
            float arranged_cross = item.target_cross;
            if (align_self == FlexAlignSelf::Stretch) {
                arranged_cross = cross_available_for_item;
            }

            float cross_offset = cross_leading_margin(direction_, item.margin);
            if (align_self == FlexAlignSelf::Center) {
                cross_offset += std::max(0.0F, (cross_available_for_item - arranged_cross) * 0.5F);
            } else if (align_self == FlexAlignSelf::End) {
                cross_offset += std::max(0.0F, cross_available_for_item - arranged_cross);
            }

            float item_main_origin = 0.0F;
            if (is_reverse(direction_)) {
                main_cursor -= trailing_margin(direction_, item.margin) + item.target_main;
                item_main_origin = main_cursor;
                main_cursor -= leading_margin(direction_, item.margin);
            } else {
                main_cursor += leading_margin(direction_, item.margin);
                item_main_origin = main_cursor;
                main_cursor += item.target_main + trailing_margin(direction_, item.margin);
            }

            const Size arranged_size = make_size(direction_, item.target_main, arranged_cross);
            const float origin_x = is_horizontal(direction_) ? item_main_origin : line_cross_origin + cross_offset;
            const float origin_y = is_horizontal(direction_) ? line_cross_origin + cross_offset : item_main_origin;

            item.child->set_bounds(Rect {
                .x = origin_x,
                .y = origin_y,
                .width = arranged_size.width,
                .height = arranged_size.height,
            });
            item.child->arrange(arranged_size);

            if (index + 1U < line.items.size()) {
                if (is_reverse(direction_)) {
                    main_cursor -= between_items;
                } else {
                    main_cursor += between_items;
                }
            }
        }

        if (wrap_ == FlexWrap::WrapReverse && std::isfinite(available_cross)) {
            line_cursor = line_cross_origin - between_lines;
        } else {
            line_cursor = line_cross_origin + line.cross_size + between_lines;
        }
    }
}

}  // namespace dcompframe