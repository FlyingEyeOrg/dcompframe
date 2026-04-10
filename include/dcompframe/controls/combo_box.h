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
#include "dcompframe/binding/observable.h"

namespace dcompframe {


class ComboBox : public StyledElement {
public:
    using SelectionChangedHandler = std::function<void(std::optional<std::size_t>, const std::string&)>;

    ComboBox();

    void set_items(std::vector<std::string> items);
    [[nodiscard]] const std::vector<std::string>& items() const;

    void set_selected_index(std::size_t index);
    [[nodiscard]] std::optional<std::size_t> selected_index() const;
    [[nodiscard]] std::string selected_text() const;
    void open_dropdown();
    void close_dropdown();
    void toggle_dropdown();
    [[nodiscard]] bool is_dropdown_open() const;
    bool select_next();
    bool select_previous();
    void set_on_selection_changed(SelectionChangedHandler handler);

private:
    void notify_selection_changed();

    std::vector<std::string> items_;
    std::optional<std::size_t> selected_index_ {};
    bool dropdown_open_ = false;
    SelectionChangedHandler on_selection_changed_ {};
};

}  // namespace dcompframe
