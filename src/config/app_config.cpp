#include "dcompframe/config/app_config.h"

#include <fstream>

#include <nlohmann/json.hpp>

namespace dcompframe {

Status AppConfigLoader::load_from_file(const std::string& path, AppConfig& config) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return Status::failure(ErrorCode::IOError, "Cannot open config file: " + path);
    }

    try {
        nlohmann::json json;
        file >> json;

        config.width = json.value("width", config.width);
        config.height = json.value("height", config.height);
        config.use_directx_backend = json.value("use_directx_backend", config.use_directx_backend);
        config.theme_name = json.value("theme_name", config.theme_name);

        if (json.contains("card_style")) {
            const auto& style = json["card_style"];
            config.card_style.corner_radius = style.value("corner_radius", config.card_style.corner_radius);
            config.card_style.border_thickness = style.value("border_thickness", config.card_style.border_thickness);
        }

        return Status::success();
    } catch (const std::exception& ex) {
        return Status::failure(ErrorCode::ParseError, ex.what());
    }
}

}  // namespace dcompframe
