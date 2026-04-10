#include <fmt/core.h>
#include <fmt/format.h>

#include <chrono>
#include <filesystem>
#include <memory>

#include "dcompframe/animation/animation_manager.h"
#include "dcompframe/binding/observable.h"
#include "dcompframe/config/app_config.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/controls/style.h"
#include "dcompframe/input/input_manager.h"
#include "dcompframe/layout/grid_panel.h"
#include "dcompframe/layout/stack_panel.h"
#include "dcompframe/render_manager.h"
#include "dcompframe/tools/window_render_target.h"
#include "dcompframe/ui_element.h"
#include "dcompframe/window_host.h"

int main() {
    dcompframe::AppConfig config;
    const auto config_status = dcompframe::AppConfigLoader::load_from_file("demo/demo-config.json", config);
    if (!config_status.ok()) {
        fmt::print("Config load warning: {}\n", config_status.message);
    }

    dcompframe::RenderManager render_manager;
    const auto backend = config.use_directx_backend ? dcompframe::RenderBackend::DirectX : dcompframe::RenderBackend::Simulated;
    if (!render_manager.initialize_with_backend(backend)) {
        fmt::print("RenderManager initialization failed for backend={}, fallback to Simulated.\n", static_cast<int>(backend));
        if (!render_manager.initialize_with_backend(dcompframe::RenderBackend::Simulated)) {
            fmt::print("RenderManager initialization failed for Simulated backend.\n");
            return 1;
        }
    }
    render_manager.start_render_thread();

    dcompframe::WindowHost host;
    if (!host.create(L"DCompFrame Demo", config.width, config.height)) {
        fmt::print("Window creation failed.\n");
        return 2;
    }
    host.set_visible(true);
    host.apply_dpi(144);

    dcompframe::Theme theme = dcompframe::Theme::make_dark();
    if (config.theme_name == "light") {
        theme = dcompframe::Theme::make_light();
    }
    if (config.theme_name == "brand") {
        theme = dcompframe::Theme::make_brand();
    }
    theme.set_active_palette(config.theme_name);
    theme.set_style("card", config.card_style);

    auto root = std::make_shared<dcompframe::GridPanel>(1, 1);
    auto card = std::make_shared<dcompframe::Card>();
    card->set_title("DCompFrame Product Demo");
    card->set_body("完整功能：主题、绑定、输入、控件、动画、渲染、诊断、配置");
    card->set_icon("rocket");
    card->set_tags({"DirectComposition", "UI", "TDD", "Product"});
    card->set_style(theme.resolve("card"));
    card->set_desired_size(dcompframe::Size {.width = 720.0F, .height = 280.0F});

    auto action = std::make_shared<dcompframe::Button>("开始");
    action->set_style(theme.resolve("button.primary"));
    action->set_on_click([&render_manager] {
        render_manager.diagnostics().log(dcompframe::LogLevel::Info, "Primary action clicked");
    });
    card->set_primary_action(action);

    auto text_box = std::make_shared<dcompframe::TextBox>();
    text_box->set_placeholder("Type title via binding");
    text_box->set_text("Demo");
    text_box->set_selection(0, 4);
    text_box->set_composition_text("DCompFrame");
    text_box->commit_composition();

    auto list_view = std::make_shared<dcompframe::ListView>();
    list_view->set_items({"Overview", "Diagnostics", "Settings", "About"});
    list_view->set_selected_index(1);
    list_view->set_groups({
        dcompframe::ListGroup {.name = "Core", .items = {"Overview", "Diagnostics"}},
        dcompframe::ListGroup {.name = "App", .items = {"Settings", "About"}},
    });
    const auto visible = list_view->visible_range(0.0F, 48.0F, 16.0F);
    render_manager.diagnostics().log(dcompframe::LogLevel::Info, fmt::format("List visible range: {}-{}", visible.first, visible.second));

    auto check_box = std::make_shared<dcompframe::CheckBox>();
    check_box->set_checked(true);

    auto combo_box = std::make_shared<dcompframe::ComboBox>();
    combo_box->set_items({"Overview", "Diagnostics", "Settings", "About"});
    combo_box->set_selected_index(1);

    auto slider = std::make_shared<dcompframe::Slider>();
    slider->set_range(0.0F, 1.0F);
    slider->set_value(0.75F);

    auto scroll_viewer = std::make_shared<dcompframe::ScrollViewer>();
    scroll_viewer->set_scroll_offset(16.0F, 48.0F);
    scroll_viewer->set_inertia_velocity(0.02F, 0.12F);
    scroll_viewer->tick_inertia(std::chrono::milliseconds {120});

    auto stack = std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    stack->set_spacing(12.0F);
    stack->add_child(text_box);
    stack->add_child(list_view);
    stack->add_child(check_box);
    stack->add_child(combo_box);
    stack->add_child(slider);
    stack->add_child(scroll_viewer);
    stack->arrange(dcompframe::Size {.width = 420.0F, .height = 380.0F});
    card->add_child(stack);

    dcompframe::BindingContext context;
    card->bind(context);
    text_box->bind_text(context.title);
    action->bind_enabled(context.enabled);
    context.title.set("DCompFrame Bound Title");
    context.body.set("Body is synchronized through observable binding.");
    context.enabled.set(true);

    root->add_child(card);
    root->set_grid_position(card, dcompframe::GridPanel::Cell {.row = 0, .col = 0, .row_span = 1, .col_span = 1});
    root->arrange(dcompframe::Size {.width = static_cast<float>(config.width), .height = static_cast<float>(config.height)});

    dcompframe::AnimationManager animation_manager;
    animation_manager.add(dcompframe::AnimationClip {
        .target = card,
        .property = dcompframe::AnimatedProperty::Opacity,
        .from = 0.2F,
        .to = 1.0F,
        .duration = std::chrono::milliseconds {350},
        .elapsed = std::chrono::milliseconds {0},
        .easing = dcompframe::EasingType::EaseOut,
        .completed = false,
    });

    animation_manager.tick(std::chrono::milliseconds {350});

    animation_manager.add(dcompframe::AnimationClip {
        .target = card,
        .property = dcompframe::AnimatedProperty::PositionX,
        .from = 0.0F,
        .to = 30.0F,
        .duration = std::chrono::milliseconds {200},
        .elapsed = std::chrono::milliseconds {0},
        .easing = dcompframe::EasingType::EaseInOut,
        .completed = false,
    });
    animation_manager.tick(std::chrono::milliseconds {200});

    dcompframe::InputManager input;
    input.set_focus_ring_root(stack);
    input.focus_next();
    input.focus_next();
    input.set_click_handler([&](dcompframe::UIElement&) { action->click(); });
    input.set_double_click_handler([&](dcompframe::UIElement&) {
        render_manager.diagnostics().log(dcompframe::LogLevel::Info, "Double click captured");
    });
    input.set_drag_handler([&](dcompframe::UIElement&, dcompframe::Point delta) {
        render_manager.diagnostics().log(dcompframe::LogLevel::Info, fmt::format("Drag delta=({}, {})", delta.x, delta.y));
    });
    input.register_shortcut('S', true, false, false, [&render_manager] {
        render_manager.diagnostics().log(dcompframe::LogLevel::Info, "Ctrl+S command executed");
    });
    input.on_mouse_down(action, dcompframe::Point {.x = 10.0F, .y = 10.0F});
    input.on_mouse_down(action, dcompframe::Point {.x = 10.0F, .y = 10.0F});
    input.on_mouse_move(dcompframe::Point {.x = 30.0F, .y = 24.0F});
    input.on_mouse_up(dcompframe::Point {.x = 30.0F, .y = 24.0F});
    input.on_key_down('S', true, false, false);
    action->click();

    render_manager.resource_manager().register_resource("main-card-texture", dcompframe::ResourceType::Texture, 2048 * 2048 * 4);
    render_manager.resource_manager().register_resource("ui-vertex-buffer", dcompframe::ResourceType::Buffer, 1024 * 64);

    dcompframe::WindowRenderTarget render_target(&render_manager, &host);
    render_target.set_overlay_scene(dcompframe::WindowRenderTarget::OverlayScene {
        .title = L"DCompFrame Product Demo Controls",
        .items = {
            L"TextBox: DCompFrame Bound Title",
            L"ListView: Overview, Diagnostics, Settings, About",
            L"CheckBox: checked",
            L"ComboBox: Diagnostics",
            L"Slider: value=0.75",
            L"ScrollViewer: offset=(16,48)",
        },
    });
    const bool initialized = render_target.initialize();
    host.request_render();
    const int rendered = host.run_message_loop([&render_target] { return render_target.render_frame(true); });

    const auto report_path = std::filesystem::path("build") / "diagnostics-report.json";
    render_manager.diagnostics().export_report(report_path);
    const auto drained = render_manager.drain_commands();

    fmt::print(
        "DCompFrame demo initialized. dpi_scale={:.2f}, target_ready={}, commits={}, resources={}, avg_frame_ms={:.2f}, p95_ms={:.2f}, cps={:.2f}, theme={}\n",
        host.config().dpi_scale,
        initialized,
        render_manager.total_commit_count(),
        render_manager.resource_manager().resource_count(),
        render_manager.diagnostics().average_frame_ms(),
        render_manager.diagnostics().frame_p95_ms(),
        render_manager.diagnostics().commits_per_second(),
        theme.active_palette());
    fmt::print("Message loop exited. rendered_frames={}\n", rendered);
    fmt::print("Drained render commands: {}\n", drained.size());
    fmt::print("Diagnostics report: {}\n", report_path.string());

    render_manager.stop_render_thread();
    host.destroy();

    return 0;
}
