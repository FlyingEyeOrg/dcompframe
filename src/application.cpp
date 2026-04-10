#include "dcompframe/application.h"

#include <algorithm>
#include <filesystem>

#include <fmt/core.h>

namespace dcompframe {

Window::Window(RenderManager* render_manager, Application* application, std::size_t window_id)
    : render_target_(render_manager, &host_), app_(application), id_(window_id) {}

bool Window::initialize(const AppConfig& config) {
    if (!host_.create(title(), config.width, config.height)) {
        fmt::print("Window creation failed for window {}.\n", id_);
        return false;
    }

    if (!build(config)) {
        host_.destroy();
        return false;
    }

    if (!render_target_.initialize()) {
        fmt::print("Render target initialization failed for window {}.\n", id_);
        host_.destroy();
        return false;
    }

    host_.set_message_handler([this](UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& result) {
        return render_target_.handle_window_message(msg, wparam, lparam, result);
    });
    host_.set_visible(true);
    host_.apply_dpi(144);
    host_.request_render();
    return true;
}

bool Window::render_if_requested() {
    if (!host_.is_created()) {
        return false;
    }

    if (!host_.consume_redraw_request()) {
        return false;
    }

    arrange_content(host_.client_size());

    return render_target_.render_frame(true);
}

bool Window::needs_continuous_rendering() const {
    return render_target_.needs_continuous_rendering();
}

std::size_t Window::id() const {
    return id_;
}

WindowHost& Window::host() {
    return host_;
}

WindowRenderTarget& Window::render_target() {
    return render_target_;
}

Application* Window::application() const {
    return app_;
}

bool Window::build(const AppConfig& config) {
    (void)config;
    return true;
}

void Window::arrange_content(const Size& client_size) {
    if (arrange_handler_) {
        arrange_handler_(client_size);
        return;
    }

    if (root_ != nullptr) {
        root_->set_bounds(Rect {.x = 0.0F, .y = 0.0F, .width = client_size.width, .height = client_size.height});
    }
}

std::wstring Window::title() const {
    return std::wstring(L"DCompFrame Window ") + std::to_wstring(id_);
}

void Window::set_root(const std::shared_ptr<UIElement>& root) {
    root_ = root;
}

void Window::set_arrange_handler(std::function<void(const Size&)> handler) {
    arrange_handler_ = std::move(handler);
}

void Window::set_interactive_controls(WindowRenderTarget::InteractiveControls controls) {
    render_target_.set_interactive_controls(std::move(controls));
}

void Window::set_primary_action_handler(std::function<void()> handler) {
    render_target_.set_primary_action_handler(std::move(handler));
}

RenderManager* Window::render_manager() const {
    return app_ != nullptr ? &app_->render_manager() : nullptr;
}

Application::~Application() {
    windows_.clear();
    render_manager_.stop_render_thread();
    if (render_manager_.is_initialized()) {
        render_manager_.shutdown();
    }
}

bool Application::initialize(std::string config_path) {
    const auto config_status = AppConfigLoader::load_from_file(config_path, config_);
    if (!config_status.ok()) {
        fmt::print("Config load warning: {}\n", config_status.message);
    }

    const auto backend = config_.use_directx_backend ? RenderBackend::DirectX : RenderBackend::Simulated;
    if (!render_manager_.initialize_with_backend(backend)) {
        fmt::print("RenderManager initialization failed.\n");
        return false;
    }

    render_manager_.start_render_thread();
    return true;
}

void Application::set_window_factory(WindowFactory factory) {
    window_factory_ = std::move(factory);
}

bool Application::create_window() {
    std::unique_ptr<Window> window;
    if (window_factory_) {
        window = window_factory_(&render_manager_, this, next_window_id_++);
    } else {
        window = std::make_unique<Window>(&render_manager_, this, next_window_id_++);
    }

    if (!window->initialize(config_)) {
        return false;
    }

    render_manager_.diagnostics().log(LogLevel::Info, fmt::format("Created demo window {}", window->id()));
    windows_.push_back(std::move(window));
    return true;
}

int Application::run() {
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

        request_animation_frames_if_due();
        if (!render_dirty_windows(rendered_frames)) {
            MsgWaitForMultipleObjectsEx(0, nullptr, 8U, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
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

RenderManager& Application::render_manager() {
    return render_manager_;
}

const AppConfig& Application::config() const {
    return config_;
}

bool Application::render_dirty_windows(int& rendered_frames) {
    bool rendered_any = false;
    for (const auto& window : windows_) {
        if (window->render_if_requested()) {
            ++rendered_frames;
            rendered_any = true;
        }
    }
    return rendered_any;
}

bool Application::request_animation_frames_if_due() {
    bool has_active_window = false;
    for (const auto& window : windows_) {
        if (window != nullptr && window->needs_continuous_rendering()) {
            has_active_window = true;
            break;
        }
    }

    if (!has_active_window) {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    if (now - last_animation_tick_ < std::chrono::milliseconds(16)) {
        return false;
    }

    last_animation_tick_ = now;
    for (const auto& window : windows_) {
        if (window != nullptr && window->needs_continuous_rendering() && window->host().is_created()) {
            window->host().request_render();
        }
    }
    return true;
}

void Application::prune_closed_windows() {
    windows_.erase(
        std::remove_if(
            windows_.begin(),
            windows_.end(),
            [](const std::unique_ptr<Window>& window) {
                return window == nullptr || !window->host().is_created();
            }),
        windows_.end());
}

}  // namespace dcompframe
