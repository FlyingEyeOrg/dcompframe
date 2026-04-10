#include "dcompframe/input/input_manager.h"

#include <algorithm>

#include <windows.h>

namespace dcompframe {

namespace {

void collect_focusable_elements(
    const std::shared_ptr<UIElement>& element,
    std::vector<std::shared_ptr<UIElement>>& focusable_elements) {
    if (!element) {
        return;
    }

    if (element->is_focusable()) {
        focusable_elements.push_back(element);
    }

    for (const auto& child : element->children()) {
        collect_focusable_elements(child, focusable_elements);
    }
}

void update_focused_element(
    const std::shared_ptr<UIElement>& next_focused,
    std::shared_ptr<UIElement>& focused) {
    if (focused == next_focused) {
        return;
    }

    if (focused) {
        focused->set_focused(false);
    }
    focused = next_focused;
    if (focused) {
        focused->set_focused(true);
    }
}

bool dispatch_pointer_event(
    const std::shared_ptr<UIElement>& target,
    EventType type,
    Point position) {
    if (!target) {
        return false;
    }

    InputEvent event {
        .type = type,
        .position = position,
    };
    target->dispatch_event(event);
    return event.handled;
}

}  // namespace

void InputManager::set_focus_ring_root(const std::shared_ptr<UIElement>& root) {
    focus_root_ = root;
    update_focused_element(nullptr, focused_);
}

void InputManager::focus_next() {
    if (!focus_root_) {
        return;
    }

    std::vector<std::shared_ptr<UIElement>> focusables;
    collect_focusable_elements(focus_root_, focusables);

    if (focusables.empty()) {
        return;
    }

    auto it = std::find(focusables.begin(), focusables.end(), focused_);
    if (it != focusables.end()) {
        ++it;
        if (it == focusables.end()) {
            it = focusables.begin();
        }
        update_focused_element(*it, focused_);
    } else {
        update_focused_element(focusables.front(), focused_);
    }
}

void InputManager::on_mouse_down(const std::shared_ptr<UIElement>& target, Point position) {
    if (!target) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = now - last_click_time_;
    if (last_click_target_ == target && elapsed < std::chrono::milliseconds(350)) {
        if (double_click_handler_) {
            double_click_handler_(*target);
        }
    } else if (click_handler_) {
        click_handler_(*target);
    }

    last_click_time_ = now;
    last_click_target_ = target;
    drag_target_ = target;
    drag_start_ = position;
}

void InputManager::on_mouse_move(Point position) {
    if (!drag_target_ || !drag_start_) {
        return;
    }

    const Point delta {.x = position.x - drag_start_->x, .y = position.y - drag_start_->y};
    if (drag_handler_) {
        drag_handler_(*drag_target_, delta);
    }
}

void InputManager::on_mouse_up(Point /*position*/) {
    drag_target_.reset();
    drag_start_.reset();
}

bool InputManager::route_pointer_down(const std::shared_ptr<UIElement>& root, Point position) {
    const auto target = hit_test(root, position);
    if (!target) {
        on_mouse_up(position);
        return false;
    }

    if (target->is_focusable()) {
        update_focused_element(target, focused_);
    }

    const bool handled = dispatch_pointer_event(target, EventType::MouseDown, position);
    if (!handled) {
        on_mouse_down(target, position);
    }
    return handled;
}

bool InputManager::route_pointer_move(const std::shared_ptr<UIElement>& root, Point position) {
    const auto target = drag_target_ ? drag_target_ : hit_test(root, position);
    const bool handled = dispatch_pointer_event(target, EventType::MouseMove, position);
    if (!handled) {
        on_mouse_move(position);
    }
    return handled;
}

bool InputManager::route_pointer_up(const std::shared_ptr<UIElement>& root, Point position) {
    const auto target = drag_target_ ? drag_target_ : hit_test(root, position);
    const bool handled = dispatch_pointer_event(target, EventType::MouseUp, position);
    if (!handled) {
        on_mouse_up(position);
    }
    return handled;
}

std::shared_ptr<UIElement> InputManager::hit_test(const std::shared_ptr<UIElement>& root, Point position) const {
    if (!root) {
        return nullptr;
    }

    return root->hit_test(position);
}

void InputManager::set_click_handler(ClickHandler handler) {
    click_handler_ = std::move(handler);
}

void InputManager::set_double_click_handler(DoubleClickHandler handler) {
    double_click_handler_ = std::move(handler);
}

void InputManager::set_drag_handler(DragHandler handler) {
    drag_handler_ = std::move(handler);
}

void InputManager::register_shortcut(int virtual_key, bool ctrl, bool alt, bool shift, ShortcutHandler handler) {
    shortcuts_[make_shortcut_key(virtual_key, ctrl, alt, shift)] = std::move(handler);
}

bool InputManager::on_key_down(int virtual_key, bool ctrl, bool alt, bool shift) {
    const auto it = shortcuts_.find(make_shortcut_key(virtual_key, ctrl, alt, shift));
    if (it == shortcuts_.end()) {
        return false;
    }

    if (it->second) {
        it->second();
    }
    return true;
}

std::shared_ptr<UIElement> InputManager::focused_element() const {
    return focused_;
}

std::uint32_t InputManager::make_shortcut_key(int virtual_key, bool ctrl, bool alt, bool shift) {
    std::uint32_t key = static_cast<std::uint32_t>(virtual_key & 0xFFFF);
    if (ctrl) {
        key |= (1U << 16U);
    }
    if (alt) {
        key |= (1U << 17U);
    }
    if (shift) {
        key |= (1U << 18U);
    }
    return key;
}

}  // namespace dcompframe
