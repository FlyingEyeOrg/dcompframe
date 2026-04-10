#pragma once

#include "dcompframe/controls/styled_element.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace dcompframe {

class RadioGroup : public StyledElement {
public:
    using SelectionChangedHandler = std::function<void(std::optional<std::size_t>, const std::string&)>;

    RadioGroup();

    void set_items(std::vector<std::string> items);
    [[nodiscard]] const std::vector<std::string>& items() const;

    void set_selected_index(std::size_t index);
    [[nodiscard]] std::optional<std::size_t> selected_index() const;
    [[nodiscard]] std::string selected_text() const;
    bool select_next();
    bool select_previous();
    void set_on_selection_changed(SelectionChangedHandler handler);

private:
    void notify_selection_changed();

    std::vector<std::string> items_ {};
    std::optional<std::size_t> selected_index_ {};
    SelectionChangedHandler on_selection_changed_ {};
};

}  // namespace dcompframe