#include "dcompframe/ui_element.h"

#include <algorithm>

namespace dcompframe {

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
}

Rect UIElement::bounds() const {
    return bounds_;
}

void UIElement::set_desired_size(const Size& desired_size) {
    desired_size_ = desired_size;
}

Size UIElement::desired_size() const {
    return desired_size_;
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

void UIElement::handle_event(InputEvent& event, EventPhase phase) const {
    if (event_handler_) {
        event_handler_(event, phase);
    }
}

}  // namespace dcompframe
