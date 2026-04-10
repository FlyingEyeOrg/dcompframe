#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "dcompframe/events/input_event.h"
#include "dcompframe/geometry.h"

namespace dcompframe {

enum class LayoutStrategy {
    Stack,
    Grid,
    Flex
};

enum class LayoutAxisAlignment {
    Start,
    Center,
    End,
    Stretch
};

enum class FlexDirection {
    Row,
    RowReverse,
    Column,
    ColumnReverse
};

enum class FlexWrap {
    NoWrap,
    Wrap,
    WrapReverse
};

enum class FlexJustifyContent {
    Start,
    End,
    Center,
    SpaceBetween,
    SpaceAround
};

enum class FlexAlignItems {
    Start,
    End,
    Center,
    Stretch
};

enum class FlexAlignContent {
    Start,
    End,
    Center,
    SpaceBetween,
    SpaceAround,
    Stretch
};

enum class FlexAlignSelf {
    Auto,
    Start,
    End,
    Center,
    Stretch
};

struct Thickness {
    float left = 0.0F;
    float top = 0.0F;
    float right = 0.0F;
    float bottom = 0.0F;
};

class LayoutManager {
public:
    void set_strategy(LayoutStrategy strategy);
    void apply_layout(const std::shared_ptr<class UIElement>& root, const Size& available_size) const;

private:
    LayoutStrategy strategy_ = LayoutStrategy::Grid;
};

class UIElement : public std::enable_shared_from_this<UIElement> {
public:
    using Ptr = std::shared_ptr<UIElement>;
    using EventHandler = std::function<void(InputEvent&, EventPhase)>;

    explicit UIElement(std::string name = "");
    virtual ~UIElement() = default;

    virtual Size measure(const Size& available_size);
    virtual void arrange(const Size& available_size);

    bool add_child(const Ptr& child);
    bool remove_child(const Ptr& child);

    [[nodiscard]] const std::vector<Ptr>& children() const;
    [[nodiscard]] std::shared_ptr<UIElement> parent() const;

    void set_bounds(const Rect& bounds);
    [[nodiscard]] Rect bounds() const;
    [[nodiscard]] Rect absolute_bounds() const;

    void set_desired_size(const Size& desired_size);
    [[nodiscard]] Size desired_size() const;
    [[nodiscard]] Size measured_size() const;

    void set_flex_grow(float flex_grow);
    [[nodiscard]] float flex_grow() const;
    void set_flex_shrink(float flex_shrink);
    [[nodiscard]] float flex_shrink() const;
    void set_flex_basis(float flex_basis);
    [[nodiscard]] float flex_basis() const;
    [[nodiscard]] bool has_flex_basis() const;
    void set_order(int order);
    [[nodiscard]] int order() const;
    void set_align_self(FlexAlignSelf align_self);
    [[nodiscard]] FlexAlignSelf align_self() const;

    void set_opacity(float opacity);
    [[nodiscard]] float opacity() const;

    void set_clip_rect(const Rect& clip_rect);
    [[nodiscard]] Rect clip_rect() const;

    void set_margin(const Thickness& margin);
    [[nodiscard]] Thickness margin() const;

    void set_transform(float translate_x, float translate_y, float scale_x, float scale_y, float rotation_deg);
    [[nodiscard]] Point translation() const;
    [[nodiscard]] Point scale() const;
    [[nodiscard]] float rotation_deg() const;

    void set_focusable(bool focusable);
    [[nodiscard]] bool is_focusable() const;
    void set_focused(bool focused);
    [[nodiscard]] bool is_focused() const;
    void set_hit_test_visible(bool hit_test_visible);
    [[nodiscard]] bool hit_test_visible() const;

    void set_event_handler(EventHandler handler);
    void dispatch_event(InputEvent& event);
    [[nodiscard]] bool contains_point(const Point& position) const;
    [[nodiscard]] std::shared_ptr<UIElement> hit_test(const Point& position);

    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] bool is_dirty() const;
    void clear_dirty_recursive();

protected:
    void handle_event(InputEvent& event, EventPhase phase) const;
    void mark_dirty();
    void set_measured_size(const Size& measured_size);
    [[nodiscard]] Size clamp_size_to_available(const Size& proposed_size, const Size& available_size) const;

private:
    std::string name_;
    std::weak_ptr<UIElement> parent_;
    std::vector<Ptr> children_;
    Rect bounds_ {};
    Size desired_size_ {};
    Size measured_size_ {};
    Rect clip_rect_ {};
    Thickness margin_ {};
    Point translation_ {};
    Point scale_ {1.0F, 1.0F};
    float rotation_deg_ = 0.0F;
    float flex_grow_ = 0.0F;
    float flex_shrink_ = 1.0F;
    float flex_basis_ = -1.0F;
    int order_ = 0;
    FlexAlignSelf align_self_ = FlexAlignSelf::Auto;
    float opacity_ = 1.0F;
    bool focusable_ = false;
    bool focused_ = false;
    bool hit_test_visible_ = true;
    bool dirty_ = true;
    EventHandler event_handler_ {};
};

}  // namespace dcompframe
