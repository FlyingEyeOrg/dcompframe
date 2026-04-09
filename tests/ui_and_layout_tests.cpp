#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "dcompframe/events/input_event.h"
#include "dcompframe/layout/grid_panel.h"
#include "dcompframe/layout/stack_panel.h"
#include "dcompframe/ui_element.h"

namespace dcompframe::tests {

TEST(UIElementTests, VisualTreeAddAndRemoveChild) {
    auto root = std::make_shared<UIElement>("root");
    auto child = std::make_shared<UIElement>("child");

    EXPECT_TRUE(root->add_child(child));
    ASSERT_EQ(root->children().size(), 1U);
    EXPECT_EQ(child->parent(), root);

    EXPECT_TRUE(root->remove_child(child));
    EXPECT_TRUE(root->children().empty());
    EXPECT_EQ(child->parent(), nullptr);
}

TEST(UIElementTests, EventDispatchSupportsCaptureTargetAndBubble) {
    auto root = std::make_shared<UIElement>("root");
    auto panel = std::make_shared<UIElement>("panel");
    auto button = std::make_shared<UIElement>("button");

    ASSERT_TRUE(root->add_child(panel));
    ASSERT_TRUE(panel->add_child(button));

    std::vector<std::string> call_order;

    root->set_event_handler([&](InputEvent&, EventPhase phase) {
        call_order.push_back("root-" + std::to_string(static_cast<int>(phase)));
    });
    panel->set_event_handler([&](InputEvent&, EventPhase phase) {
        call_order.push_back("panel-" + std::to_string(static_cast<int>(phase)));
    });
    button->set_event_handler([&](InputEvent&, EventPhase phase) {
        call_order.push_back("button-" + std::to_string(static_cast<int>(phase)));
    });

    InputEvent event;
    button->dispatch_event(event);

    const std::vector<std::string> expected {
        "root-0",
        "panel-0",
        "button-1",
        "panel-2",
        "root-2"
    };
    EXPECT_EQ(call_order, expected);
}

TEST(GridPanelTests, ArrangeSplitsCellsAndAppliesPlacement) {
    auto grid = std::make_shared<GridPanel>(2, 2);
    auto top_left = std::make_shared<UIElement>("top_left");
    auto bottom_right = std::make_shared<UIElement>("bottom_right");

    ASSERT_TRUE(grid->add_child(top_left));
    ASSERT_TRUE(grid->add_child(bottom_right));

    grid->set_grid_position(top_left, GridPanel::Cell {.row = 0, .col = 0, .row_span = 1, .col_span = 1});
    grid->set_grid_position(bottom_right, GridPanel::Cell {.row = 1, .col = 1, .row_span = 1, .col_span = 1});

    grid->arrange(Size {.width = 200.0F, .height = 100.0F});

    const auto tl = top_left->bounds();
    EXPECT_FLOAT_EQ(tl.x, 0.0F);
    EXPECT_FLOAT_EQ(tl.y, 0.0F);
    EXPECT_FLOAT_EQ(tl.width, 100.0F);
    EXPECT_FLOAT_EQ(tl.height, 50.0F);

    const auto br = bottom_right->bounds();
    EXPECT_FLOAT_EQ(br.x, 100.0F);
    EXPECT_FLOAT_EQ(br.y, 50.0F);
    EXPECT_FLOAT_EQ(br.width, 100.0F);
    EXPECT_FLOAT_EQ(br.height, 50.0F);
}

TEST(StackPanelTests, VerticalArrangeRespectsSpacing) {
    auto stack = std::make_shared<StackPanel>(Orientation::Vertical);
    stack->set_spacing(10.0F);

    auto a = std::make_shared<UIElement>("a");
    auto b = std::make_shared<UIElement>("b");

    a->set_desired_size(Size {.width = 40.0F, .height = 20.0F});
    b->set_desired_size(Size {.width = 80.0F, .height = 30.0F});

    ASSERT_TRUE(stack->add_child(a));
    ASSERT_TRUE(stack->add_child(b));

    stack->arrange(Size {.width = 200.0F, .height = 200.0F});

    const auto a_rect = a->bounds();
    const auto b_rect = b->bounds();

    EXPECT_FLOAT_EQ(a_rect.x, 0.0F);
    EXPECT_FLOAT_EQ(a_rect.y, 0.0F);
    EXPECT_FLOAT_EQ(a_rect.width, 40.0F);
    EXPECT_FLOAT_EQ(a_rect.height, 20.0F);

    EXPECT_FLOAT_EQ(b_rect.x, 0.0F);
    EXPECT_FLOAT_EQ(b_rect.y, 30.0F);
    EXPECT_FLOAT_EQ(b_rect.width, 80.0F);
    EXPECT_FLOAT_EQ(b_rect.height, 30.0F);
}

TEST(StackPanelTests, HorizontalArrangeWrapsWhenEnabled) {
    auto stack = std::make_shared<StackPanel>(Orientation::Horizontal);
    stack->set_spacing(5.0F);
    stack->set_wrap_enabled(true);

    auto a = std::make_shared<UIElement>("a");
    auto b = std::make_shared<UIElement>("b");

    a->set_desired_size(Size {.width = 60.0F, .height = 20.0F});
    b->set_desired_size(Size {.width = 60.0F, .height = 20.0F});

    ASSERT_TRUE(stack->add_child(a));
    ASSERT_TRUE(stack->add_child(b));

    stack->arrange(Size {.width = 100.0F, .height = 200.0F});

    const auto a_rect = a->bounds();
    const auto b_rect = b->bounds();

    EXPECT_FLOAT_EQ(a_rect.x, 0.0F);
    EXPECT_FLOAT_EQ(a_rect.y, 0.0F);

    EXPECT_FLOAT_EQ(b_rect.x, 0.0F);
    EXPECT_FLOAT_EQ(b_rect.y, 25.0F);
}

}  // namespace dcompframe::tests
