#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <optional>

#include "dcompframe/events/input_event.h"
#include "dcompframe/ui_element.h"

namespace dcompframe {

class InputManager {
public:
    using ClickHandler = std::function<void(UIElement&)>;
    using DoubleClickHandler = std::function<void(UIElement&)>;
    using DragHandler = std::function<void(UIElement&, Point)>;
    using ShortcutHandler = std::function<void()>;

    void set_focus_ring_root(const std::shared_ptr<UIElement>& root);
    void focus_next();

    void on_mouse_down(const std::shared_ptr<UIElement>& target, Point position);
    void on_mouse_move(Point position);
    void on_mouse_up(Point position);

    void set_click_handler(ClickHandler handler);
    void set_double_click_handler(DoubleClickHandler handler);
    void set_drag_handler(DragHandler handler);
    void register_shortcut(int virtual_key, bool ctrl, bool alt, bool shift, ShortcutHandler handler);
    bool on_key_down(int virtual_key, bool ctrl, bool alt, bool shift);

    [[nodiscard]] std::shared_ptr<UIElement> focused_element() const;

private:
    static std::uint32_t make_shortcut_key(int virtual_key, bool ctrl, bool alt, bool shift);

    std::shared_ptr<UIElement> focus_root_ {};
    std::shared_ptr<UIElement> focused_ {};
    std::shared_ptr<UIElement> drag_target_ {};
    std::optional<Point> drag_start_ {};
    std::chrono::steady_clock::time_point last_click_time_ {};
    std::shared_ptr<UIElement> last_click_target_ {};
    ClickHandler click_handler_ {};
    DoubleClickHandler double_click_handler_ {};
    DragHandler drag_handler_ {};
    std::unordered_map<std::uint32_t, ShortcutHandler> shortcuts_ {};
};

}  // namespace dcompframe
