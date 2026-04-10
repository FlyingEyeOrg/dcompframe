#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "dcompframe/controls/styled_element.h"

namespace dcompframe {

class LogBox : public StyledElement {
public:
    LogBox();

    void set_lines(std::vector<std::string> lines);
    [[nodiscard]] const std::vector<std::string>& lines() const;

    void append_line(std::string line);
    void clear();

    void set_auto_scroll(bool auto_scroll);
    [[nodiscard]] bool auto_scroll() const;

    void set_max_lines(std::size_t max_lines);
    [[nodiscard]] std::size_t max_lines() const;

    void set_scroll_offset(float scroll_offset);
    [[nodiscard]] float scroll_offset() const;
    void scroll_by(float delta);

private:
    void trim_to_max_lines();

    std::vector<std::string> lines_;
    bool auto_scroll_ = true;
    std::size_t max_lines_ = 200U;
    float scroll_offset_ = 0.0F;
};

}  // namespace dcompframe