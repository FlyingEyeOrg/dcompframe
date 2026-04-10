#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "dcompframe/binding/observable.h"
#include "dcompframe/config/app_config.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/layout/grid_panel.h"
#include "dcompframe/render_manager.h"
#include "dcompframe/tools/window_render_target.h"
#include "dcompframe/window_host.h"

namespace dcompframe {

class Application;

class Window {
public:
    Window(RenderManager* render_manager, Application* application, std::size_t window_id);

    bool initialize(const AppConfig& config);
    bool render_if_requested();
    [[nodiscard]] bool needs_continuous_rendering() const;
    [[nodiscard]] std::size_t id() const;
    [[nodiscard]] WindowHost& host();

private:
    WindowHost host_ {};
    WindowRenderTarget render_target_;
    BindingContext binding_context_ {};
    std::shared_ptr<GridPanel> root_grid_ = std::make_shared<GridPanel>(12, 2);
    std::shared_ptr<Button> primary_button_ = std::make_shared<Button>("新建窗口");
    std::shared_ptr<TextBox> text_box_ = std::make_shared<TextBox>();
    std::shared_ptr<RichTextBox> rich_text_box_ = std::make_shared<RichTextBox>();
    std::shared_ptr<CheckBox> check_box_ = std::make_shared<CheckBox>();
    std::shared_ptr<ComboBox> combo_box_ = std::make_shared<ComboBox>();
    std::shared_ptr<Slider> slider_ = std::make_shared<Slider>();
    std::shared_ptr<ScrollViewer> scroll_viewer_ = std::make_shared<ScrollViewer>();
    std::shared_ptr<ItemsControl> items_control_ = std::make_shared<ItemsControl>();
    std::shared_ptr<ListView> list_view_ = std::make_shared<ListView>();
    std::shared_ptr<TextBlock> text_block_ = std::make_shared<TextBlock>("StackPanel 式纵向示例布局，标题固定，内容区独立滚动。");
    std::shared_ptr<Label> label_ = std::make_shared<Label>("状态：动画预览中");
    std::shared_ptr<Image> image_ = std::make_shared<Image>();
    std::shared_ptr<Progress> progress_ = std::make_shared<Progress>();
    std::shared_ptr<Loading> loading_ = std::make_shared<Loading>();
    std::shared_ptr<LogBox> log_box_ = std::make_shared<LogBox>();
    std::shared_ptr<TabControl> tab_control_ = std::make_shared<TabControl>();
    std::shared_ptr<Popup> popup_ = std::make_shared<Popup>();
    std::shared_ptr<Expander> expander_ = std::make_shared<Expander>();
    std::shared_ptr<Card> card_ = std::make_shared<Card>();
    Application* app_ = nullptr;
    std::size_t id_ = 0;
};

class Application {
public:
    bool initialize(std::string config_path = "demo/demo-config.json");
    bool create_window();
    int run();

    [[nodiscard]] RenderManager& render_manager();

private:
    bool render_dirty_windows(int& rendered_frames);
    bool request_animation_frames_if_due();
    void prune_closed_windows();

    AppConfig config_ {};
    RenderManager render_manager_ {};
    std::vector<std::unique_ptr<Window>> windows_ {};
    std::size_t next_window_id_ = 1;
    std::chrono::steady_clock::time_point last_animation_tick_ = std::chrono::steady_clock::now();
};

}  // namespace dcompframe
