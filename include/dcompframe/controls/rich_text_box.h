#pragma once
#include <optional>
#include <cstddef>
#include <chrono>

#include "dcompframe/ui_element.h"
#include "dcompframe/controls/controls_common.h"
#include "dcompframe/controls/styled_element.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "dcompframe/binding/observable.h"

namespace dcompframe {


class RichTextBox : public StyledElement {
public:
    RichTextBox();

    void set_rich_text(std::string rich_text);
    [[nodiscard]] const std::string& rich_text() const;

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

    void set_scroll_offset(float scroll_offset);
    [[nodiscard]] float scroll_offset() const;
    void scroll_by(float delta);

private:
    void replace_selection_with(const std::string& replacement);
    void clamp_selection();

    std::string rich_text_;
    std::size_t selection_start_ = 0;
    std::size_t selection_end_ = 0;
    std::size_t caret_position_ = 0;
    float scroll_offset_ = 0.0F;
};

}  // namespace dcompframe
