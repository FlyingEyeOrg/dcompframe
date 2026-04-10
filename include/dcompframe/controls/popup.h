#pragma once

#include "dcompframe/controls/styled_element.h"

#include <string>

namespace dcompframe {

class Popup : public StyledElement {
public:
    Popup();

    void set_open(bool open);
    [[nodiscard]] bool is_open() const;

    void set_modal(bool modal);
    [[nodiscard]] bool is_modal() const;

    void set_title(std::string title);
    [[nodiscard]] const std::string& title() const;

    void set_body(std::string body);
    [[nodiscard]] const std::string& body() const;

private:
    bool open_ = false;
    bool modal_ = false;
    std::string title_;
    std::string body_;
};

}  // namespace dcompframe
