#pragma once

#include "dcompframe/controls/styled_element.h"

#include <string>

namespace dcompframe {

class Loading : public StyledElement {
public:
    Loading();

    void set_active(bool active);
    [[nodiscard]] bool active() const;

    void set_overlay_mode(bool overlay_mode);
    [[nodiscard]] bool overlay_mode() const;

    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const;

private:
    bool active_ = false;
    bool overlay_mode_ = false;
    std::string text_ = "Loading...";
};

}  // namespace dcompframe
