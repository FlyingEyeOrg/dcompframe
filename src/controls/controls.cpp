#include "dcompframe/controls/controls.h"

#include <algorithm>
#include <cmath>

#include <windows.h>

namespace dcompframe {

namespace {

template <typename T>
T min_value(const T& left, const T& right) {
    return left < right ? left : right;
}

template <typename T>
T max_value(const T& left, const T& right) {
    return left > right ? left : right;
}

template <typename T>
T clamp_value(const T& value, const T& low, const T& high) {
    return min_value(max_value(value, low), high);
}

std::wstring utf8_to_wstring(const std::string& value) {
    if (value.empty()) {
        return {};
    }

    const int count = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (count <= 1) {
        return {};
    }

    std::vector<wchar_t> buffer(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, buffer.data(), count);
    return std::wstring(buffer.data());
}

std::string wstring_to_utf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }

    const int count = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (count <= 1) {
        return {};
    }

    std::vector<char> buffer(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, buffer.data(), count, nullptr, nullptr);
    return std::string(buffer.data());
}

std::size_t code_unit_length(const std::string& value) {
    return utf8_to_wstring(value).size();
}

}  // namespace

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

Label::Label(std::string text) : StyledElement("label"), text_(std::move(text)) {}

void Label::set_text(std::string text) {
    text_ = std::move(text);
    mark_dirty();
}

const std::string& Label::text() const {
    return text_;
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

Progress::Progress() : StyledElement("progress") {}

void Progress::set_range(float min_value, float max_value) {
    min_ = min_value;
    max_ = max_value >= min_value ? max_value : min_value;
    value_ = clamp_value(value_, min_, max_);
    mark_dirty();
}

void Progress::set_value(float value) {
    value_ = clamp_value(value, min_, max_);
    mark_dirty();
}

float Progress::value() const {
    return value_;
}

float Progress::min_value() const {
    return min_;
}

float Progress::max_value() const {
    return max_;
}

void Progress::set_indeterminate(bool indeterminate) {
    indeterminate_ = indeterminate;
    mark_dirty();
}

bool Progress::is_indeterminate() const {
    return indeterminate_;
}

float Progress::normalized_value() const {
    const float range = max_ - min_;
    if (range <= 0.0001F) {
        return 0.0F;
    }
    return clamp_value((value_ - min_) / range, 0.0F, 1.0F);
}

Loading::Loading() : StyledElement("loading") {}

void Loading::set_active(bool active) {
    active_ = active;
    mark_dirty();
}

bool Loading::active() const {
    return active_;
}

void Loading::set_overlay_mode(bool overlay_mode) {
    overlay_mode_ = overlay_mode;
    mark_dirty();
}

bool Loading::overlay_mode() const {
    return overlay_mode_;
}

void Loading::set_text(std::string text) {
    text_ = std::move(text);
    mark_dirty();
}

const std::string& Loading::text() const {
    return text_;
}

TabControl::TabControl() : StyledElement("tab_control") {
    set_focusable(true);
}

void TabControl::set_tabs(std::vector<std::string> tabs) {
    tabs_ = std::move(tabs);
    if (tabs_.empty()) {
        selected_index_.reset();
    } else if (!selected_index_ || *selected_index_ >= tabs_.size()) {
        selected_index_ = 0U;
    }
    mark_dirty();
}

const std::vector<std::string>& TabControl::tabs() const {
    return tabs_;
}

void TabControl::set_selected_index(std::size_t index) {
    if (tabs_.empty() || index >= tabs_.size()) {
        selected_index_.reset();
    } else {
        selected_index_ = index;
    }
    mark_dirty();
}

std::optional<std::size_t> TabControl::selected_index() const {
    return selected_index_;
}

std::string TabControl::selected_tab() const {
    if (!selected_index_ || *selected_index_ >= tabs_.size()) {
        return {};
    }
    return tabs_[*selected_index_];
}

bool TabControl::select_next() {
    if (tabs_.empty()) {
        return false;
    }
    const std::size_t next_index = selected_index_ ? ((*selected_index_ + 1U) % tabs_.size()) : 0U;
    set_selected_index(next_index);
    return true;
}

bool TabControl::select_previous() {
    if (tabs_.empty()) {
        return false;
    }
    const std::size_t prev_index = selected_index_ ? ((*selected_index_ + tabs_.size() - 1U) % tabs_.size()) : 0U;
    set_selected_index(prev_index);
    return true;
}

Popup::Popup() : StyledElement("popup") {
    set_focusable(true);
}

void Popup::set_open(bool open) {
    open_ = open;
    mark_dirty();
}

bool Popup::is_open() const {
    return open_;
}

void Popup::set_modal(bool modal) {
    modal_ = modal;
    mark_dirty();
}

bool Popup::is_modal() const {
    return modal_;
}

void Popup::set_title(std::string title) {
    title_ = std::move(title);
    mark_dirty();
}

const std::string& Popup::title() const {
    return title_;
}

void Popup::set_body(std::string body) {
    body_ = std::move(body);
    mark_dirty();
}

const std::string& Popup::body() const {
    return body_;
}

Expander::Expander() : StyledElement("expander") {
    set_focusable(true);
}

void Expander::set_header(std::string header) {
    header_ = std::move(header);
    mark_dirty();
}

const std::string& Expander::header() const {
    return header_;
}

void Expander::set_content_text(std::string content) {
    content_text_ = std::move(content);
    mark_dirty();
}

const std::string& Expander::content_text() const {
    return content_text_;
}

void Expander::set_expanded(bool expanded) {
    expanded_ = expanded;
    mark_dirty();
}

bool Expander::expanded() const {
    return expanded_;
}

bool Expander::toggle() {
    set_expanded(!expanded_);
    return expanded_;
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
    clamp_selection();
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
    const std::size_t max_size = code_unit_length(text_);
    selection_start_ = min_value(start, max_size);
    selection_end_ = min_value(end, max_size);
    caret_position_ = selection_end_;
    mark_dirty();
}

std::pair<std::size_t, std::size_t> TextBox::selection() const {
    return {min_value(selection_start_, selection_end_), max_value(selection_start_, selection_end_)};
}

bool TextBox::has_selection() const {
    return selection_start_ != selection_end_;
}

void TextBox::clear_selection() {
    selection_start_ = caret_position_;
    selection_end_ = caret_position_;
    mark_dirty();
}

void TextBox::select_all() {
    selection_start_ = 0;
    selection_end_ = code_unit_length(text_);
    caret_position_ = selection_end_;
    mark_dirty();
}

void TextBox::set_caret_position(std::size_t position, bool extend_selection) {
    const std::size_t clamped = min_value(position, code_unit_length(text_));
    caret_position_ = clamped;
    if (extend_selection) {
        selection_end_ = caret_position_;
    } else {
        selection_start_ = caret_position_;
        selection_end_ = caret_position_;
    }
    mark_dirty();
}

std::size_t TextBox::caret_position() const {
    return caret_position_;
}

void TextBox::move_caret_left(bool extend_selection) {
    if (caret_position_ == 0) {
        return;
    }

    set_caret_position(caret_position_ - 1, extend_selection);
}

void TextBox::move_caret_right(bool extend_selection) {
    const std::size_t length = code_unit_length(text_);
    if (caret_position_ >= length) {
        return;
    }

    set_caret_position(caret_position_ + 1, extend_selection);
}

void TextBox::move_caret_home(bool extend_selection) {
    set_caret_position(0, extend_selection);
}

void TextBox::move_caret_end(bool extend_selection) {
    set_caret_position(code_unit_length(text_), extend_selection);
}

bool TextBox::insert_text(std::string text) {
    if (text.empty()) {
        return false;
    }

    replace_selection_with(text);
    return true;
}

bool TextBox::backspace() {
    if (has_selection()) {
        replace_selection_with("");
        return true;
    }

    if (caret_position_ == 0) {
        return false;
    }

    const std::wstring wide = utf8_to_wstring(text_);
    std::wstring updated = wide;
    updated.erase(caret_position_ - 1, 1);
    text_ = wstring_to_utf8(updated);
    caret_position_ -= 1;
    selection_start_ = caret_position_;
    selection_end_ = caret_position_;
    notify_text_changed();
    mark_dirty();
    return true;
}

bool TextBox::delete_forward() {
    if (has_selection()) {
        replace_selection_with("");
        return true;
    }

    const std::wstring wide = utf8_to_wstring(text_);
    if (caret_position_ >= wide.size()) {
        return false;
    }

    std::wstring updated = wide;
    updated.erase(caret_position_, 1);
    text_ = wstring_to_utf8(updated);
    notify_text_changed();
    mark_dirty();
    return true;
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

    replace_selection_with(composition_text_);
    composition_text_.clear();
}

void TextBox::bind_text(Observable<std::string>& observable) {
    if (text_binding_id_ != 0) {
        bound_text_->unbind(text_binding_id_);
    }

    bound_text_ = &observable;
    text_binding_id_ = observable.bind([this](const std::string& value) {
        updating_from_binding_ = true;
        set_text(value);
        updating_from_binding_ = false;
    });
    set_text(observable.get());
}

void TextBox::set_on_text_changed(TextChangedHandler handler) {
    on_text_changed_ = std::move(handler);
}

void TextBox::replace_selection_with(const std::string& replacement) {
    const auto [begin, end] = selection();
    const std::wstring wide_text = utf8_to_wstring(text_);
    const std::wstring wide_replacement = utf8_to_wstring(replacement);
    std::wstring updated = wide_text;
    updated.replace(begin, end - begin, wide_replacement);
    text_ = wstring_to_utf8(updated);
    caret_position_ = begin + wide_replacement.size();
    selection_start_ = caret_position_;
    selection_end_ = caret_position_;
    notify_text_changed();
    mark_dirty();
}

void TextBox::clamp_selection() {
    const std::size_t length = code_unit_length(text_);
    selection_start_ = min_value(selection_start_, length);
    selection_end_ = min_value(selection_end_, length);
    caret_position_ = min_value(caret_position_, length);
}

void TextBox::notify_text_changed() {
    if (!updating_from_binding_ && bound_text_ != nullptr) {
        bound_text_->set(text_);
    }

    if (on_text_changed_) {
        on_text_changed_(text_);
    }
}

RichTextBox::RichTextBox() : StyledElement("rich_text_box") {
    set_focusable(true);
    set_text_alignment(TextHorizontalAlignment::Left, TextVerticalAlignment::Top);
}

void RichTextBox::set_rich_text(std::string rich_text) {
    rich_text_ = std::move(rich_text);
    clamp_selection();
    mark_dirty();
}

const std::string& RichTextBox::rich_text() const {
    return rich_text_;
}

void RichTextBox::set_selection(std::size_t start, std::size_t end) {
    const std::size_t max_size = code_unit_length(rich_text_);
    selection_start_ = min_value(start, max_size);
    selection_end_ = min_value(end, max_size);
    caret_position_ = selection_end_;
    mark_dirty();
}

std::pair<std::size_t, std::size_t> RichTextBox::selection() const {
    return {min_value(selection_start_, selection_end_), max_value(selection_start_, selection_end_)};
}

bool RichTextBox::has_selection() const {
    return selection_start_ != selection_end_;
}

void RichTextBox::clear_selection() {
    selection_start_ = caret_position_;
    selection_end_ = caret_position_;
    mark_dirty();
}

void RichTextBox::select_all() {
    selection_start_ = 0;
    selection_end_ = code_unit_length(rich_text_);
    caret_position_ = selection_end_;
    mark_dirty();
}

void RichTextBox::set_caret_position(std::size_t position, bool extend_selection) {
    const std::size_t clamped = min_value(position, code_unit_length(rich_text_));
    caret_position_ = clamped;
    if (extend_selection) {
        selection_end_ = caret_position_;
    } else {
        selection_start_ = caret_position_;
        selection_end_ = caret_position_;
    }
    mark_dirty();
}

std::size_t RichTextBox::caret_position() const {
    return caret_position_;
}

void RichTextBox::move_caret_left(bool extend_selection) {
    if (caret_position_ == 0) {
        return;
    }

    set_caret_position(caret_position_ - 1, extend_selection);
}

void RichTextBox::move_caret_right(bool extend_selection) {
    const std::size_t length = code_unit_length(rich_text_);
    if (caret_position_ >= length) {
        return;
    }

    set_caret_position(caret_position_ + 1, extend_selection);
}

void RichTextBox::move_caret_home(bool extend_selection) {
    set_caret_position(0, extend_selection);
}

void RichTextBox::move_caret_end(bool extend_selection) {
    set_caret_position(code_unit_length(rich_text_), extend_selection);
}

bool RichTextBox::insert_text(std::string text) {
    replace_selection_with(std::move(text));
    return true;
}

bool RichTextBox::backspace() {
    if (has_selection()) {
        replace_selection_with("");
        return true;
    }

    if (caret_position_ == 0) {
        return false;
    }

    const std::wstring wide = utf8_to_wstring(rich_text_);
    std::wstring updated = wide;
    updated.erase(updated.begin() + static_cast<std::ptrdiff_t>(caret_position_ - 1));
    rich_text_ = wstring_to_utf8(updated);
    set_caret_position(caret_position_ - 1, false);
    mark_dirty();
    return true;
}

bool RichTextBox::delete_forward() {
    if (has_selection()) {
        replace_selection_with("");
        return true;
    }

    const std::wstring wide = utf8_to_wstring(rich_text_);
    if (caret_position_ >= wide.size()) {
        return false;
    }

    std::wstring updated = wide;
    updated.erase(updated.begin() + static_cast<std::ptrdiff_t>(caret_position_));
    rich_text_ = wstring_to_utf8(updated);
    mark_dirty();
    return true;
}

void RichTextBox::set_scroll_offset(float scroll_offset) {
    scroll_offset_ = max_value(0.0F, scroll_offset);
    mark_dirty();
}

float RichTextBox::scroll_offset() const {
    return scroll_offset_;
}

void RichTextBox::scroll_by(float delta) {
    set_scroll_offset(scroll_offset_ + delta);
}

void RichTextBox::replace_selection_with(const std::string& replacement) {
    const auto [start, end] = selection();
    std::wstring wide = utf8_to_wstring(rich_text_);
    std::wstring replacement_wide = utf8_to_wstring(replacement);
    wide.replace(start, end - start, replacement_wide);
    rich_text_ = wstring_to_utf8(wide);
    const std::size_t next_position = start + replacement_wide.size();
    selection_start_ = next_position;
    selection_end_ = next_position;
    caret_position_ = next_position;
    mark_dirty();
}

void RichTextBox::clamp_selection() {
    const std::size_t length = code_unit_length(rich_text_);
    selection_start_ = min_value(selection_start_, length);
    selection_end_ = min_value(selection_end_, length);
    caret_position_ = min_value(caret_position_, length);
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

    const std::size_t begin = static_cast<std::size_t>(max_value(0.0F, scroll_offset) / item_height);
    const std::size_t visible_count = static_cast<std::size_t>(std::ceil(viewport_height / item_height)) + 1U;
    const std::size_t end = min_value(items_.size(), begin + visible_count);
    return {min_value(begin, items_.size()), end};
}

void ListView::set_scroll_offset(float scroll_offset) {
    scroll_offset_ = max_value(0.0F, scroll_offset);
    mark_dirty();
}

float ListView::scroll_offset() const {
    return scroll_offset_;
}

void ListView::scroll_by(float delta) {
    set_scroll_offset(scroll_offset_ + delta);
}

ItemsControl::ItemsControl() : StyledElement("items_control") {
    set_focusable(true);
}

void ItemsControl::set_items(std::vector<std::string> items) {
    items_ = std::move(items);
    if (selected_index_ && *selected_index_ >= items_.size()) {
        selected_index_.reset();
    }
    mark_dirty();
}

const std::vector<std::string>& ItemsControl::items() const {
    return items_;
}

void ItemsControl::append_item(std::string item) {
    items_.push_back(std::move(item));
    mark_dirty();
}

void ItemsControl::clear_items() {
    items_.clear();
    selected_index_.reset();
    mark_dirty();
}

void ItemsControl::set_selected_index(std::size_t index) {
    if (index < items_.size()) {
        selected_index_ = index;
    } else {
        selected_index_.reset();
    }
    mark_dirty();
}

std::optional<std::size_t> ItemsControl::selected_index() const {
    return selected_index_;
}

void ItemsControl::set_item_spacing(float item_spacing) {
    item_spacing_ = item_spacing >= 0.0F ? item_spacing : 0.0F;
    mark_dirty();
}

float ItemsControl::item_spacing() const {
    return item_spacing_;
}

void ItemsControl::set_scroll_offset(float scroll_offset) {
    scroll_offset_ = max_value(0.0F, scroll_offset);
    mark_dirty();
}

float ItemsControl::scroll_offset() const {
    return scroll_offset_;
}

void ItemsControl::scroll_by(float delta) {
    set_scroll_offset(scroll_offset_ + delta);
}

std::pair<std::size_t, std::size_t> ItemsControl::visible_range(float scroll_offset, float viewport_height, float item_height) const {
    if (items_.empty() || viewport_height <= 0.0F || item_height <= 0.0F) {
        return {0U, 0U};
    }

    const float stride = item_height + item_spacing_;
    const std::size_t begin = static_cast<std::size_t>(max_value(0.0F, scroll_offset) / max_value(1.0F, stride));
    const std::size_t visible_count = static_cast<std::size_t>(std::ceil(viewport_height / max_value(1.0F, stride))) + 1U;
    const std::size_t end = min_value(items_.size(), begin + visible_count);
    return {min_value(begin, items_.size()), end};
}

ScrollViewer::ScrollViewer() : StyledElement("scroll_viewer") {}

void ScrollViewer::set_scroll_offset(float x, float y) {
    offset_ = Point {.x = max_value(0.0F, x), .y = max_value(0.0F, y)};
    mark_dirty();
}

Point ScrollViewer::scroll_offset() const {
    return offset_;
}

void ScrollViewer::scroll_by(float delta_x, float delta_y) {
    set_scroll_offset(offset_.x + delta_x, offset_.y + delta_y);
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
            return max_value(0.0F, value - deceleration * dt);
        }
        return min_value(0.0F, value + deceleration * dt);
    };

    velocity_.x = damp(velocity_.x);
    velocity_.y = damp(velocity_.y);
    mark_dirty();
}

Point ScrollViewer::inertia_velocity() const {
    return velocity_;
}

void ScrollViewer::set_content(const std::shared_ptr<ItemsControl>& content) {
    if (content_ == content) {
        return;
    }

    content_ = content;
    mark_dirty();
}

std::shared_ptr<ItemsControl> ScrollViewer::content() const {
    return content_;
}

CheckBox::CheckBox() : StyledElement("check_box") {
    set_focusable(true);
}

void CheckBox::set_checked(bool checked) {
    if (checked_ == checked) {
        return;
    }

    checked_ = checked;
    set_state(checked_ ? ControlState::Selected : ControlState::Normal);
    mark_dirty();

    if (on_checked_changed_) {
        on_checked_changed_(checked_);
    }
}

bool CheckBox::checked() const {
    return checked_;
}

bool CheckBox::toggle() {
    set_checked(!checked_);
    return checked_;
}

void CheckBox::set_on_checked_changed(CheckedChangedHandler handler) {
    on_checked_changed_ = std::move(handler);
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
    notify_selection_changed();
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
    notify_selection_changed();
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

void ComboBox::open_dropdown() {
    dropdown_open_ = !items_.empty();
    mark_dirty();
}

void ComboBox::close_dropdown() {
    dropdown_open_ = false;
    mark_dirty();
}

void ComboBox::toggle_dropdown() {
    if (dropdown_open_) {
        close_dropdown();
        return;
    }

    open_dropdown();
}

bool ComboBox::is_dropdown_open() const {
    return dropdown_open_;
}

bool ComboBox::select_next() {
    if (items_.empty()) {
        return false;
    }

    const std::size_t next = selected_index_ ? min_value(*selected_index_ + 1U, items_.size() - 1U) : 0U;
    if (selected_index_ && *selected_index_ == next) {
        return false;
    }

    set_selected_index(next);
    return true;
}

bool ComboBox::select_previous() {
    if (items_.empty()) {
        return false;
    }

    const std::size_t previous = selected_index_ ? (*selected_index_ == 0U ? 0U : *selected_index_ - 1U) : 0U;
    if (selected_index_ && *selected_index_ == previous) {
        return false;
    }

    set_selected_index(previous);
    return true;
}

void ComboBox::set_on_selection_changed(SelectionChangedHandler handler) {
    on_selection_changed_ = std::move(handler);
}

void ComboBox::notify_selection_changed() {
    if (on_selection_changed_) {
        on_selection_changed_(selected_index_, selected_text());
    }
}

Slider::Slider() : StyledElement("slider") {
    set_focusable(true);
}

void Slider::set_range(float min_value, float max_value) {
    min_ = min_value;
    max_ = max_value >= min_value ? max_value : min_value;
    value_ = clamp_value(value_, min_, max_);
    mark_dirty();
}

void Slider::set_value(float value) {
    const float clamped = clamp_value(value, min_, max_);
    if (std::abs(clamped - value_) <= 0.0001F) {
        return;
    }

    value_ = clamped;
    mark_dirty();

    if (on_value_changed_) {
        on_value_changed_(value_);
    }
}

float Slider::value() const {
    return value_;
}

float Slider::min_value() const {
    return min_;
}

float Slider::max_value() const {
    return max_;
}

void Slider::set_step(float step) {
    step_ = step > 0.0F ? step : 1.0F;
}

float Slider::step() const {
    return step_;
}

float Slider::normalized_value() const {
    const float range = max_ - min_;
    if (range <= 0.0001F) {
        return 0.0F;
    }

    return (value_ - min_) / range;
}

void Slider::set_value_from_ratio(float ratio) {
    const float clamped_ratio = clamp_value(ratio, 0.0F, 1.0F);
    set_value(min_ + (max_ - min_) * clamped_ratio);
}

bool Slider::step_by(float delta_steps) {
    const float previous = value_;
    set_value(value_ + delta_steps * step_);
    return std::abs(previous - value_) > 0.0001F;
}

void Slider::set_on_value_changed(ValueChangedHandler handler) {
    on_value_changed_ = std::move(handler);
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
