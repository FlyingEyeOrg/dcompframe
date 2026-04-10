#include "dcompframe/layout/stack_panel.h"

namespace dcompframe {

StackPanel::StackPanel(Orientation orientation)
    : FlexPanel(orientation == Orientation::Vertical ? FlexDirection::Column : FlexDirection::Row) {
    set_align_content(FlexAlignContent::Start);
}

void StackPanel::set_spacing(float spacing) {
    set_row_gap(spacing);
    set_column_gap(spacing);
}

void StackPanel::set_wrap_enabled(bool enabled) {
    set_wrap(enabled ? FlexWrap::Wrap : FlexWrap::NoWrap);
}

void StackPanel::set_main_axis_alignment(LayoutAxisAlignment alignment) {
    switch (alignment) {
    case LayoutAxisAlignment::Start:
        set_justify_content(FlexJustifyContent::Start);
        break;
    case LayoutAxisAlignment::Center:
        set_justify_content(FlexJustifyContent::Center);
        break;
    case LayoutAxisAlignment::End:
        set_justify_content(FlexJustifyContent::End);
        break;
    case LayoutAxisAlignment::Stretch:
        set_justify_content(FlexJustifyContent::Start);
        break;
    }
}

void StackPanel::set_cross_axis_alignment(LayoutAxisAlignment alignment) {
    switch (alignment) {
    case LayoutAxisAlignment::Start:
        set_align_items(FlexAlignItems::Start);
        break;
    case LayoutAxisAlignment::Center:
        set_align_items(FlexAlignItems::Center);
        break;
    case LayoutAxisAlignment::End:
        set_align_items(FlexAlignItems::End);
        break;
    case LayoutAxisAlignment::Stretch:
        set_align_items(FlexAlignItems::Stretch);
        break;
    }
}

}  // namespace dcompframe
