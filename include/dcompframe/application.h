#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "dcompframe/config/app_config.h"
#include "dcompframe/render_manager.h"
#include "dcompframe/tools/window_render_target.h"
#include "dcompframe/ui_element.h"
#include "dcompframe/window_host.h"

namespace dcompframe {

class Application;

class Window {
public:
    Window(RenderManager* render_manager, Application* application, std::size_t window_id);
    virtual ~Window() = default;

    bool initialize(const AppConfig& config);
    bool render_if_requested();
    [[nodiscard]] bool needs_continuous_rendering() const;
    [[nodiscard]] std::size_t id() const;
    [[nodiscard]] WindowHost& host();
    [[nodiscard]] WindowRenderTarget& render_target();
    [[nodiscard]] Application* application() const;

protected:
    virtual bool build(const AppConfig& config);
    virtual void arrange_content(const Size& client_size);
    [[nodiscard]] virtual std::wstring title() const;

    void set_root(const std::shared_ptr<UIElement>& root);
    void set_arrange_handler(std::function<void(const Size&)> handler);
    void set_interactive_controls(WindowRenderTarget::InteractiveControls controls);
    void set_primary_action_handler(std::function<void()> handler);
    [[nodiscard]] RenderManager* render_manager() const;

private:
    WindowHost host_ {};
    WindowRenderTarget render_target_;
    std::shared_ptr<UIElement> root_ {};
    std::function<void(const Size&)> arrange_handler_ {};
    Application* app_ = nullptr;
    std::size_t id_ = 0;
};

class Application {
public:
    using WindowFactory = std::function<std::unique_ptr<Window>(RenderManager*, Application*, std::size_t)>;

    ~Application();

    bool initialize(std::string config_path = "demo/demo-config.json");
    void set_window_factory(WindowFactory factory);
    bool create_window();
    int run();

    [[nodiscard]] RenderManager& render_manager();
    [[nodiscard]] const AppConfig& config() const;

private:
    bool render_dirty_windows(int& rendered_frames);
    bool request_animation_frames_if_due();
    void prune_closed_windows();

    AppConfig config_ {};
    RenderManager render_manager_ {};
    std::vector<std::unique_ptr<Window>> windows_ {};
    std::size_t next_window_id_ = 1;
    std::chrono::steady_clock::time_point last_animation_tick_ = std::chrono::steady_clock::now();
    WindowFactory window_factory_ {};
};

}  // namespace dcompframe
