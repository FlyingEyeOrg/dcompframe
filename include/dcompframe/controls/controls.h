#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "dcompframe/binding/observable.h"
#include "dcompframe/controls/style.h"
#include "dcompframe/ui_element.h"

namespace dcompframe {

enum class ControlState {
    Normal,
    Hovered,
    Pressed,
    Disabled,
    Selected
};

enum class TextHorizontalAlignment {
    Left,
    Center,
    Right
};

enum class TextVerticalAlignment {
    Top,
    Center,
    Bottom
};

class StyledElement : public UIElement {
public:
    explicit StyledElement(std::string name);

    void set_style(Style style);
    [[nodiscard]] Style style() const;

    void set_state(ControlState state);
    [[nodiscard]] ControlState state() const;

    void set_text_alignment(TextHorizontalAlignment horizontal, TextVerticalAlignment vertical);
    [[nodiscard]] TextHorizontalAlignment text_horizontal_alignment() const;
    [[nodiscard]] TextVerticalAlignment text_vertical_alignment() const;

private:
    Style style_ {};
    ControlState state_ = ControlState::Normal;
    TextHorizontalAlignment text_horizontal_alignment_ = TextHorizontalAlignment::Center;
    TextVerticalAlignment text_vertical_alignment_ = TextVerticalAlignment::Center;
};

class Panel : public StyledElement {
public:
    Panel();

    void arrange(const Size& available_size);
};

class TextBlock : public StyledElement {
public:
    explicit TextBlock(std::string text = "");

    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const;

private:
    std::string text_;
};

class Image : public StyledElement {
public:
    Image();

    void set_source(std::string source);
    [[nodiscard]] const std::string& source() const;

private:
    std::string source_;
};

class Button : public StyledElement {
public:
    explicit Button(std::string text = "");

    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const;

    void set_on_click(std::function<void()> callback);
    bool click();
    void bind_enabled(Observable<bool>& observable);

private:
    std::string text_;
    std::function<void()> on_click_ {};
    int enabled_binding_id_ = 0;
};

class TextBox : public StyledElement {
public:
    using TextChangedHandler = std::function<void(const std::string&)>;

    TextBox();

    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const;

    void set_placeholder(std::string placeholder);
    [[nodiscard]] const std::string& placeholder() const;

    void set_selection(std::size_t start, std::size_t end);
    [[nodiscard]] std::pair<std::size_t, std::size_t> selection() const;
    [[nodiscard]] bool has_selection() const;
    void clear_selection();
    void select_all();

    void set_caret_position(std::size_t position, bool extend_selection = false);
    [[nodiscard]] std::size_t caret_position() const;
    void move_caret_left(bool extend_selection = false);
    void move_caret_right(bool extend_selection = false);
    void move_caret_home(bool extend_selection = false);
    void move_caret_end(bool extend_selection = false);

    bool insert_text(std::string text);
    bool backspace();
    bool delete_forward();

    void set_composition_text(std::string text);
    [[nodiscard]] const std::string& composition_text() const;
    void commit_composition();

    void bind_text(Observable<std::string>& observable);
    void set_on_text_changed(TextChangedHandler handler);

private:
    void replace_selection_with(const std::string& replacement);
    void clamp_selection();
    void notify_text_changed();

    std::string text_;
    std::string placeholder_;
    std::size_t selection_start_ = 0;
    std::size_t selection_end_ = 0;
    std::size_t caret_position_ = 0;
    std::string composition_text_;
    int text_binding_id_ = 0;
    Observable<std::string>* bound_text_ = nullptr;
    bool updating_from_binding_ = false;
    TextChangedHandler on_text_changed_ {};
};

class RichTextBox : public StyledElement {
public:
    RichTextBox();

    void set_rich_text(std::string rich_text);
    [[nodiscard]] const std::string& rich_text() const;

private:
    std::string rich_text_;
};

struct ListGroup {
    std::string name;
    std::vector<std::string> items;
};

class ListView : public StyledElement {
public:
    ListView();

    void set_items(std::vector<std::string> items);
    [[nodiscard]] const std::vector<std::string>& items() const;
    void set_selected_index(std::size_t index);
    [[nodiscard]] std::optional<std::size_t> selected_index() const;

    void set_groups(std::vector<ListGroup> groups);
    [[nodiscard]] const std::vector<ListGroup>& groups() const;
    [[nodiscard]] std::pair<std::size_t, std::size_t> visible_range(float scroll_offset, float viewport_height, float item_height) const;

private:
    std::vector<std::string> items_;
    std::vector<ListGroup> groups_;
    std::optional<std::size_t> selected_index_ {};
};

class ScrollViewer : public StyledElement {
public:
    ScrollViewer();

    void set_scroll_offset(float x, float y);
    [[nodiscard]] Point scroll_offset() const;
    void set_inertia_velocity(float x, float y);
    void tick_inertia(std::chrono::milliseconds delta_time, float deceleration = 0.0015F);
    [[nodiscard]] Point inertia_velocity() const;

private:
    Point offset_ {};
    Point velocity_ {};
};

class CheckBox : public StyledElement {
public:
    using CheckedChangedHandler = std::function<void(bool)>;

    CheckBox();

    void set_checked(bool checked);
    [[nodiscard]] bool checked() const;
    bool toggle();
    void set_on_checked_changed(CheckedChangedHandler handler);

private:
    bool checked_ = false;
    CheckedChangedHandler on_checked_changed_ {};
};

class ComboBox : public StyledElement {
public:
    using SelectionChangedHandler = std::function<void(std::optional<std::size_t>, const std::string&)>;

    ComboBox();

    void set_items(std::vector<std::string> items);
    [[nodiscard]] const std::vector<std::string>& items() const;

    void set_selected_index(std::size_t index);
    [[nodiscard]] std::optional<std::size_t> selected_index() const;
    [[nodiscard]] std::string selected_text() const;
    void open_dropdown();
    void close_dropdown();
    void toggle_dropdown();
    [[nodiscard]] bool is_dropdown_open() const;
    bool select_next();
    bool select_previous();
    void set_on_selection_changed(SelectionChangedHandler handler);

private:
    void notify_selection_changed();

    std::vector<std::string> items_;
    std::optional<std::size_t> selected_index_ {};
    bool dropdown_open_ = false;
    SelectionChangedHandler on_selection_changed_ {};
};

class Slider : public StyledElement {
public:
    using ValueChangedHandler = std::function<void(float)>;

    Slider();

    void set_range(float min_value, float max_value);
    void set_value(float value);
    [[nodiscard]] float value() const;
    [[nodiscard]] float min_value() const;
    [[nodiscard]] float max_value() const;
    void set_step(float step);
    [[nodiscard]] float step() const;
    [[nodiscard]] float normalized_value() const;
    void set_value_from_ratio(float ratio);
    bool step_by(float delta_steps);
    void set_on_value_changed(ValueChangedHandler handler);

private:
    float min_ = 0.0F;
    float max_ = 100.0F;
    float value_ = 0.0F;
    float step_ = 1.0F;
    ValueChangedHandler on_value_changed_ {};
};

class Card : public StyledElement {
public:
    Card();

    void set_title(std::string title);
    void set_body(std::string body);
    void set_icon(std::string icon);
    void set_tags(std::vector<std::string> tags);

    [[nodiscard]] const std::string& title() const;
    [[nodiscard]] const std::string& body() const;
    [[nodiscard]] const std::string& icon() const;
    [[nodiscard]] const std::vector<std::string>& tags() const;

    void set_primary_action(std::shared_ptr<Button> action);
    [[nodiscard]] std::shared_ptr<Button> primary_action() const;
    void bind(BindingContext& context);

private:
    std::string title_;
    std::string body_;
    std::string icon_;
    std::vector<std::string> tags_;
    std::shared_ptr<Button> primary_action_ {};
    int title_binding_id_ = 0;
    int body_binding_id_ = 0;
};

}  // namespace dcompframe
