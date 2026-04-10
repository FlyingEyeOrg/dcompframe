#pragma once

#include "dcompframe/ui_element.h"

namespace dcompframe {

class FlexPanel : public UIElement {
public:
    explicit FlexPanel(FlexDirection direction = FlexDirection::Column);

    void set_direction(FlexDirection direction);
    [[nodiscard]] FlexDirection direction() const;

    void set_wrap(FlexWrap wrap);
    [[nodiscard]] FlexWrap wrap() const;

    void set_justify_content(FlexJustifyContent justify_content);
    [[nodiscard]] FlexJustifyContent justify_content() const;

    void set_align_items(FlexAlignItems align_items);
    [[nodiscard]] FlexAlignItems align_items() const;

    void set_align_content(FlexAlignContent align_content);
    [[nodiscard]] FlexAlignContent align_content() const;

    void set_row_gap(float row_gap);
    [[nodiscard]] float row_gap() const;

    void set_column_gap(float column_gap);
    [[nodiscard]] float column_gap() const;

    Size measure(const Size& available_size) override;
    void arrange(const Size& available_size) override;

private:
    FlexDirection direction_ = FlexDirection::Column;
    FlexWrap wrap_ = FlexWrap::NoWrap;
    FlexJustifyContent justify_content_ = FlexJustifyContent::Start;
    FlexAlignItems align_items_ = FlexAlignItems::Stretch;
    FlexAlignContent align_content_ = FlexAlignContent::Stretch;
    float row_gap_ = 0.0F;
    float column_gap_ = 0.0F;
};

}  // namespace dcompframe