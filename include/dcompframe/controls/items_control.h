#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "dcompframe/controls/styled_element.h"

namespace dcompframe {

class ItemsControl : public StyledElement {
public:
    ItemsControl();

    void set_items(std::vector<std::string> items);
    [[nodiscard]] const std::vector<std::string>& items() const;

    void append_item(std::string item);
    void clear_items();

    void set_selected_index(std::size_t index);
    [[nodiscard]] std::optional<std::size_t> selected_index() const;

    void set_item_spacing(float item_spacing);
    [[nodiscard]] float item_spacing() const;
    void set_scroll_offset(float scroll_offset);
    [[nodiscard]] float scroll_offset() const;
    void scroll_by(float delta);

    [[nodiscard]] std::pair<std::size_t, std::size_t> visible_range(
        float scroll_offset,
        float viewport_height,
        float item_height) const;

private:
    std::vector<std::string> items_;
    std::optional<std::size_t> selected_index_ {};
    float item_spacing_ = 8.0F;
    float scroll_offset_ = 0.0F;
};

}  // namespace dcompframe