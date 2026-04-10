#include "dcompframe/controls/style.h"

namespace dcompframe {

Theme Theme::make_dark() {
    Theme theme;
    theme.active_palette_ = "dark";
    theme.set_style("card", Style {.background = {25, 31, 40, 255}, .foreground = {235, 236, 240, 255}});
    theme.set_style("button.primary", Style {.background = {52, 102, 255, 255}, .foreground = {255, 255, 255, 255}});
    return theme;
}

Theme Theme::make_light() {
    Theme theme;
    theme.active_palette_ = "light";
    theme.set_style("card", Style {.background = {245, 246, 248, 255}, .foreground = {30, 30, 30, 255}});
    theme.set_style("button.primary", Style {.background = {28, 100, 242, 255}, .foreground = {255, 255, 255, 255}});
    return theme;
}

Theme Theme::make_brand() {
    Theme theme;
    theme.active_palette_ = "brand";
    theme.set_style("card", Style {.background = {20, 44, 64, 255}, .foreground = {237, 248, 255, 255}, .corner_radius = 16.0F});
    theme.set_style("button.primary", Style {.background = {0, 170, 166, 255}, .foreground = {255, 255, 255, 255}, .corner_radius = 18.0F});
    return theme;
}

void Theme::set_style(std::string key, Style style) {
    styles_[std::move(key)] = style;
}

Style Theme::resolve(const std::string& key) const {
    return styles_.at(key);
}

void Theme::set_active_palette(std::string palette_name) {
    active_palette_ = std::move(palette_name);
}

const std::string& Theme::active_palette() const {
    return active_palette_;
}

}  // namespace dcompframe
