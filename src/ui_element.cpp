#include "dcompframe/ui_element.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace dcompframe {

namespace {

float clamp_dimension(float proposed, float available) {
    const float safe_proposed = proposed > 0.0F ? proposed : 0.0F;
    if (!std::isfinite(available)) {
        return safe_proposed;
    }

    return std::clamp(safe_proposed, 0.0F, available);
}

float apply_upper_bound(float value, float upper_bound) {
    if (upper_bound < 0.0F) {
        return value;
    }

    return std::min(value, upper_bound);
}

float apply_lower_bound(float value, float lower_bound) {
    if (lower_bound <= 0.0F) {
        return value;
    }

    return std::max(value, lower_bound);
}

Size apply_size_constraints(const Size& proposed, const Size& min_size, const Size& max_size) {
    return Size {
        .width = apply_upper_bound(apply_lower_bound(proposed.width, min_size.width), max_size.width),
        .height = apply_upper_bound(apply_lower_bound(proposed.height, min_size.height), max_size.height),
    };
}

Size intrinsic_size_for_element(const std::string& name) {
    if (name == "button") {
        return Size {.width = 132.0F, .height = 40.0F};
    }
    if (name == "text_box" || name == "combo_box" || name == "check_box" || name == "slider") {
        return Size {.width = 280.0F, .height = 40.0F};
    }
    if (name == "rich_text_box") {
        return Size {.width = 320.0F, .height = 180.0F};
    }
    if (name == "text_block") {
        return Size {.width = 320.0F, .height = 56.0F};
    }
    if (name == "label") {
        return Size {.width = 160.0F, .height = 24.0F};
    }
    if (name == "image") {
        return Size {.width = 320.0F, .height = 140.0F};
    }
    if (name == "progress" || name == "loading") {
        return Size {.width = 220.0F, .height = 28.0F};
    }
    if (name == "list_view" || name == "items_control" || name == "scroll_viewer") {
        return Size {.width = 320.0F, .height = 180.0F};
    }
    if (name == "log_box") {
        return Size {.width = 320.0F, .height = 160.0F};
    }
    if (name == "tab_control") {
        return Size {.width = 280.0F, .height = 32.0F};
    }
    if (name == "popup") {
        return Size {.width = 280.0F, .height = 132.0F};
    }
    if (name == "expander") {
        return Size {.width = 280.0F, .height = 84.0F};
    }
    if (name == "card") {
        return Size {.width = 360.0F, .height = 280.0F};
    }
    if (name == "toggle_switch") {
        return Size {.width = 64.0F, .height = 32.0F};
    }
    if (name == "radio_group") {
        return Size {.width = 280.0F, .height = 40.0F};
    }
    if (name == "badge") {
        return Size {.width = 96.0F, .height = 28.0F};
    }
    if (name == "divider") {
        return Size {.width = 0.0F, .height = 12.0F};
    }

    return Size {};
}

}  // namespace

void LayoutManager::set_strategy(LayoutStrategy strategy) {
    strategy_ = strategy;
}

void LayoutManager::apply_layout(const std::shared_ptr<UIElement>& root, const Size& available_size) const {
    if (!root) {
        return;
    }

    root->set_bounds(Rect {.x = 0.0F, .y = 0.0F, .width = available_size.width, .height = available_size.height});
    (void)strategy_;
    root->measure(available_size);
    root->arrange(available_size);
}

UIElement::UIElement(std::string name) : name_(std::move(name)) {}

Size UIElement::measure(const Size& available_size) {
    const Size intrinsic = intrinsic_size_for_element(name_);
    const Size proposed {
        .width = desired_size_.width > 0.0F ? desired_size_.width : intrinsic.width,
        .height = desired_size_.height > 0.0F ? desired_size_.height : intrinsic.height,
    };
    const Size resolved = clamp_size_to_available(apply_size_constraints(proposed, min_size_, max_size_), available_size);
    set_measured_size(resolved);
    return resolved;
}

void UIElement::arrange(const Size& available_size) {
    const Rect current_bounds = bounds_;
    set_bounds(Rect {.x = current_bounds.x, .y = current_bounds.y, .width = available_size.width, .height = available_size.height});
}

bool UIElement::add_child(const Ptr& child) {
    if (!child || child.get() == this) {
        return false;
    }

    if (std::find(children_.begin(), children_.end(), child) != children_.end()) {
        return false;
    }

    const auto existing_parent = child->parent();
    if (existing_parent && existing_parent.get() != this) {
        return false;
    }

    children_.push_back(child);
    child->parent_ = shared_from_this();
    mark_dirty();
    return true;
}

bool UIElement::remove_child(const Ptr& child) {
    const auto it = std::find(children_.begin(), children_.end(), child);
    if (it == children_.end()) {
        return false;
    }

    (*it)->parent_.reset();
    children_.erase(it);
    mark_dirty();
    return true;
}

const std::vector<UIElement::Ptr>& UIElement::children() const {
    return children_;
}

std::shared_ptr<UIElement> UIElement::parent() const {
    return parent_.lock();
}

void UIElement::set_bounds(const Rect& bounds) {
    bounds_ = bounds;
    mark_dirty();
}

Rect UIElement::bounds() const {
    return bounds_;
}

Rect UIElement::absolute_bounds() const {
    Rect result = bounds_;
    auto current = parent_.lock();
    while (current) {
        const Rect parent_bounds = current->bounds();
        result.x += parent_bounds.x;
        result.y += parent_bounds.y;
        current = current->parent();
    }
    return result;
}

void UIElement::set_desired_size(const Size& desired_size) {
    desired_size_ = desired_size;
    mark_dirty();
}

Size UIElement::desired_size() const {
    return desired_size_;
}

Size UIElement::measured_size() const {
    return measured_size_;
}

void UIElement::set_flex_grow(float flex_grow) {
    flex_grow_ = flex_grow > 0.0F ? flex_grow : 0.0F;
    mark_dirty();
}

float UIElement::flex_grow() const {
    return flex_grow_;
}

void UIElement::set_flex_shrink(float flex_shrink) {
    flex_shrink_ = flex_shrink >= 0.0F ? flex_shrink : 0.0F;
    mark_dirty();
}

float UIElement::flex_shrink() const {
    return flex_shrink_;
}

void UIElement::set_flex_basis(float flex_basis) {
    flex_basis_ = flex_basis >= 0.0F ? flex_basis : -1.0F;
    mark_dirty();
}

float UIElement::flex_basis() const {
    return flex_basis_;
}

bool UIElement::has_flex_basis() const {
    return flex_basis_ >= 0.0F;
}

void UIElement::set_order(int order) {
    order_ = order;
    mark_dirty();
}

int UIElement::order() const {
    return order_;
}

void UIElement::set_align_self(FlexAlignSelf align_self) {
    align_self_ = align_self;
    mark_dirty();
}

FlexAlignSelf UIElement::align_self() const {
    return align_self_;
}

void UIElement::set_opacity(float opacity) {
    opacity_ = std::clamp(opacity, 0.0F, 1.0F);
    mark_dirty();
}

float UIElement::opacity() const {
    return opacity_;
}

void UIElement::set_clip_rect(const Rect& clip_rect) {
    clip_rect_ = clip_rect;
    mark_dirty();
}

Rect UIElement::clip_rect() const {
    return clip_rect_;
}

void UIElement::set_margin(const Thickness& margin) {
    margin_ = margin;
    mark_dirty();
}

Thickness UIElement::margin() const {
    return margin_;
}

void UIElement::set_padding(const Thickness& padding) {
    padding_ = padding;
    mark_dirty();
}

Thickness UIElement::padding() const {
    return padding_;
}

void UIElement::set_min_size(const Size& min_size) {
    min_size_ = Size {
        .width = std::max(0.0F, min_size.width),
        .height = std::max(0.0F, min_size.height),
    };
    if (max_size_.width >= 0.0F && max_size_.width < min_size_.width) {
        max_size_.width = min_size_.width;
    }
    if (max_size_.height >= 0.0F && max_size_.height < min_size_.height) {
        max_size_.height = min_size_.height;
    }
    mark_dirty();
}

Size UIElement::min_size() const {
    return min_size_;
}

void UIElement::set_max_size(const Size& max_size) {
    max_size_ = Size {
        .width = max_size.width >= 0.0F ? std::max(max_size.width, min_size_.width) : -1.0F,
        .height = max_size.height >= 0.0F ? std::max(max_size.height, min_size_.height) : -1.0F,
    };
    mark_dirty();
}

Size UIElement::max_size() const {
    return max_size_;
}

void UIElement::set_transform(float translate_x, float translate_y, float scale_x, float scale_y, float rotation_deg) {
    translation_ = Point {.x = translate_x, .y = translate_y};
    scale_ = Point {.x = scale_x, .y = scale_y};
    rotation_deg_ = rotation_deg;
    mark_dirty();
}

Point UIElement::translation() const {
    return translation_;
}

Point UIElement::scale() const {
    return scale_;
}

float UIElement::rotation_deg() const {
    return rotation_deg_;
}

void UIElement::set_focusable(bool focusable) {
    focusable_ = focusable;
    if (!focusable_) {
        focused_ = false;
    }
}

bool UIElement::is_focusable() const {
    return focusable_;
}

void UIElement::set_focused(bool focused) {
    if (!focusable_ && focused) {
        return;
    }

    focused_ = focused;
    mark_dirty();
}

bool UIElement::is_focused() const {
    return focused_;
}

void UIElement::set_hit_test_visible(bool hit_test_visible) {
    hit_test_visible_ = hit_test_visible;
    mark_dirty();
}

bool UIElement::hit_test_visible() const {
    return hit_test_visible_;
}

void UIElement::set_event_handler(EventHandler handler) {
    event_handler_ = std::move(handler);
}

void UIElement::dispatch_event(InputEvent& event) {
    if (event.target == nullptr) {
        event.target = this;
    }

    std::vector<std::shared_ptr<UIElement>> path;
    auto current = parent();
    while (current) {
        path.push_back(current);
        current = current->parent();
    }

    for (auto it = path.rbegin(); it != path.rend() && !event.handled; ++it) {
        event.phase = EventPhase::Capture;
        event.current_target = it->get();
        (*it)->handle_event(event, EventPhase::Capture);
    }

    if (event.handled) {
        event.current_target = nullptr;
        return;
    }

    event.phase = EventPhase::Target;
    event.current_target = this;
    handle_event(event, EventPhase::Target);

    if (event.handled) {
        event.current_target = nullptr;
        return;
    }

    for (const auto& ancestor : path) {
        event.phase = EventPhase::Bubble;
        event.current_target = ancestor.get();
        ancestor->handle_event(event, EventPhase::Bubble);
        if (event.handled) {
            event.current_target = nullptr;
            return;
        }
    }

    event.current_target = nullptr;
}

bool UIElement::contains_point(const Point& position) const {
    const Rect rect = absolute_bounds();
    if (rect.width <= 0.0F || rect.height <= 0.0F) {
        return false;
    }

    const bool inside_bounds = position.x >= rect.x && position.x <= (rect.x + rect.width)
        && position.y >= rect.y && position.y <= (rect.y + rect.height);
    if (!inside_bounds) {
        return false;
    }

    if (clip_rect_.width <= 0.0F || clip_rect_.height <= 0.0F) {
        return true;
    }

    const Rect clip {
        .x = rect.x + clip_rect_.x,
        .y = rect.y + clip_rect_.y,
        .width = clip_rect_.width,
        .height = clip_rect_.height,
    };
    return position.x >= clip.x && position.x <= (clip.x + clip.width)
        && position.y >= clip.y && position.y <= (clip.y + clip.height);
}

std::shared_ptr<UIElement> UIElement::hit_test(const Point& position) {
    if (!hit_test_visible_ || !contains_point(position)) {
        return nullptr;
    }

    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if (const auto hit = (*it)->hit_test(position)) {
            return hit;
        }
    }

    return shared_from_this();
}

const std::string& UIElement::name() const {
    return name_;
}

bool UIElement::is_dirty() const {
    return dirty_;
}

void UIElement::clear_dirty_recursive() {
    dirty_ = false;
    for (const auto& child : children_) {
        child->clear_dirty_recursive();
    }
}

void UIElement::handle_event(InputEvent& event, EventPhase phase) const {
    if (event_handler_) {
        event_handler_(event, phase);
    }
}

void UIElement::mark_dirty() {
    dirty_ = true;
    if (const auto p = parent_.lock()) {
        p->mark_dirty();
    }
}

void UIElement::set_measured_size(const Size& measured_size) {
    measured_size_ = measured_size;
}

Size UIElement::clamp_size_to_available(const Size& proposed_size, const Size& available_size) const {
    const Size constrained = apply_size_constraints(proposed_size, min_size_, max_size_);
    return Size {
        .width = clamp_dimension(constrained.width, available_size.width),
        .height = clamp_dimension(constrained.height, available_size.height),
    };
}

}  // namespace dcompframe
