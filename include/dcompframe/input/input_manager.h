#pragma once

#include <chrono>
#include <functional>
#include <optional>

#include "dcompframe/events/input_event.h"
#include "dcompframe/ui_element.h"

namespace dcompframe {

class InputManager {
public:
    using ClickHandler = std::function<void(UIElement&)>;
    using DoubleClickHandler = std::function<void(UIElement&)>;
    using DragHandler = std::function<void(UIElement&, Point)>;
    using LongPressHandler = std::function<void(UIElement&)>;

    void set_focus_ring_root(const std::shared_ptr<UIElement>& root);
    void focus_next();

    void on_mouse_down(const std::shared_ptr<UIElement>& target, Point position);
    void on_mouse_move(Point position);
    void on_mouse_up(Point position);
    void tick(std::chrono::milliseconds hold_threshold = std::chrono::milliseconds {500});

    void set_click_handler(ClickHandler handler);
    void set_double_click_handler(DoubleClickHandler handler);
    void set_drag_handler(DragHandler handler);
    void set_long_press_handler(LongPressHandler handler);

    [[nodiscard]] std::shared_ptr<UIElement> focused_element() const;

private:
    std::shared_ptr<UIElement> focus_root_ {};
    std::shared_ptr<UIElement> focused_ {};
    std::shared_ptr<UIElement> drag_target_ {};
    std::optional<Point> drag_start_ {};
    std::chrono::steady_clock::time_point last_click_time_ {};
    std::shared_ptr<UIElement> last_click_target_ {};
    std::chrono::steady_clock::time_point press_started_at_ {};
    std::shared_ptr<UIElement> pressed_target_ {};
    bool long_press_fired_ = false;
    ClickHandler click_handler_ {};
    DoubleClickHandler double_click_handler_ {};
    DragHandler drag_handler_ {};
    LongPressHandler long_press_handler_ {};
};

}  // namespace dcompframe
