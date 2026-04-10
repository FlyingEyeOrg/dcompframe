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
    EXPECT_FLOAT_EQ(a_rect.width, 200.0F);
    EXPECT_FLOAT_EQ(a_rect.height, 20.0F);

    EXPECT_FLOAT_EQ(b_rect.x, 0.0F);
    EXPECT_FLOAT_EQ(b_rect.y, 30.0F);
    EXPECT_FLOAT_EQ(b_rect.width, 200.0F);
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
    EXPECT_FLOAT_EQ(a_rect.width, 60.0F);
    EXPECT_FLOAT_EQ(a_rect.height, 20.0F);

    EXPECT_FLOAT_EQ(b_rect.x, 0.0F);
    EXPECT_FLOAT_EQ(b_rect.y, 25.0F);
    EXPECT_FLOAT_EQ(b_rect.width, 60.0F);
    EXPECT_FLOAT_EQ(b_rect.height, 20.0F);
}

TEST(StackPanelTests, ArrangePreservesParentOffsetAndRespectsMargin) {
    auto stack = std::make_shared<StackPanel>(Orientation::Vertical);
    auto child = std::make_shared<UIElement>("child");

    child->set_desired_size(Size {.width = 40.0F, .height = 30.0F});
    child->set_margin(Thickness {.left = 6.0F, .top = 8.0F, .right = 10.0F, .bottom = 4.0F});
    ASSERT_TRUE(stack->add_child(child));

    stack->set_bounds(Rect {.x = 40.0F, .y = 24.0F, .width = 0.0F, .height = 0.0F});
    stack->arrange(Size {.width = 180.0F, .height = 120.0F});

    const auto stack_rect = stack->bounds();
    const auto child_rect = child->bounds();
    const auto absolute_child_rect = child->absolute_bounds();

    EXPECT_FLOAT_EQ(stack_rect.x, 40.0F);
    EXPECT_FLOAT_EQ(stack_rect.y, 24.0F);
    EXPECT_FLOAT_EQ(child_rect.x, 6.0F);
    EXPECT_FLOAT_EQ(child_rect.y, 8.0F);
    EXPECT_FLOAT_EQ(child_rect.width, 164.0F);
    EXPECT_FLOAT_EQ(child_rect.height, 30.0F);
    EXPECT_FLOAT_EQ(absolute_child_rect.x, 46.0F);
    EXPECT_FLOAT_EQ(absolute_child_rect.y, 32.0F);
}

TEST(GridPanelTests, ArrangePreservesParentOffsetAndAppliesMargins) {
    auto grid = std::make_shared<GridPanel>(1, 2);
    auto left = std::make_shared<UIElement>("left");
    auto right = std::make_shared<UIElement>("right");

    left->set_margin(Thickness {.left = 4.0F, .top = 6.0F, .right = 8.0F, .bottom = 10.0F});
    right->set_margin(Thickness {.left = 2.0F, .top = 4.0F, .right = 6.0F, .bottom = 8.0F});
    ASSERT_TRUE(grid->add_child(left));
    ASSERT_TRUE(grid->add_child(right));
    grid->set_grid_position(left, GridPanel::Cell {.row = 0, .col = 0});
    grid->set_grid_position(right, GridPanel::Cell {.row = 0, .col = 1});

    grid->set_bounds(Rect {.x = 20.0F, .y = 12.0F, .width = 0.0F, .height = 0.0F});
    grid->arrange(Size {.width = 200.0F, .height = 80.0F});

    const auto grid_rect = grid->bounds();
    const auto left_rect = left->bounds();
    const auto right_rect = right->bounds();
    const auto absolute_right_rect = right->absolute_bounds();

    EXPECT_FLOAT_EQ(grid_rect.x, 20.0F);
    EXPECT_FLOAT_EQ(grid_rect.y, 12.0F);
    EXPECT_FLOAT_EQ(left_rect.x, 4.0F);
    EXPECT_FLOAT_EQ(left_rect.y, 6.0F);
    EXPECT_FLOAT_EQ(left_rect.width, 88.0F);
    EXPECT_FLOAT_EQ(left_rect.height, 64.0F);
    EXPECT_FLOAT_EQ(right_rect.x, 102.0F);
    EXPECT_FLOAT_EQ(right_rect.y, 4.0F);
    EXPECT_FLOAT_EQ(right_rect.width, 92.0F);
    EXPECT_FLOAT_EQ(right_rect.height, 68.0F);
    EXPECT_FLOAT_EQ(absolute_right_rect.x, 122.0F);
    EXPECT_FLOAT_EQ(absolute_right_rect.y, 16.0F);
}

TEST(LayoutPanelsTests, NestedPanelsArrangeChildrenRecursively) {
    auto root = std::make_shared<StackPanel>(Orientation::Vertical);
    auto grid = std::make_shared<GridPanel>(1, 2);
    auto left_stack = std::make_shared<StackPanel>(Orientation::Vertical);
    auto right_stack = std::make_shared<StackPanel>(Orientation::Vertical);
    auto left_child = std::make_shared<UIElement>("left_child");
    auto right_child = std::make_shared<UIElement>("right_child");

    root->set_spacing(12.0F);
    left_stack->set_spacing(10.0F);
    right_stack->set_spacing(10.0F);
    grid->set_desired_size(Size {.width = 300.0F, .height = 120.0F});
    left_child->set_margin(Thickness {.left = 5.0F, .top = 24.0F, .right = 7.0F, .bottom = 0.0F});
    right_child->set_margin(Thickness {.left = 3.0F, .top = 24.0F, .right = 9.0F, .bottom = 0.0F});
    left_child->set_desired_size(Size {.width = 0.0F, .height = 40.0F});
    right_child->set_desired_size(Size {.width = 0.0F, .height = 50.0F});

    ASSERT_TRUE(root->add_child(grid));
    ASSERT_TRUE(grid->add_child(left_stack));
    ASSERT_TRUE(grid->add_child(right_stack));
    grid->set_grid_position(left_stack, GridPanel::Cell {.row = 0, .col = 0});
    grid->set_grid_position(right_stack, GridPanel::Cell {.row = 0, .col = 1});
    ASSERT_TRUE(left_stack->add_child(left_child));
    ASSERT_TRUE(right_stack->add_child(right_child));

    root->arrange(Size {.width = 300.0F, .height = 200.0F});

    const auto left_absolute = left_child->absolute_bounds();
    const auto right_absolute = right_child->absolute_bounds();

    EXPECT_FLOAT_EQ(left_absolute.x, 5.0F);
    EXPECT_FLOAT_EQ(left_absolute.y, 24.0F);
    EXPECT_FLOAT_EQ(left_absolute.width, 138.0F);
    EXPECT_FLOAT_EQ(left_absolute.height, 40.0F);
    EXPECT_FLOAT_EQ(right_absolute.x, 153.0F);
    EXPECT_FLOAT_EQ(right_absolute.y, 24.0F);
    EXPECT_FLOAT_EQ(right_absolute.width, 138.0F);
    EXPECT_FLOAT_EQ(right_absolute.height, 50.0F);
}

}  // namespace dcompframe::tests
