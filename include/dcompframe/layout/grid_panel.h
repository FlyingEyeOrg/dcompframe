#pragma once

#include <vector>
#include <unordered_map>

#include "dcompframe/ui_element.h"

namespace dcompframe {

class GridPanel : public UIElement {
public:
    struct Cell {
        int row = 0;
        int col = 0;
        int row_span = 1;
        int col_span = 1;
    };

    GridPanel(int rows, int cols);

    void set_rows_cols(int rows, int cols);
    void set_grid_position(const Ptr& child, Cell cell);
    Size measure(const Size& available_size) override;
    void arrange(const Size& available_size) override;

private:
    int rows_ = 1;
    int cols_ = 1;
    std::unordered_map<const UIElement*, Cell> placements_;
    std::vector<float> measured_row_heights_ {};
    std::vector<float> measured_col_widths_ {};
};

}  // namespace dcompframe
