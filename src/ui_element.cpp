#include "dcompframe/ui_element.h"

#include <algorithm>
#include <utility>

namespace dcompframe {

void LayoutManager::set_strategy(LayoutStrategy strategy) {
    strategy_ = strategy;
}

void LayoutManager::apply_layout(const std::shared_ptr<UIElement>& root, const Size& available_size) const {
    if (!root) {
        return;
    }

    switch (strategy_) {
    case LayoutStrategy::Absolute:
        root->set_bounds(Rect {.x = root->bounds().x, .y = root->bounds().y, .width = available_size.width, .height = available_size.height});
        break;
    case LayoutStrategy::Stack: {
        float cursor_y = 0.0F;
        for (const auto& child : root->children()) {
            const auto desired = child->desired_size();
            child->set_bounds(Rect {.x = 0.0F, .y = cursor_y, .width = desired.width, .height = desired.height});
            cursor_y += desired.height;
        }
        break;
    }
    case LayoutStrategy::Grid: {
        const auto& children = root->children();
        if (children.empty()) {
            break;
        }

        const float width = available_size.width / static_cast<float>(children.size());
        float cursor_x = 0.0F;
        for (const auto& child : children) {
            child->set_bounds(Rect {.x = cursor_x, .y = 0.0F, .width = width, .height = available_size.height});
            cursor_x += width;
        }
        break;
    }
    }
}

UIElement::UIElement(std::string name) : name_(std::move(name)) {}

bool UIElement::add_child(const Ptr& child) {
    if (!child || child.get() == this) {
        return false;
    }

    children_.push_back(child);
    child->parent_ = shared_from_this();
    return true;
}

bool UIElement::remove_child(const Ptr& child) {
    const auto it = std::find(children_.begin(), children_.end(), child);
    if (it == children_.end()) {
        return false;
    }

    (*it)->parent_.reset();
    children_.erase(it);
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

void UIElement::set_desired_size(const Size& desired_size) {
    desired_size_ = desired_size;
    mark_dirty();
}

Size UIElement::desired_size() const {
    return desired_size_;
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

void UIElement::set_event_handler(EventHandler handler) {
    event_handler_ = std::move(handler);
}

void UIElement::dispatch_event(InputEvent& event) {
    std::vector<std::shared_ptr<UIElement>> path;
    auto current = parent();
    while (current) {
        path.push_back(current);
        current = current->parent();
    }

    for (auto it = path.rbegin(); it != path.rend() && !event.handled; ++it) {
        (*it)->handle_event(event, EventPhase::Capture);
    }

    if (event.handled) {
        return;
    }

    handle_event(event, EventPhase::Target);

    if (event.handled) {
        return;
    }

    for (const auto& ancestor : path) {
        ancestor->handle_event(event, EventPhase::Bubble);
        if (event.handled) {
            return;
        }
    }
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

}  // namespace dcompframe
