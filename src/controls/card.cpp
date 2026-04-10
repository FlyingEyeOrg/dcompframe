#include "dcompframe/controls/card.h"
#include <algorithm>
#include <cmath>
#include <windows.h>

namespace dcompframe {


Card::Card() : StyledElement("card") {}

void Card::set_title(std::string title) {
    title_ = std::move(title);
    mark_dirty();
}

void Card::set_body(std::string body) {
    body_ = std::move(body);
    mark_dirty();
}

void Card::set_icon(std::string icon) {
    icon_ = std::move(icon);
    mark_dirty();
}

void Card::set_tags(std::vector<std::string> tags) {
    tags_ = std::move(tags);
    mark_dirty();
}

const std::string& Card::title() const {
    return title_;
}

const std::string& Card::body() const {
    return body_;
}

const std::string& Card::icon() const {
    return icon_;
}

const std::vector<std::string>& Card::tags() const {
    return tags_;
}

void Card::set_primary_action(std::shared_ptr<Button> action) {
    if (primary_action_) {
        remove_child(primary_action_);
    }

    primary_action_ = std::move(action);
    if (primary_action_) {
        add_child(primary_action_);
    }

    mark_dirty();
}

std::shared_ptr<Button> Card::primary_action() const {
    return primary_action_;
}

void Card::bind(BindingContext& context) {
    if (title_binding_id_ != 0) {
        context.title.unbind(title_binding_id_);
    }
    if (body_binding_id_ != 0) {

}  // namespace dcompframe
