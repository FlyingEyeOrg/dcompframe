#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

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

class StyledElement : public UIElement {
public:
    explicit StyledElement(std::string name);

    void set_style(Style style);
    [[nodiscard]] Style style() const;

    void set_state(ControlState state);
    [[nodiscard]] ControlState state() const;

private:
    Style style_ {};
    ControlState state_ = ControlState::Normal;
};

class Panel : public StyledElement {
public:
    Panel();
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

private:
    std::string text_;
    std::function<void()> on_click_ {};
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

private:
    std::string title_;
    std::string body_;
    std::string icon_;
    std::vector<std::string> tags_;
    std::shared_ptr<Button> primary_action_ {};
};

}  // namespace dcompframe
