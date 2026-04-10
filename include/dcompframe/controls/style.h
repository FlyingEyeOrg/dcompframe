#pragma once
#include <optional>
#include <cstddef>
#include <chrono>

#include <string>
#include <unordered_map>

namespace dcompframe {

struct Color {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 255;

    [[nodiscard]] bool operator==(const Color& other) const = default;
};

struct Style {
    Color background {30, 30, 30, 255};
    Color foreground {230, 230, 230, 255};
    Color border {70, 70, 70, 255};
    float border_thickness = 1.0F;
    float corner_radius = 6.0F;
};

class Theme {
public:
    static Theme make_dark();
    static Theme make_light();
    static Theme make_brand();

    void set_style(std::string key, Style style);
    [[nodiscard]] Style resolve(const std::string& key) const;

    void set_active_palette(std::string palette_name);
    [[nodiscard]] const std::string& active_palette() const;

private:
    std::unordered_map<std::string, Style> styles_;
    std::string active_palette_ = "dark";
};

}  // namespace dcompframe
