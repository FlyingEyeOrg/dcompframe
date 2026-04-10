#include <fmt/core.h>
#include <fmt/format.h>

#include <filesystem>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <windows.h>

#include "dcompframe/config/app_config.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/render_manager.h"
#include "dcompframe/tools/window_render_target.h"
#include "dcompframe/window_host.h"

namespace {

class DemoApplication;

struct DemoWindow {
    DemoWindow(dcompframe::RenderManager* render_manager, DemoApplication* application, std::size_t window_id)
        : render_target(render_manager, &host), app(application), id(window_id) {}

    bool initialize(const dcompframe::AppConfig& config, DemoApplication* application);
    bool render_if_requested();

    dcompframe::WindowHost host;
    dcompframe::WindowRenderTarget render_target;
    dcompframe::BindingContext binding_context;
    std::shared_ptr<dcompframe::Button> primary_button = std::make_shared<dcompframe::Button>("新建窗口");
    std::shared_ptr<dcompframe::TextBox> text_box = std::make_shared<dcompframe::TextBox>();
    std::shared_ptr<dcompframe::CheckBox> check_box = std::make_shared<dcompframe::CheckBox>();
    std::shared_ptr<dcompframe::ComboBox> combo_box = std::make_shared<dcompframe::ComboBox>();
    std::shared_ptr<dcompframe::Slider> slider = std::make_shared<dcompframe::Slider>();
    std::shared_ptr<dcompframe::ScrollViewer> scroll_viewer = std::make_shared<dcompframe::ScrollViewer>();
    DemoApplication* app = nullptr;
    std::size_t id = 0;
};

class DemoApplication {
public:
    bool initialize() {
        const auto config_status = dcompframe::AppConfigLoader::load_from_file("demo/demo-config.json", config_);
        if (!config_status.ok()) {
            fmt::print("Config load warning: {}\n", config_status.message);
        }

        const auto backend = config_.use_directx_backend ? dcompframe::RenderBackend::DirectX : dcompframe::RenderBackend::Simulated;
        if (!render_manager_.initialize_with_backend(backend)) {
            fmt::print("RenderManager initialization failed.\n");
            return false;
        }

        render_manager_.start_render_thread();
        render_manager_.resource_manager().register_resource("main-card-texture", dcompframe::ResourceType::Texture, 2048 * 2048 * 4);
        render_manager_.resource_manager().register_resource("ui-vertex-buffer", dcompframe::ResourceType::Buffer, 1024 * 64);
        return create_window();
    }

    bool create_window() {
        auto window = std::make_unique<DemoWindow>(&render_manager_, this, next_window_id_++);
        if (!window->initialize(config_, this)) {
            return false;
        }

        render_manager_.diagnostics().log(dcompframe::LogLevel::Info, fmt::format("Created demo window {}", window->id));
        windows_.push_back(std::move(window));
        return true;
    }

    int run() {
        int rendered_frames = 0;
        render_dirty_windows(rendered_frames);

        while (true) {
            prune_closed_windows();
            if (windows_.empty()) {
                break;
            }

            MSG message {};
            if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) {
                    prune_closed_windows();
                    if (windows_.empty()) {
                        break;
                    }
                    continue;
                }

                TranslateMessage(&message);
                DispatchMessageW(&message);
                render_dirty_windows(rendered_frames);
                continue;
            }

            if (!render_dirty_windows(rendered_frames)) {
                WaitMessage();
            }
        }

        const auto report_path = std::filesystem::path("build") / "diagnostics-report.json";
        render_manager_.diagnostics().export_report(report_path);
        const auto drained = render_manager_.drain_commands();

        fmt::print(
            "DCompFrame interactive demo finished. commits={}, resources={}, avg_frame_ms={:.2f}, p95_ms={:.2f}, cps={:.2f}\n",
            render_manager_.total_commit_count(),
            render_manager_.resource_manager().resource_count(),
            render_manager_.diagnostics().average_frame_ms(),
            render_manager_.diagnostics().frame_p95_ms(),
            render_manager_.diagnostics().commits_per_second());
        fmt::print("Rendered frames={}\n", rendered_frames);
        fmt::print("Drained render commands: {}\n", drained.size());
        fmt::print("Diagnostics report: {}\n", report_path.string());

        render_manager_.stop_render_thread();
        windows_.clear();
        return 0;
    }

    dcompframe::RenderManager& render_manager() {
        return render_manager_;
    }

private:
    bool render_dirty_windows(int& rendered_frames) {
        bool rendered_any = false;
        for (const auto& window : windows_) {
            if (window->render_if_requested()) {
                ++rendered_frames;
                rendered_any = true;
            }
        }
        return rendered_any;
    }

    void prune_closed_windows() {
        windows_.erase(
            std::remove_if(
                windows_.begin(),
                windows_.end(),
                [](const std::unique_ptr<DemoWindow>& window) {
                    return window == nullptr || !window->host.is_created();
                }),
            windows_.end());
    }

    dcompframe::AppConfig config_;
    dcompframe::RenderManager render_manager_;
    std::vector<std::unique_ptr<DemoWindow>> windows_;
    std::size_t next_window_id_ = 1;
};

bool DemoWindow::initialize(const dcompframe::AppConfig& config, DemoApplication* application) {
    app = application;
    if (!host.create(std::wstring(L"DCompFrame Demo - Window ") + std::to_wstring(id), config.width, config.height)) {
        fmt::print("Window creation failed for window {}.\n", id);
        return false;
    }

    text_box->set_placeholder("请输入窗口标题，支持复制粘贴、选区和键盘导航");
    text_box->bind_text(binding_context.title);
    binding_context.title.set(fmt::format("Window {}", id));

    check_box->set_checked((id % 2U) == 1U);
    combo_box->set_items({"Overview", "Diagnostics", "Editor", "Preview", "Settings", "Account", "Advanced Options"});
    combo_box->set_selected_index(id % combo_box->items().size());
    slider->set_range(0.0F, 100.0F);
    slider->set_step(5.0F);
    slider->set_value(25.0F + static_cast<float>((id - 1U) % 4U) * 20.0F);

    primary_button->set_on_click([this] {
        if (app != nullptr) {
            app->render_manager().diagnostics().log(dcompframe::LogLevel::Info, fmt::format("Create window requested from window {}", id));
        }
    });
    primary_button->bind_enabled(binding_context.enabled);
    binding_context.enabled.set(true);

    text_box->set_on_text_changed([this](const std::string& value) {
        if (app != nullptr) {
            app->render_manager().diagnostics().log(dcompframe::LogLevel::Info, fmt::format("Window {} title edited: {}", id, value));
        }
        host.request_render();
    });
    check_box->set_on_checked_changed([this](bool checked) {
        if (app != nullptr) {
            app->render_manager().diagnostics().log(dcompframe::LogLevel::Info, fmt::format("Window {} checkbox={}", id, checked));
        }
        host.request_render();
    });
    combo_box->set_on_selection_changed([this](std::optional<std::size_t> index, const std::string& value) {
        if (app != nullptr) {
            app->render_manager().diagnostics().log(
                dcompframe::LogLevel::Info,
                fmt::format("Window {} combo index={} value={}", id, index ? static_cast<int>(*index) : -1, value));
        }
        host.request_render();
    });
    slider->set_on_value_changed([this](float value) {
        if (app != nullptr) {
            app->render_manager().diagnostics().log(dcompframe::LogLevel::Info, fmt::format("Window {} slider={:.2f}", id, value));
        }
        host.request_render();
    });

    render_target.set_interactive_controls(dcompframe::WindowRenderTarget::InteractiveControls {
        .primary_button = primary_button,
        .text_box = text_box,
        .check_box = check_box,
        .combo_box = combo_box,
        .slider = slider,
        .scroll_viewer = scroll_viewer,
    });
    render_target.set_primary_action_handler([application] {
        if (application != nullptr) {
            application->create_window();
        }
    });
    if (!render_target.initialize()) {
        fmt::print("Render target initialization failed for window {}.\n", id);
        host.destroy();
        return false;
    }

    host.set_message_handler([this](UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& result) {
        return render_target.handle_window_message(msg, wparam, lparam, result);
    });
    host.set_visible(true);
    host.apply_dpi(144);
    host.request_render();
    return true;
}

bool DemoWindow::render_if_requested() {
    if (!host.is_created()) {
        return false;
    }

    if (!host.consume_redraw_request()) {
        return false;
    }

    return render_target.render_frame(true);
}

}  // namespace

int main() {
    DemoApplication application;
    if (!application.initialize()) {
        return 1;
    }

    return application.run();
}
