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
#include <utility>
#include "dcompframe/binding/observable.h"

namespace dcompframe {


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

}  // namespace dcompframe
