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
#include "dcompframe/layout/grid_panel.h"
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
    std::shared_ptr<dcompframe::GridPanel> root_grid = std::make_shared<dcompframe::GridPanel>(12, 2);
    std::shared_ptr<dcompframe::Button> primary_button = std::make_shared<dcompframe::Button>("新建窗口");
    std::shared_ptr<dcompframe::TextBox> text_box = std::make_shared<dcompframe::TextBox>();
    std::shared_ptr<dcompframe::RichTextBox> rich_text_box = std::make_shared<dcompframe::RichTextBox>();
    std::shared_ptr<dcompframe::CheckBox> check_box = std::make_shared<dcompframe::CheckBox>();
    std::shared_ptr<dcompframe::ComboBox> combo_box = std::make_shared<dcompframe::ComboBox>();
    std::shared_ptr<dcompframe::Slider> slider = std::make_shared<dcompframe::Slider>();
    std::shared_ptr<dcompframe::ScrollViewer> scroll_viewer = std::make_shared<dcompframe::ScrollViewer>();
    std::shared_ptr<dcompframe::ItemsControl> items_control = std::make_shared<dcompframe::ItemsControl>();
    std::shared_ptr<dcompframe::ListView> list_view = std::make_shared<dcompframe::ListView>();
    std::shared_ptr<dcompframe::TextBlock> text_block = std::make_shared<dcompframe::TextBlock>("Element Plus 风格预览");
        std::shared_ptr<dcompframe::Label> label = std::make_shared<dcompframe::Label>("状态：准备就绪");
    std::shared_ptr<dcompframe::Image> image = std::make_shared<dcompframe::Image>();
        std::shared_ptr<dcompframe::Progress> progress = std::make_shared<dcompframe::Progress>();
        std::shared_ptr<dcompframe::Loading> loading = std::make_shared<dcompframe::Loading>();
        std::shared_ptr<dcompframe::TabControl> tab_control = std::make_shared<dcompframe::TabControl>();
        std::shared_ptr<dcompframe::Popup> popup = std::make_shared<dcompframe::Popup>();
        std::shared_ptr<dcompframe::Expander> expander = std::make_shared<dcompframe::Expander>();
    std::shared_ptr<dcompframe::Card> card = std::make_shared<dcompframe::Card>();
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

    rich_text_box->set_rich_text(
        "RichTextBox 示例: 这里现在支持直接编辑。\n"
        "你可以输入多行文本、回车换行，并观察光标、选区和自动换行表现。\n"
        "当前 Demo 会统一将非布局控件渲染为更接近 Element Plus 的浅色表单风格。\n"
        "窗口缩放时，表单区、列表区、卡片区和底部滚动区会一起自适应。");

    check_box->set_checked((id % 2U) == 1U);
    combo_box->set_items({"Overview", "Diagnostics", "Editor", "Preview", "Settings", "Account", "Advanced Options"});
    combo_box->set_selected_index(id % combo_box->items().size());
    slider->set_range(0.0F, 100.0F);
    slider->set_step(5.0F);
    slider->set_value(25.0F + static_cast<float>((id - 1U) % 4U) * 20.0F);

    items_control->set_items({
        "Alpha capability",
        "Beta diagnostics",
        "Gamma animation",
        "Delta layout token",
        "Epsilon state sync",
        "Zeta typography",
        "Eta popup layering",
        "Theta responsive panel",
        "Iota virtual viewport",
        "Kappa action footer",
    });
    items_control->set_selected_index((id - 1U) % items_control->items().size());
    items_control->set_item_spacing(6.0F);

    std::vector<std::string> scroll_items;
    scroll_items.reserve(36);
    for (int index = 1; index <= 36; ++index) {
        scroll_items.push_back(fmt::format("Scroll item {:02d} - 用于检查滚动条、裁剪区、文本对齐与 hover 呈现", index));
    }
    auto scroll_content = std::make_shared<dcompframe::ItemsControl>();
    scroll_content->set_items(std::move(scroll_items));
    scroll_viewer->set_content(scroll_content);

    list_view->set_groups({
        dcompframe::ListGroup {.name = "Primary", .items = {"Layout", "Rendering", "Input", "Composition", "Typography"}},
        dcompframe::ListGroup {.name = "Secondary", .items = {"Diagnostics", "Theme", "Assets", "Preview", "Scroll"}},
        dcompframe::ListGroup {.name = "Utility", .items = {"Card", "Popup", "Editor", "Tokens", "Status"}},
    });
    list_view->set_selected_index(2);

    image->set_source("demo://element-plus-placeholder");
        progress->set_range(0.0F, 100.0F);
        progress->set_value(36.0F + static_cast<float>((id % 4U) * 12U));
        progress->set_indeterminate(false);
        loading->set_active((id % 2U) == 0U);
        loading->set_overlay_mode(false);
        loading->set_text("同步渲染状态...");
        tab_control->set_tabs({"概览", "交互", "诊断"});
        tab_control->set_selected_index((id - 1U) % 3U);
        popup->set_title("状态弹层");
        popup->set_body("用于演示 Popup/Modal 语义与层级展示。\nEsc 可关闭临时 UI。\n当前为示例展示模式。");
        popup->set_modal(true);
        popup->set_open((id % 2U) == 1U);
        expander->set_header("更多控件说明");
        expander->set_content_text("Expander 在收起时不占据正文空间，展开后展示附加说明。\n该行为遵循 ui-requirements 的容器与状态规范。");
        expander->set_expanded((id % 3U) != 0U);
    card->set_title("Element Plus Card");
    card->set_body("用于展示卡片、标签、按钮与描述文本的组合布局。当前使用更轻的分层、浅蓝标题带和更贴近 Web Element Plus 的信息密度。");
    card->set_icon("picture");
    card->set_tags({"Preview", "Control", "Card"});
    card->set_primary_action(primary_button);

    root_grid->set_rows_cols(12, 2);
    root_grid->add_child(text_box);
    root_grid->set_grid_position(text_box, {.row = 0, .col = 0});
    root_grid->add_child(rich_text_box);
    root_grid->set_grid_position(rich_text_box, {.row = 1, .col = 0, .row_span = 3});
    root_grid->add_child(check_box);
    root_grid->set_grid_position(check_box, {.row = 4, .col = 0});
    root_grid->add_child(combo_box);
    root_grid->set_grid_position(combo_box, {.row = 5, .col = 0});
    root_grid->add_child(slider);
    root_grid->set_grid_position(slider, {.row = 6, .col = 0});
    root_grid->add_child(text_block);
    root_grid->set_grid_position(text_block, {.row = 0, .col = 1});
    root_grid->add_child(image);
    root_grid->set_grid_position(image, {.row = 1, .col = 1, .row_span = 2});
    root_grid->add_child(card);
    root_grid->set_grid_position(card, {.row = 3, .col = 1, .row_span = 2});
    root_grid->add_child(list_view);
    root_grid->set_grid_position(list_view, {.row = 5, .col = 1, .row_span = 2});
    root_grid->add_child(items_control);
    root_grid->set_grid_position(items_control, {.row = 7, .col = 1, .row_span = 2});
    root_grid->add_child(scroll_viewer);
    root_grid->set_grid_position(scroll_viewer, {.row = 9, .col = 0, .row_span = 3, .col_span = 2});

    primary_button->set_on_click([this] {
        if (app != nullptr) {
            app->render_manager().diagnostics().log(dcompframe::LogLevel::Info, fmt::format("Create window requested from window {}", id));
        }
    });
    primary_button->bind_enabled(binding_context.enabled);
    binding_context.enabled.set(true);

    rich_text_box->set_rich_text(rich_text_box->rich_text() + fmt::format("\n\nWindow {} 已准备就绪。", id));

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
        .rich_text_box = rich_text_box,
        .check_box = check_box,
        .combo_box = combo_box,
        .slider = slider,
        .scroll_viewer = scroll_viewer,
        .list_view = list_view,
        .items_control = items_control,
        .text_block = text_block,
        .label = label,
        .image = image,
        .progress = progress,
        .loading = loading,
        .tab_control = tab_control,
        .popup = popup,
        .expander = expander,
        .card = card,
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

    const auto size = host.client_size();
    root_grid->arrange(dcompframe::Size {.width = size.width, .height = size.height});

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
