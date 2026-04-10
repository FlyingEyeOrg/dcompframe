#include <gtest/gtest.h>

#include "dcompframe/application.h"
#include "dcompframe/render_manager.h"
#include "dcompframe/window_host.h"

namespace dcompframe::tests {

namespace {

class SkeletonWindow final : public Window {
public:
    using Window::Window;

    [[nodiscard]] bool build_called() const {
        return build_called_;
    }

protected:
    bool build(const AppConfig& config) override {
        build_called_ = true;
        (void)config;
        return true;
    }

private:
    bool build_called_ = false;
};

}  // namespace

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

TEST(WindowHostTests, DestroySingleWindowDoesNotQuitWhenOtherWindowAlive) {
    WindowHost first;
    WindowHost second;

    ASSERT_TRUE(first.create(L"FirstWindow", 320, 240));
    ASSERT_TRUE(second.create(L"SecondWindow", 320, 240));

    first.destroy();

    second.request_render();
    const int rendered = second.run_message_loop([] { return true; }, 1);
    EXPECT_EQ(rendered, 1);

    second.destroy();
}

TEST(WindowHostTests, DestroyNotificationClearsInternalState) {
    WindowHost host;

    ASSERT_TRUE(host.create(L"DestroyStateHost", 320, 240));
    EXPECT_TRUE(host.is_created());

    host.on_destroyed();
    EXPECT_FALSE(host.is_created());
    EXPECT_FALSE(host.is_visible());
    EXPECT_EQ(host.hwnd(), nullptr);
}

TEST(WindowTests, SkeletonWindowInitializesWithoutDemoControls) {
    RenderManager manager;
    ASSERT_TRUE(manager.initialize(true));

    SkeletonWindow window(&manager, nullptr, 1U);
    AppConfig config;
    config.width = 320;
    config.height = 240;

    ASSERT_TRUE(window.initialize(config));
    EXPECT_TRUE(window.build_called());
    EXPECT_FALSE(window.needs_continuous_rendering());

    window.host().request_render();
    EXPECT_TRUE(window.render_if_requested());

    window.host().destroy();
    manager.shutdown();
}

TEST(ApplicationTests, InitializeDoesNotCreateWindowUntilRequested) {
    int factory_invocations = 0;

    {
        Application app;
        app.set_window_factory([&factory_invocations](RenderManager* render_manager, Application* application, std::size_t window_id) {
            ++factory_invocations;
            return std::make_unique<SkeletonWindow>(render_manager, application, window_id);
        });

        ASSERT_TRUE(app.initialize("demo/demo-config.json"));
        EXPECT_EQ(factory_invocations, 0);

        ASSERT_TRUE(app.create_window());
        EXPECT_EQ(factory_invocations, 1);
    }
}

}  // namespace dcompframe::tests
