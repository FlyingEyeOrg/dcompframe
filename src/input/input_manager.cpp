#include "dcompframe/input/input_manager.h"

#include <algorithm>

namespace dcompframe {

void InputManager::set_focus_ring_root(const std::shared_ptr<UIElement>& root) {
    focus_root_ = root;
    focused_.reset();
}

void InputManager::focus_next() {
    if (!focus_root_) {
        return;
    }

    std::vector<std::shared_ptr<UIElement>> focusables;
    for (const auto& child : focus_root_->children()) {
        if (child->is_focusable()) {
            focusables.push_back(child);
        }
    }

    if (focusables.empty()) {
        return;
    }

    auto it = std::find(focusables.begin(), focusables.end(), focused_);
    if (it != focusables.end()) {
        (*it)->set_focused(false);
        ++it;
        if (it == focusables.end()) {
            it = focusables.begin();
        }
        focused_ = *it;
    } else {
        focused_ = focusables.front();
    }

    focused_->set_focused(true);
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
    press_started_at_ = now;
    pressed_target_ = target;
    long_press_fired_ = false;
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
    pressed_target_.reset();
    long_press_fired_ = false;
}

void InputManager::tick(std::chrono::milliseconds hold_threshold) {
    if (!pressed_target_ || long_press_fired_ || !long_press_handler_) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (now - press_started_at_ >= hold_threshold) {
        long_press_handler_(*pressed_target_);
        long_press_fired_ = true;
    }
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

void InputManager::set_long_press_handler(LongPressHandler handler) {
    long_press_handler_ = std::move(handler);
}

std::shared_ptr<UIElement> InputManager::focused_element() const {
    return focused_;
}

}  // namespace dcompframe
