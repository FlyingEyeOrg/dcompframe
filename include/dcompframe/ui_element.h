#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "dcompframe/events/input_event.h"
#include "dcompframe/geometry.h"

namespace dcompframe {

class UIElement : public std::enable_shared_from_this<UIElement> {
public:
    using Ptr = std::shared_ptr<UIElement>;
    using EventHandler = std::function<void(InputEvent&, EventPhase)>;

    explicit UIElement(std::string name = "");
    virtual ~UIElement() = default;

    bool add_child(const Ptr& child);
    bool remove_child(const Ptr& child);

    [[nodiscard]] const std::vector<Ptr>& children() const;
    [[nodiscard]] std::shared_ptr<UIElement> parent() const;

    void set_bounds(const Rect& bounds);
    [[nodiscard]] Rect bounds() const;

    void set_desired_size(const Size& desired_size);
    [[nodiscard]] Size desired_size() const;

    void set_event_handler(EventHandler handler);
    void dispatch_event(InputEvent& event);

    [[nodiscard]] const std::string& name() const;

protected:
    void handle_event(InputEvent& event, EventPhase phase) const;

private:
    std::string name_;
    std::weak_ptr<UIElement> parent_;
    std::vector<Ptr> children_;
    Rect bounds_ {};
    Size desired_size_ {};
    EventHandler event_handler_ {};
};

}  // namespace dcompframe
