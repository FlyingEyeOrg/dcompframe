#include <gtest/gtest.h>

#include "dcompframe/render_manager.h"
#include "dcompframe/window_host.h"

namespace dcompframe::tests {

TEST(RenderManagerTests, InitializeAndShutdown) {
    RenderManager manager;

    EXPECT_TRUE(manager.initialize(true));
    EXPECT_TRUE(manager.is_initialized());

    manager.shutdown();
    EXPECT_FALSE(manager.is_initialized());

    EXPECT_FALSE(manager.initialize(false));
}

TEST(RenderManagerTests, CommitRequiresInitBindingAndDirtyFlag) {
    RenderManager manager;
    auto bridge = manager.create_composition_bridge();

    EXPECT_FALSE(bridge.commit_changes(true));

    ASSERT_TRUE(manager.initialize(true));
    EXPECT_FALSE(bridge.commit_changes(true));

    const auto fake_hwnd = reinterpret_cast<HWND>(1);
    EXPECT_TRUE(bridge.bind_target_handle(fake_hwnd));
    EXPECT_EQ(bridge.bound_hwnd(), fake_hwnd);

    EXPECT_FALSE(bridge.commit_changes(false));
    EXPECT_TRUE(bridge.commit_changes(true));
    EXPECT_EQ(bridge.commit_count(), 1);
    EXPECT_EQ(manager.total_commit_count(), 1);
}

TEST(WindowHostTests, DefaultConfigIncludesNoRedirectionBitmap) {
    WindowHost host;

    EXPECT_NE(host.config().ex_style & WS_EX_NOREDIRECTIONBITMAP, 0UL);
}

TEST(WindowHostTests, ResizeAndDpiChangeTriggerRedraw) {
    WindowHost host;

    host.on_size_changed(1280, 720);
    const auto size = host.client_size();
    EXPECT_FLOAT_EQ(size.width, 1280.0F);
    EXPECT_FLOAT_EQ(size.height, 720.0F);

    EXPECT_TRUE(host.consume_redraw_request());
    EXPECT_FALSE(host.consume_redraw_request());

    host.apply_dpi(144);
    EXPECT_NEAR(host.config().dpi_scale, 1.5F, 0.001F);
    EXPECT_TRUE(host.consume_redraw_request());
}

TEST(WindowHostTests, StateTransitionsAreTracked) {
    WindowHost host;

    host.set_window_state(WindowState::Maximized);
    EXPECT_EQ(host.window_state(), WindowState::Maximized);

    host.set_window_state(WindowState::Fullscreen);
    EXPECT_EQ(host.window_state(), WindowState::Fullscreen);
}

}  // namespace dcompframe::tests
