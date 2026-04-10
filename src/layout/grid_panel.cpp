#include "dcompframe/layout/grid_panel.h"

#include <algorithm>
#include <numeric>
#include <limits>

namespace dcompframe {

GridPanel::GridPanel(int rows, int cols)
    : UIElement("grid_panel"), rows_(rows > 0 ? rows : 1), cols_(cols > 0 ? cols : 1) {}

void GridPanel::set_rows_cols(int rows, int cols) {
    rows_ = rows > 0 ? rows : 1;
    cols_ = cols > 0 ? cols : 1;
}

void GridPanel::set_grid_position(const Ptr& child, Cell cell) {
    if (!child) {
        return;
    }

    placements_[child.get()] = cell;
}

Size GridPanel::measure(const Size& available_size) {
    measured_row_heights_.assign(static_cast<std::size_t>(rows_), 0.0F);
    measured_col_widths_.assign(static_cast<std::size_t>(cols_), 0.0F);

    for (const auto& child : children()) {
        Cell cell {};
        if (const auto it = placements_.find(child.get()); it != placements_.end()) {
            cell = it->second;
        }

        const int safe_row = std::clamp(cell.row, 0, rows_ - 1);
        const int safe_col = std::clamp(cell.col, 0, cols_ - 1);
        const int safe_row_span = std::clamp(cell.row_span, 1, rows_ - safe_row);
        const int safe_col_span = std::clamp(cell.col_span, 1, cols_ - safe_col);
        const Thickness margin = child->margin();
        const float child_width_limit = std::isfinite(available_size.width)
            ? std::max(0.0F, available_size.width / static_cast<float>(safe_col_span) - margin.left - margin.right)
            : std::numeric_limits<float>::infinity();
        const float child_height_limit = std::isfinite(available_size.height)
            ? std::max(0.0F, available_size.height / static_cast<float>(safe_row_span) - margin.top - margin.bottom)
            : std::numeric_limits<float>::infinity();
        const Size child_size = child->measure(Size {.width = child_width_limit, .height = child_height_limit});

        const float outer_width = margin.left + child_size.width + margin.right;
        const float outer_height = margin.top + child_size.height + margin.bottom;
        const float width_per_col = outer_width / static_cast<float>(safe_col_span);
        const float height_per_row = outer_height / static_cast<float>(safe_row_span);

        for (int col = 0; col < safe_col_span; ++col) {
            measured_col_widths_[static_cast<std::size_t>(safe_col + col)] = std::max(
                measured_col_widths_[static_cast<std::size_t>(safe_col + col)],
                width_per_col);
        }
        for (int row = 0; row < safe_row_span; ++row) {
            measured_row_heights_[static_cast<std::size_t>(safe_row + row)] = std::max(
                measured_row_heights_[static_cast<std::size_t>(safe_row + row)],
                height_per_row);
        }
    }

    const float desired_width = std::accumulate(measured_col_widths_.begin(), measured_col_widths_.end(), 0.0F);
    const float desired_height = std::accumulate(measured_row_heights_.begin(), measured_row_heights_.end(), 0.0F);
    set_measured_size(clamp_size_to_available(Size {.width = desired_width, .height = desired_height}, available_size));
    return measured_size();
}

void GridPanel::arrange(const Size& available_size) {
    const Rect current_bounds = bounds();
    set_bounds(Rect {.x = current_bounds.x, .y = current_bounds.y, .width = available_size.width, .height = available_size.height});

    if (measured_col_widths_.empty() || measured_row_heights_.empty()) {
        measure(available_size);
    }

    std::vector<float> final_col_widths = measured_col_widths_;
    std::vector<float> final_row_heights = measured_row_heights_;

    const float measured_width = std::accumulate(final_col_widths.begin(), final_col_widths.end(), 0.0F);
    const float measured_height = std::accumulate(final_row_heights.begin(), final_row_heights.end(), 0.0F);
    const float extra_width = available_size.width - measured_width;
    const float extra_height = available_size.height - measured_height;

    if (!final_col_widths.empty()) {
        const float width_delta = extra_width / static_cast<float>(final_col_widths.size());
        for (float& value : final_col_widths) {
            value = std::max(0.0F, value + width_delta);
        }
    }
    if (!final_row_heights.empty()) {
        const float height_delta = extra_height / static_cast<float>(final_row_heights.size());
        for (float& value : final_row_heights) {
            value = std::max(0.0F, value + height_delta);
        }
    }

    for (const auto& child : children()) {
        Cell cell {};
        if (const auto it = placements_.find(child.get()); it != placements_.end()) {
            cell = it->second;
        }

        const int safe_row = std::clamp(cell.row, 0, rows_ - 1);
        const int safe_col = std::clamp(cell.col, 0, cols_ - 1);
        const int safe_row_span = std::clamp(cell.row_span, 1, rows_ - safe_row);
        const int safe_col_span = std::clamp(cell.col_span, 1, cols_ - safe_col);
        const Thickness margin = child->margin();
        float raw_x = 0.0F;
        for (int index = 0; index < safe_col; ++index) {
            raw_x += final_col_widths[static_cast<std::size_t>(index)];
        }
        float raw_y = 0.0F;
        for (int index = 0; index < safe_row; ++index) {
            raw_y += final_row_heights[static_cast<std::size_t>(index)];
        }
        float raw_width = 0.0F;
        for (int index = 0; index < safe_col_span; ++index) {
            raw_width += final_col_widths[static_cast<std::size_t>(safe_col + index)];
        }
        float raw_height = 0.0F;
        for (int index = 0; index < safe_row_span; ++index) {
            raw_height += final_row_heights[static_cast<std::size_t>(safe_row + index)];
        }

        child->set_bounds(Rect {
            .x = raw_x + margin.left,
            .y = raw_y + margin.top,
            .width = std::max(0.0F, raw_width - margin.left - margin.right),
            .height = std::max(0.0F, raw_height - margin.top - margin.bottom),
        });
        child->arrange(Size {
            .width = std::max(0.0F, raw_width - margin.left - margin.right),
            .height = std::max(0.0F, raw_height - margin.top - margin.bottom),
        });
    }
}

}  // namespace dcompframe
