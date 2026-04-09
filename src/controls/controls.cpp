#include "dcompframe/controls/controls.h"

#include <algorithm>

namespace dcompframe {

StyledElement::StyledElement(std::string name) : UIElement(std::move(name)) {}

void StyledElement::set_style(Style style) {
    style_ = style;
    mark_dirty();
}

Style StyledElement::style() const {
    return style_;
}

void StyledElement::set_state(ControlState state) {
    state_ = state;
    mark_dirty();
}

ControlState StyledElement::state() const {
    return state_;
}

Panel::Panel() : StyledElement("panel") {}

TextBlock::TextBlock(std::string text) : StyledElement("text_block"), text_(std::move(text)) {}

void TextBlock::set_text(std::string text) {
    text_ = std::move(text);
    mark_dirty();
}

const std::string& TextBlock::text() const {
    return text_;
}

Image::Image() : StyledElement("image") {}

void Image::set_source(std::string source) {
    source_ = std::move(source);
    mark_dirty();
}

const std::string& Image::source() const {
    return source_;
}

Button::Button(std::string text) : StyledElement("button"), text_(std::move(text)) {
    set_focusable(true);
}

void Button::set_text(std::string text) {
    text_ = std::move(text);
    mark_dirty();
}

const std::string& Button::text() const {
    return text_;
}

void Button::set_on_click(std::function<void()> callback) {
    on_click_ = std::move(callback);
}

bool Button::click() {
    if (state() == ControlState::Disabled) {
        return false;
    }

    if (on_click_) {
        on_click_();
    }

    return true;
}

void Button::bind_enabled(Observable<bool>& observable) {
    if (enabled_binding_id_ != 0) {
        observable.unbind(enabled_binding_id_);
    }

    enabled_binding_id_ = observable.bind([this](bool enabled) {
        set_state(enabled ? ControlState::Normal : ControlState::Disabled);
    });
    set_state(observable.get() ? ControlState::Normal : ControlState::Disabled);
}

TextBox::TextBox() : StyledElement("text_box") {
    set_focusable(true);
}

void TextBox::set_text(std::string text) {
    text_ = std::move(text);
    mark_dirty();
}

const std::string& TextBox::text() const {
    return text_;
}

void TextBox::set_placeholder(std::string placeholder) {
    placeholder_ = std::move(placeholder);
    mark_dirty();
}

const std::string& TextBox::placeholder() const {
    return placeholder_;
}

void TextBox::bind_text(Observable<std::string>& observable) {
    if (text_binding_id_ != 0) {
        observable.unbind(text_binding_id_);
    }

    text_binding_id_ = observable.bind([this](const std::string& value) {
        set_text(value);
    });
    set_text(observable.get());
}

ListView::ListView() : StyledElement("list_view") {
    set_focusable(true);
}

void ListView::set_items(std::vector<std::string> items) {
    items_ = std::move(items);
    if (selected_index_ && *selected_index_ >= items_.size()) {
        selected_index_.reset();
    }
    mark_dirty();
}

const std::vector<std::string>& ListView::items() const {
    return items_;
}

void ListView::set_selected_index(std::size_t index) {
    if (index < items_.size()) {
        selected_index_ = index;
    } else {
        selected_index_.reset();
    }
    mark_dirty();
}

std::optional<std::size_t> ListView::selected_index() const {
    return selected_index_;
}

ScrollViewer::ScrollViewer() : StyledElement("scroll_viewer") {}

void ScrollViewer::set_scroll_offset(float x, float y) {
    offset_ = Point {.x = x, .y = y};
    mark_dirty();
}

Point ScrollViewer::scroll_offset() const {
    return offset_;
}

CheckBox::CheckBox() : StyledElement("check_box") {
    set_focusable(true);
}

void CheckBox::set_checked(bool checked) {
    checked_ = checked;
    set_state(checked_ ? ControlState::Selected : ControlState::Normal);
    mark_dirty();
}

bool CheckBox::checked() const {
    return checked_;
}

Slider::Slider() : StyledElement("slider") {
    set_focusable(true);
}

void Slider::set_range(float min_value, float max_value) {
    min_ = min_value;
    max_ = max_value >= min_value ? max_value : min_value;
    value_ = std::clamp(value_, min_, max_);
    mark_dirty();
}

void Slider::set_value(float value) {
    value_ = std::clamp(value, min_, max_);
    mark_dirty();
}

float Slider::value() const {
    return value_;
}

Card::Card() : StyledElement("card") {}

void Card::set_title(std::string title) {
    title_ = std::move(title);
    mark_dirty();
}

void Card::set_body(std::string body) {
    body_ = std::move(body);
    mark_dirty();
}

void Card::set_icon(std::string icon) {
    icon_ = std::move(icon);
    mark_dirty();
}

void Card::set_tags(std::vector<std::string> tags) {
    tags_ = std::move(tags);
    mark_dirty();
}

const std::string& Card::title() const {
    return title_;
}

const std::string& Card::body() const {
    return body_;
}

const std::string& Card::icon() const {
    return icon_;
}

const std::vector<std::string>& Card::tags() const {
    return tags_;
}

void Card::set_primary_action(std::shared_ptr<Button> action) {
    if (primary_action_) {
        remove_child(primary_action_);
    }

    primary_action_ = std::move(action);
    if (primary_action_) {
        add_child(primary_action_);
    }

    mark_dirty();
}

std::shared_ptr<Button> Card::primary_action() const {
    return primary_action_;
}

void Card::bind(BindingContext& context) {
    if (title_binding_id_ != 0) {
        context.title.unbind(title_binding_id_);
    }
    if (body_binding_id_ != 0) {
        context.body.unbind(body_binding_id_);
    }

    title_binding_id_ = context.title.bind([this](const std::string& value) { set_title(value); });
    body_binding_id_ = context.body.bind([this](const std::string& value) { set_body(value); });
    set_title(context.title.get());
    set_body(context.body.get());
}

}  // namespace dcompframe
