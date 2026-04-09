#include "dcompframe/input/input_manager.h"

#include <algorithm>

#include <windows.h>

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
