#include "dcompframe/layout/grid_panel.h"

#include <algorithm>

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

void GridPanel::arrange(const Size& available_size) {
    const Rect current_bounds = bounds();
    set_bounds(Rect {.x = current_bounds.x, .y = current_bounds.y, .width = available_size.width, .height = available_size.height});

    const float cell_width = available_size.width / static_cast<float>(cols_);
    const float cell_height = available_size.height / static_cast<float>(rows_);

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
        const float raw_x = static_cast<float>(safe_col) * cell_width;
        const float raw_y = static_cast<float>(safe_row) * cell_height;
        const float raw_width = static_cast<float>(safe_col_span) * cell_width;
        const float raw_height = static_cast<float>(safe_row_span) * cell_height;

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
