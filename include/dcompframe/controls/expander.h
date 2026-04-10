#pragma once

#include "dcompframe/controls/styled_element.h"

#include <string>

namespace dcompframe {

class Expander : public StyledElement {
public:
    Expander();

    void set_header(std::string header);
    [[nodiscard]] const std::string& header() const;

    void set_content_text(std::string content);
    [[nodiscard]] const std::string& content_text() const;

    void set_expanded(bool expanded);
    [[nodiscard]] bool expanded() const;
    bool toggle();

private:
    std::string header_;
    std::string content_text_;
    bool expanded_ = false;
};

}  // namespace dcompframe
