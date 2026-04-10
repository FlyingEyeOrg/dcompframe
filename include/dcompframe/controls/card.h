#pragma once
#include <optional>
#include <cstddef>
#include <chrono>

#include "dcompframe/ui_element.h"
#include "dcompframe/controls/controls_common.h"
#include "dcompframe/controls/styled_element.h"
#include "dcompframe/controls/button.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "dcompframe/binding/observable.h"

namespace dcompframe {


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
