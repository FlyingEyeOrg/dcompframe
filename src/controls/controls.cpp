#include "dcompframe/controls/controls.h"

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

}  // namespace dcompframe
