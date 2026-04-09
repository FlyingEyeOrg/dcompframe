#include <gtest/gtest.h>

#include <memory>

#include "dcompframe/render_manager.h"
#include "dcompframe/tools/window_render_target.h"
#include "dcompframe/ui_element.h"
#include "dcompframe/window_host.h"

namespace dcompframe::tests {

TEST(LayoutManagerTests, StackStrategyAppliesSequentialBounds) {
    LayoutManager manager;
    manager.set_strategy(LayoutStrategy::Stack);

    auto root = std::make_shared<UIElement>("root");
    auto child_a = std::make_shared<UIElement>("a");
    auto child_b = std::make_shared<UIElement>("b");

    child_a->set_desired_size(Size {.width = 50.0F, .height = 10.0F});
    child_b->set_desired_size(Size {.width = 60.0F, .height = 15.0F});

    ASSERT_TRUE(root->add_child(child_a));
    ASSERT_TRUE(root->add_child(child_b));

    manager.apply_layout(root, Size {.width = 300.0F, .height = 120.0F});

    EXPECT_FLOAT_EQ(child_a->bounds().y, 0.0F);
    EXPECT_FLOAT_EQ(child_b->bounds().y, 10.0F);
}

TEST(UIElementTests, DirtyAndFocusFlagsPropagateCorrectly) {
    auto root = std::make_shared<UIElement>("root");
    auto child = std::make_shared<UIElement>("child");
    ASSERT_TRUE(root->add_child(child));

    root->clear_dirty_recursive();
    EXPECT_FALSE(root->is_dirty());
    EXPECT_FALSE(child->is_dirty());

    child->set_opacity(0.5F);
    EXPECT_TRUE(child->is_dirty());
    EXPECT_TRUE(root->is_dirty());

    child->set_focusable(true);
    child->set_focused(true);
    EXPECT_TRUE(child->is_focused());
}

TEST(WindowRenderTargetTests, InitializeAndPresentFrames) {
    RenderManager render_manager;
    ASSERT_TRUE(render_manager.initialize(true));

    WindowHost host;
    ASSERT_TRUE(host.create(L"RenderTargetTest", 320, 200));

    WindowRenderTarget render_target(&render_manager, &host);
    ASSERT_TRUE(render_target.initialize());
    EXPECT_TRUE(render_target.render_frame(true));
    EXPECT_EQ(render_target.presented_frames(), 1);

    host.destroy();
}

}  // namespace dcompframe::tests
