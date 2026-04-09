#include "dcompframe/controls/style.h"

namespace dcompframe {

void Theme::set_style(std::string key, Style style) {
    styles_[std::move(key)] = style;
}

Style Theme::resolve(const std::string& key, const Style& fallback) const {
    if (const auto it = styles_.find(key); it != styles_.end()) {
        return it->second;
    }

    return fallback;
}

}  // namespace dcompframe
