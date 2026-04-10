#include "dcompframe/application.h"

#include <algorithm>
#include <filesystem>

#include <fmt/core.h>
#include <fmt/format.h>

namespace dcompframe {

Window::Window(RenderManager* render_manager, Application* application, std::size_t window_id)
    : render_target_(render_manager, &host_), app_(application), id_(window_id) {}

bool Window::initialize(const AppConfig& config) {
    if (!host_.create(std::wstring(L"DCompFrame Demo - Window ") + std::to_wstring(id_), config.width, config.height)) {
        fmt::print("Window creation failed for window {}.\n", id_);
        return false;
    }

    text_box_->set_placeholder("请输入窗口标题，支持复制粘贴、选区和键盘导航");
    text_box_->bind_text(binding_context_.title);
    binding_context_.title.set(fmt::format("Window {}", id_));

    rich_text_box_->set_rich_text(
        "RichTextBox 示例: 这里现在支持直接编辑。\n"
        "你可以输入多行文本、回车换行，并观察光标、选区和自动换行表现。\n"
        "当前 Demo 会统一将非布局控件渲染为更接近 Element Plus 的浅色表单风格。\n"
        "窗口缩放时，表单区、列表区、卡片区和底部滚动区会一起自适应。");

    check_box_->set_checked((id_ % 2U) == 1U);
    combo_box_->set_items({"Overview", "Diagnostics", "Editor", "Preview", "Settings", "Account", "Advanced Options"});
    combo_box_->set_selected_index(id_ % combo_box_->items().size());
    slider_->set_range(0.0F, 100.0F);
    slider_->set_step(5.0F);
    slider_->set_value(25.0F + static_cast<float>((id_ - 1U) % 4U) * 20.0F);

    items_control_->set_items({
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
        "Lambda tab switching",
        "Mu expander details",
        "Nu scrollbar track click",
        "Xi stacked sections",
    });
    items_control_->set_selected_index((id_ - 1U) % items_control_->items().size());
    items_control_->set_item_spacing(6.0F);

    std::vector<std::string> scroll_items;
    scroll_items.reserve(48);
    for (int index = 1; index <= 48; ++index) {
        scroll_items.push_back(fmt::format("Scroll item {:02d} - 用于检查滚动条、裁剪区、文本对齐与 hover 呈现", index));
    }
    auto scroll_content = std::make_shared<ItemsControl>();
    scroll_content->set_items(std::move(scroll_items));
    scroll_viewer_->set_content(scroll_content);

    list_view_->set_groups({
        ListGroup {.name = "Primary", .items = {"Layout", "Rendering", "Input", "Composition", "Typography", "Animation"}},
        ListGroup {.name = "Secondary", .items = {"Diagnostics", "Theme", "Assets", "Preview", "Scroll", "Tabs"}},
        ListGroup {.name = "Utility", .items = {"Card", "Popup", "Editor", "Tokens", "Status", "Expander"}},
    });
    list_view_->set_selected_index(2);

    image_->set_source("demo://element-plus-placeholder");
    progress_->set_range(0.0F, 100.0F);
    progress_->set_value(36.0F + static_cast<float>((id_ % 4U) * 12U));
    progress_->set_indeterminate(false);
    loading_->set_active((id_ % 2U) == 0U);
    loading_->set_overlay_mode(false);
    loading_->set_text("正在同步控件状态与合成帧...");
    log_box_->set_max_lines(120);
    log_box_->set_lines({
        fmt::format("[window:{}] Demo 初始化完成", id_),
        "[layout] 标题区与内容区已解耦",
        "[tabs] 页签区域启用独立正文区",
        "[scroll] 滚动条支持轨道点击与聚焦态",
        "[perf] 动画改为节流驱动，避免消息阻塞卡顿",
    });
    tab_control_->set_tabs({"概览", "动画", "数据", "诊断"});
    tab_control_->set_selected_index((id_ - 1U) % 4U);
    popup_->set_title("状态弹层");
    popup_->set_body("用于演示 Popup/Modal 语义与层级展示。\nEsc 可关闭临时 UI。\n当前为示例展示模式。");
    popup_->set_modal(true);
    popup_->set_open((id_ % 2U) == 1U);
    expander_->set_header("更多控件说明");
    expander_->set_content_text("点击标题可展开或收起。\n收起时不再占用正文布局空间。\n滚动条轨道支持直接点击跳转，并显示聚焦态。\n右侧预览区包含 TabControl、Loading、Progress 与日志框示例。");
    expander_->set_expanded(true);
    card_->set_title("Element Plus Card");
    card_->set_body("用于展示卡片、页签、动画、进度与展开区的组合布局。当前改为更清晰的纵向分区，便于在滚动时保持标题稳定。");
    card_->set_icon("picture");
    card_->set_tags({"Preview", "Tabs", "Animation"});
    card_->set_primary_action(primary_button_);

    root_grid_->set_rows_cols(12, 2);
    root_grid_->add_child(text_box_);
    root_grid_->set_grid_position(text_box_, {.row = 0, .col = 0});
    root_grid_->add_child(rich_text_box_);
    root_grid_->set_grid_position(rich_text_box_, {.row = 1, .col = 0, .row_span = 3});
    root_grid_->add_child(check_box_);
    root_grid_->set_grid_position(check_box_, {.row = 4, .col = 0});
    root_grid_->add_child(combo_box_);
    root_grid_->set_grid_position(combo_box_, {.row = 5, .col = 0});
    root_grid_->add_child(slider_);
    root_grid_->set_grid_position(slider_, {.row = 6, .col = 0});
    root_grid_->add_child(text_block_);
    root_grid_->set_grid_position(text_block_, {.row = 0, .col = 1});
    root_grid_->add_child(image_);
    root_grid_->set_grid_position(image_, {.row = 1, .col = 1, .row_span = 2});
    root_grid_->add_child(card_);
    root_grid_->set_grid_position(card_, {.row = 3, .col = 1, .row_span = 2});
    root_grid_->add_child(list_view_);
    root_grid_->set_grid_position(list_view_, {.row = 5, .col = 1, .row_span = 2});
    root_grid_->add_child(items_control_);
    root_grid_->set_grid_position(items_control_, {.row = 7, .col = 1, .row_span = 2});
    root_grid_->add_child(scroll_viewer_);
    root_grid_->set_grid_position(scroll_viewer_, {.row = 9, .col = 0, .row_span = 3, .col_span = 2});

    primary_button_->set_on_click([this] {
        if (app_ != nullptr) {
            app_->render_manager().diagnostics().log(LogLevel::Info, fmt::format("Create window requested from window {}", id_));
        }
    });
    primary_button_->bind_enabled(binding_context_.enabled);
    binding_context_.enabled.set(true);

    rich_text_box_->set_rich_text(rich_text_box_->rich_text() + fmt::format("\n\nWindow {} 已准备就绪。", id_));

    text_box_->set_on_text_changed([this](const std::string& value) {
        if (app_ != nullptr) {
            app_->render_manager().diagnostics().log(LogLevel::Info, fmt::format("Window {} title edited: {}", id_, value));
        }
        log_box_->append_line(fmt::format("[input] 标题更新为: {}", value));
        host_.request_render();
    });
    check_box_->set_on_checked_changed([this](bool checked) {
        if (app_ != nullptr) {
            app_->render_manager().diagnostics().log(LogLevel::Info, fmt::format("Window {} checkbox={}", id_, checked));
        }
        log_box_->append_line(fmt::format("[toggle] CheckBox = {}", checked ? "true" : "false"));
        host_.request_render();
    });
    combo_box_->set_on_selection_changed([this](std::optional<std::size_t> index, const std::string& value) {
        if (app_ != nullptr) {
            app_->render_manager().diagnostics().log(
                LogLevel::Info,
                fmt::format("Window {} combo index={} value={}", id_, index ? static_cast<int>(*index) : -1, value));
        }
        log_box_->append_line(fmt::format("[combo] index={} value={}", index ? static_cast<int>(*index) : -1, value));
        host_.request_render();
    });
    slider_->set_on_value_changed([this](float value) {
        if (app_ != nullptr) {
            app_->render_manager().diagnostics().log(LogLevel::Info, fmt::format("Window {} slider={:.2f}", id_, value));
        }
        progress_->set_value(value);
        loading_->set_active(value < 100.0F);
        log_box_->append_line(fmt::format("[slider] value={:.0f}", value));
        host_.request_render();
    });

    render_target_.set_interactive_controls(WindowRenderTarget::InteractiveControls {
        .primary_button = primary_button_,
        .text_box = text_box_,
        .rich_text_box = rich_text_box_,
        .check_box = check_box_,
        .combo_box = combo_box_,
        .slider = slider_,
        .scroll_viewer = scroll_viewer_,
        .list_view = list_view_,
        .items_control = items_control_,
        .text_block = text_block_,
        .label = label_,
        .image = image_,
        .progress = progress_,
        .loading = loading_,
        .log_box = log_box_,
        .tab_control = tab_control_,
        .popup = popup_,
        .expander = expander_,
        .card = card_,
    });
    render_target_.set_primary_action_handler([this] {
        if (app_ != nullptr) {
            app_->create_window();
        }
    });

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

    const auto size = host_.client_size();
    root_grid_->arrange(Size {.width = size.width, .height = size.height});

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
    render_manager_.resource_manager().register_resource("main-card-texture", ResourceType::Texture, 2048 * 2048 * 4);
    render_manager_.resource_manager().register_resource("ui-vertex-buffer", ResourceType::Buffer, 1024 * 64);
    return create_window();
}

bool Application::create_window() {
    auto window = std::make_unique<Window>(&render_manager_, this, next_window_id_++);
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
