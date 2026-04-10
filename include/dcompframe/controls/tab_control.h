#pragma once

#include "dcompframe/controls/styled_element.h"

#include <optional>
#include <string>
#include <vector>

namespace dcompframe {

class TabControl : public StyledElement {
public:
    TabControl();

    void set_tabs(std::vector<std::string> tabs);
    [[nodiscard]] const std::vector<std::string>& tabs() const;

    void set_selected_index(std::size_t index);
    [[nodiscard]] std::optional<std::size_t> selected_index() const;
    [[nodiscard]] std::string selected_tab() const;

    bool select_next();
    bool select_previous();

private:
    std::vector<std::string> tabs_;
    std::optional<std::size_t> selected_index_ {};
};

}  // namespace dcompframe
