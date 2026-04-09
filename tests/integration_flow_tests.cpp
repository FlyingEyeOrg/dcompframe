#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include "dcompframe/animation/animation_manager.h"
#include "dcompframe/binding/observable.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/controls/style.h"
#include "dcompframe/input/input_manager.h"
#include "dcompframe/layout/grid_panel.h"
#include "dcompframe/render_manager.h"
#include "dcompframe/tools/window_render_target.h"
#include "dcompframe/window_host.h"

namespace dcompframe::tests {

TEST(IntegrationTests, WindowRenderAnimationAndInputFlow) {
    RenderManager render_manager;
    ASSERT_TRUE(render_manager.initialize(true));

    WindowHost host;
    ASSERT_TRUE(host.create(L"IntegrationHost", 800, 600));
    host.set_visible(true);

    auto root = std::make_shared<GridPanel>(1, 1);
    auto card = std::make_shared<Card>();
    auto action = std::make_shared<Button>("Go");
    card->set_primary_action(action);
    ASSERT_TRUE(root->add_child(card));

    BindingContext context;
    card->bind(context);
    context.title.set("Integration");
    context.body.set("Flow");

    AnimationManager animation;
    animation.add(AnimationClip {
        .target = card,
        .property = AnimatedProperty::Opacity,
        .from = 0.0F,
        .to = 1.0F,
        .duration = std::chrono::milliseconds {120},
        .elapsed = std::chrono::milliseconds {0},
        .easing = EasingType::EaseInOut,
        .completed = false,
    });
    animation.tick(std::chrono::milliseconds {120});
    EXPECT_NEAR(card->opacity(), 1.0F, 0.0001F);

    InputManager input;
    input.set_focus_ring_root(root);
    input.focus_next();

    int clicked = 0;
    action->set_on_click([&clicked] { ++clicked; });
    input.set_click_handler([&](UIElement&) { action->click(); });
    input.on_mouse_down(action, Point {.x = 1.0F, .y = 1.0F});
    EXPECT_EQ(clicked, 1);

    WindowRenderTarget target(&render_manager, &host);
    ASSERT_TRUE(target.initialize());
    host.request_render();
    const int rendered = host.run_message_loop([&target] { return target.render_frame(true); }, 3);
    EXPECT_GE(rendered, 1);
    EXPECT_GE(render_manager.total_commit_count(), 1);

    host.destroy();
}

TEST(IntegrationTests, DeviceLossRecoveryStressLoopRemainsStable) {
    RenderManager render_manager;
    ASSERT_TRUE(render_manager.initialize(true));

    WindowHost host;
    ASSERT_TRUE(host.create(L"RecoveryStressHost", 640, 360));
    host.set_visible(true);

    WindowRenderTarget target(&render_manager, &host);
    ASSERT_TRUE(target.initialize());

    constexpr int kIterations = 500;
    int recovered_count = 0;
    for (int i = 0; i < kIterations; ++i) {
        render_manager.device_recovery().notify_device_lost();
        host.request_render();
        const bool committed = target.render_frame(true);
        if (committed) {
            ++recovered_count;
        }
    }

    EXPECT_EQ(render_manager.device_recovery().recover_count(), kIterations);
    EXPECT_EQ(recovered_count, kIterations);
    EXPECT_GE(render_manager.total_commit_count(), kIterations);

    host.destroy();
}

}  // namespace dcompframe::tests
