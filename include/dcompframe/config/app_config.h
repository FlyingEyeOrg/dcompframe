#pragma once

#include <string>

#include "dcompframe/controls/style.h"
#include "dcompframe/errors.h"

namespace dcompframe {

struct AppConfig {
    int width = 1280;
    int height = 720;
    bool use_directx_backend = false;
    std::string theme_name = "dark";
    Style card_style {};
};

class AppConfigLoader {
public:
    static Status load_from_file(const std::string& path, AppConfig& config);
};

}  // namespace dcompframe
