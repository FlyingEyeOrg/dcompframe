#pragma once

#include "dcompframe/controls/styled_element.h"

#include <string>

namespace dcompframe {

enum class BadgeTone {
    Neutral,
    Primary,
    Success,
    Warning,
    Danger,
};

class Badge : public StyledElement {
public:
    explicit Badge(std::string text = "");

    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const;

    void set_tone(BadgeTone tone);
    [[nodiscard]] BadgeTone tone() const;

private:
    std::string text_ {};
    BadgeTone tone_ = BadgeTone::Neutral;
};

}  // namespace dcompframe