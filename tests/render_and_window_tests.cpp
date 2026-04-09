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

TEST(RenderManagerTests, ResourceManagerAndRecoveryWorkflows) {
    RenderManager manager;
    ASSERT_TRUE(manager.initialize(true));

    auto& resources = manager.resource_manager();
    EXPECT_TRUE(resources.register_resource("main_texture", ResourceType::Texture, 4096));
    EXPECT_TRUE(resources.register_resource("vertex_buffer", ResourceType::Buffer, 1024));
    EXPECT_EQ(resources.resource_count(), 2U);
    EXPECT_EQ(resources.total_bytes(), 5120U);
    EXPECT_TRUE(resources.contains("main_texture"));

    EXPECT_TRUE(resources.release_resource("main_texture"));
    EXPECT_EQ(resources.resource_count(), 1U);

    auto& recovery = manager.device_recovery();
    recovery.notify_device_lost();
    EXPECT_TRUE(recovery.is_device_lost());
    EXPECT_TRUE(recovery.try_recover());
    EXPECT_FALSE(recovery.is_device_lost());
    EXPECT_EQ(recovery.recover_count(), 1);

    auto& diagnostics = manager.diagnostics();
    diagnostics.log(LogLevel::Warning, "commit throttled");
    diagnostics.log(LogLevel::Error, "device timeout");
    EXPECT_EQ(diagnostics.warning_count(), 1U);
    EXPECT_EQ(diagnostics.error_count(), 1U);
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

TEST(WindowHostTests, CreateMessageLoopAndDestroy) {
    WindowHost host;

    ASSERT_TRUE(host.create(L"DCompFrameTest", 640, 480));
    EXPECT_TRUE(host.is_created());
    EXPECT_NE(host.hwnd(), nullptr);

    host.set_visible(true);
    EXPECT_TRUE(host.is_visible());

    host.request_render();
    const int rendered = host.run_message_loop([] { return true; }, 3);
    EXPECT_EQ(rendered, 1);

    host.destroy();
    EXPECT_FALSE(host.is_created());
}

TEST(WindowHostTests, MessageLoopExitsWhenQuitPosted) {
    WindowHost host;
    ASSERT_TRUE(host.create(L"DCompFrameQuitTest", 320, 240));

    PostQuitMessage(0);
    const int rendered = host.run_message_loop([] { return false; });
    EXPECT_EQ(rendered, 0);

    host.destroy();
}

}  // namespace dcompframe::tests
