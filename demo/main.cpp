#include <fmt/core.h>

#include <chrono>
#include <memory>

#include "dcompframe/animation/animation_manager.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/controls/style.h"
#include "dcompframe/layout/grid_panel.h"
#include "dcompframe/layout/stack_panel.h"
#include "dcompframe/render_manager.h"
#include "dcompframe/tools/window_render_target.h"
#include "dcompframe/ui_element.h"
#include "dcompframe/window_host.h"

int main() {
    dcompframe::RenderManager render_manager;
    if (!render_manager.initialize_with_backend(dcompframe::RenderBackend::Simulated)) {
        fmt::print("RenderManager initialization failed.\n");
        return 1;
    }

    dcompframe::WindowHost host;
    if (!host.create(L"DCompFrame Demo", 1280, 720)) {
        fmt::print("Window creation failed.\n");
        return 2;
    }
    host.set_visible(true);
    host.apply_dpi(144);

    dcompframe::Theme theme;
    dcompframe::Style card_style {};
    card_style.background = dcompframe::Color {25, 31, 40, 255};
    card_style.corner_radius = 14.0F;
    theme.set_style("card", card_style);

    auto root = std::make_shared<dcompframe::GridPanel>(1, 1);
    auto card = std::make_shared<dcompframe::Card>();
    card->set_title("DCompFrame");
    card->set_body("完整模块示例：控件、动画、渲染目标与诊断");
    card->set_icon("rocket");
    card->set_tags({"DirectComposition", "UI", "TDD"});
    card->set_style(theme.resolve("card"));
    card->set_desired_size(dcompframe::Size {.width = 720.0F, .height = 280.0F});

    auto action = std::make_shared<dcompframe::Button>("开始");
    action->set_on_click([&render_manager] {
        render_manager.diagnostics().log(dcompframe::LogLevel::Info, "Primary action clicked");
    });
    card->set_primary_action(action);

    root->add_child(card);
    root->set_grid_position(card, dcompframe::GridPanel::Cell {.row = 0, .col = 0, .row_span = 1, .col_span = 1});
    root->arrange(dcompframe::Size {.width = 1280.0F, .height = 720.0F});

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
    action->click();

    render_manager.resource_manager().register_resource("main-card-texture", dcompframe::ResourceType::Texture, 2048 * 2048 * 4);

    dcompframe::WindowRenderTarget render_target(&render_manager, &host);
    const bool initialized = render_target.initialize();
    host.request_render();
    const int rendered = host.run_message_loop([&render_target] { return render_target.render_frame(true); });

    fmt::print(
        "DCompFrame demo initialized. dpi_scale={:.2f}, target_ready={}, commits={}, resources={}, avg_frame_ms={:.2f}\n",
        host.config().dpi_scale,
        initialized,
        render_manager.total_commit_count(),
        render_manager.resource_manager().resource_count(),
        render_manager.diagnostics().average_frame_ms());
    fmt::print("Message loop exited. rendered_frames={}\n", rendered);

    host.destroy();

    return 0;
}
