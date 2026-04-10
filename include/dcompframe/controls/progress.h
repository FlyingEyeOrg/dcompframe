#pragma once

#include "dcompframe/controls/styled_element.h"

namespace dcompframe {

class Progress : public StyledElement {
public:
    Progress();

    void set_range(float min_value, float max_value);
    void set_value(float value);
    [[nodiscard]] float value() const;
    [[nodiscard]] float min_value() const;
    [[nodiscard]] float max_value() const;

    void set_indeterminate(bool indeterminate);
    [[nodiscard]] bool is_indeterminate() const;

    [[nodiscard]] float normalized_value() const;

private:
    float min_ = 0.0F;
    float max_ = 100.0F;
    float value_ = 0.0F;
    bool indeterminate_ = false;
};

}  // namespace dcompframe
