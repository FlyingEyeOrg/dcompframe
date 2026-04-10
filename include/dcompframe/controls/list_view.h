#pragma once
#include <optional>
#include <cstddef>
#include <chrono>

#include "dcompframe/ui_element.h"
#include "dcompframe/controls/controls_common.h"
#include "dcompframe/controls/styled_element.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <utility>
#include "dcompframe/binding/observable.h"

namespace dcompframe {


struct ListGroup {
    std::string name;
    std::vector<std::string> items;
};

class ListView : public StyledElement {
public:
    ListView();

    void set_items(std::vector<std::string> items);
    [[nodiscard]] const std::vector<std::string>& items() const;
    void set_selected_index(std::size_t index);
    [[nodiscard]] std::optional<std::size_t> selected_index() const;

    void set_groups(std::vector<ListGroup> groups);
    [[nodiscard]] const std::vector<ListGroup>& groups() const;
    [[nodiscard]] std::pair<std::size_t, std::size_t> visible_range(float scroll_offset, float viewport_height, float item_height) const;

private:
    std::vector<std::string> items_;
    std::vector<ListGroup> groups_;
    std::optional<std::size_t> selected_index_ {};
};

}  // namespace dcompframe
