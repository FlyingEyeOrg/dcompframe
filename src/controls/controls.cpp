#include "dcompframe/controls/controls.h"

#include <algorithm>
#include <cmath>

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

void StyledElement::set_text_alignment(TextHorizontalAlignment horizontal, TextVerticalAlignment vertical) {
    text_horizontal_alignment_ = horizontal;
    text_vertical_alignment_ = vertical;
    mark_dirty();
}

TextHorizontalAlignment StyledElement::text_horizontal_alignment() const {
    return text_horizontal_alignment_;
}

TextVerticalAlignment StyledElement::text_vertical_alignment() const {
    return text_vertical_alignment_;
}

Panel::Panel() : StyledElement("panel") {}

void Panel::arrange(const Size& available_size) {
    set_bounds(Rect {.x = 0.0F, .y = 0.0F, .width = available_size.width, .height = available_size.height});
    for (const auto& child : children()) {
        child->set_bounds(Rect {.x = 0.0F, .y = 0.0F, .width = available_size.width, .height = available_size.height});
    }
}

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

void TextBox::set_selection(std::size_t start, std::size_t end) {
    const std::size_t max_size = text_.size();
    selection_start_ = std::min(start, max_size);
    selection_end_ = std::min(end, max_size);
    if (selection_start_ > selection_end_) {
        std::swap(selection_start_, selection_end_);
    }
    mark_dirty();
}

std::pair<std::size_t, std::size_t> TextBox::selection() const {
    return {selection_start_, selection_end_};
}

void TextBox::set_composition_text(std::string text) {
    composition_text_ = std::move(text);
    mark_dirty();
}

const std::string& TextBox::composition_text() const {
    return composition_text_;
}

void TextBox::commit_composition() {
    if (composition_text_.empty()) {
        return;
    }

    const std::size_t start = std::min(selection_start_, text_.size());
    const std::size_t end = std::min(selection_end_, text_.size());
    text_.replace(start, end - start, composition_text_);
    selection_start_ = start + composition_text_.size();
    selection_end_ = selection_start_;
    composition_text_.clear();
    mark_dirty();
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

RichTextBox::RichTextBox() : StyledElement("rich_text_box") {
    set_focusable(true);
    set_text_alignment(TextHorizontalAlignment::Left, TextVerticalAlignment::Top);
}

void RichTextBox::set_rich_text(std::string rich_text) {
    rich_text_ = std::move(rich_text);
    mark_dirty();
}

const std::string& RichTextBox::rich_text() const {
    return rich_text_;
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

void ListView::set_groups(std::vector<ListGroup> groups) {
    groups_ = std::move(groups);
    items_.clear();
    for (const auto& group : groups_) {
        items_.insert(items_.end(), group.items.begin(), group.items.end());
    }
    if (selected_index_ && *selected_index_ >= items_.size()) {
        selected_index_.reset();
    }
    mark_dirty();
}

const std::vector<ListGroup>& ListView::groups() const {
    return groups_;
}

std::pair<std::size_t, std::size_t> ListView::visible_range(float scroll_offset, float viewport_height, float item_height) const {
    if (items_.empty() || viewport_height <= 0.0F || item_height <= 0.0F) {
        return {0U, 0U};
    }

    const std::size_t begin = static_cast<std::size_t>(std::max(0.0F, scroll_offset) / item_height);
    const std::size_t visible_count = static_cast<std::size_t>(std::ceil(viewport_height / item_height)) + 1U;
    const std::size_t end = std::min(items_.size(), begin + visible_count);
    return {std::min(begin, items_.size()), end};
}

ScrollViewer::ScrollViewer() : StyledElement("scroll_viewer") {}

void ScrollViewer::set_scroll_offset(float x, float y) {
    offset_ = Point {.x = x, .y = y};
    mark_dirty();
}

Point ScrollViewer::scroll_offset() const {
    return offset_;
}

void ScrollViewer::set_inertia_velocity(float x, float y) {
    velocity_ = Point {.x = x, .y = y};
}

void ScrollViewer::tick_inertia(std::chrono::milliseconds delta_time, float deceleration) {
    const float dt = static_cast<float>(delta_time.count());
    if (dt <= 0.0F) {
        return;
    }

    offset_.x += velocity_.x * dt;
    offset_.y += velocity_.y * dt;

    const auto damp = [&](float value) {
        if (value > 0.0F) {
            return std::max(0.0F, value - deceleration * dt);
        }
        return std::min(0.0F, value + deceleration * dt);
    };

    velocity_.x = damp(velocity_.x);
    velocity_.y = damp(velocity_.y);
    mark_dirty();
}

Point ScrollViewer::inertia_velocity() const {
    return velocity_;
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

ComboBox::ComboBox() : StyledElement("combo_box") {
    set_focusable(true);
}

void ComboBox::set_items(std::vector<std::string> items) {
    items_ = std::move(items);
    if (selected_index_ && *selected_index_ >= items_.size()) {
        selected_index_.reset();
    }
    mark_dirty();
}

const std::vector<std::string>& ComboBox::items() const {
    return items_;
}

void ComboBox::set_selected_index(std::size_t index) {
    if (index < items_.size()) {
        selected_index_ = index;
    } else {
        selected_index_.reset();
    }
    mark_dirty();
}

std::optional<std::size_t> ComboBox::selected_index() const {
    return selected_index_;
}

std::string ComboBox::selected_text() const {
    if (!selected_index_ || *selected_index_ >= items_.size()) {
        return {};
    }
    return items_[*selected_index_];
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
