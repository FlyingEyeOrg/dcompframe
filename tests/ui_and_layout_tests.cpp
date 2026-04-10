#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "dcompframe/events/input_event.h"
#include "dcompframe/input/input_manager.h"
#include "dcompframe/layout/flex_panel.h"
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

TEST(UIElementTests, HitTestFindsDeepestVisibleDescendant) {
    auto root = std::make_shared<UIElement>("root");
    auto container = std::make_shared<UIElement>("container");
    auto leaf = std::make_shared<UIElement>("leaf");

    ASSERT_TRUE(root->add_child(container));
    ASSERT_TRUE(container->add_child(leaf));

    root->set_bounds(Rect {.x = 0.0F, .y = 0.0F, .width = 320.0F, .height = 240.0F});
    container->set_bounds(Rect {.x = 20.0F, .y = 20.0F, .width = 200.0F, .height = 160.0F});
    leaf->set_bounds(Rect {.x = 12.0F, .y = 14.0F, .width = 72.0F, .height = 40.0F});

    const auto hit = root->hit_test(Point {.x = 40.0F, .y = 44.0F});
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->name(), "leaf");

    leaf->set_hit_test_visible(false);
    const auto fallback = root->hit_test(Point {.x = 40.0F, .y = 44.0F});
    ASSERT_NE(fallback, nullptr);
    EXPECT_EQ(fallback->name(), "container");
}

TEST(FlexPanelTests, ColumnArrangeRespectsGapAndStretch) {
    auto column = std::make_shared<FlexPanel>(FlexDirection::Column);
    column->set_row_gap(10.0F);

    auto a = std::make_shared<UIElement>("a");
    auto b = std::make_shared<UIElement>("b");

    a->set_desired_size(Size {.width = 40.0F, .height = 20.0F});
    b->set_desired_size(Size {.width = 80.0F, .height = 30.0F});

    ASSERT_TRUE(column->add_child(a));
    ASSERT_TRUE(column->add_child(b));

    column->measure(Size {.width = 200.0F, .height = 200.0F});
    column->arrange(Size {.width = 200.0F, .height = 200.0F});

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

TEST(FlexPanelTests, HorizontalWrapMovesOverflowItemsToNextLine) {
    auto row = std::make_shared<FlexPanel>(FlexDirection::Row);
    row->set_column_gap(5.0F);
    row->set_wrap(FlexWrap::Wrap);
    row->set_align_items(FlexAlignItems::Start);
    row->set_align_content(FlexAlignContent::Start);

    auto a = std::make_shared<UIElement>("a");
    auto b = std::make_shared<UIElement>("b");

    a->set_desired_size(Size {.width = 60.0F, .height = 20.0F});
    b->set_desired_size(Size {.width = 60.0F, .height = 20.0F});

    ASSERT_TRUE(row->add_child(a));
    ASSERT_TRUE(row->add_child(b));

    row->measure(Size {.width = 100.0F, .height = 200.0F});
    row->arrange(Size {.width = 100.0F, .height = 200.0F});

    const auto a_rect = a->bounds();
    const auto b_rect = b->bounds();

    EXPECT_FLOAT_EQ(a_rect.x, 0.0F);
    EXPECT_FLOAT_EQ(a_rect.y, 0.0F);
    EXPECT_FLOAT_EQ(a_rect.width, 60.0F);
    EXPECT_FLOAT_EQ(a_rect.height, 20.0F);

    EXPECT_FLOAT_EQ(b_rect.x, 0.0F);
    EXPECT_FLOAT_EQ(b_rect.y, 20.0F);
    EXPECT_FLOAT_EQ(b_rect.width, 60.0F);
    EXPECT_FLOAT_EQ(b_rect.height, 20.0F);
}

TEST(FlexPanelTests, ArrangePreservesParentOffsetAndRespectsMarginAndPadding) {
    auto stack = std::make_shared<FlexPanel>(FlexDirection::Column);
    auto child = std::make_shared<UIElement>("child");

    child->set_desired_size(Size {.width = 40.0F, .height = 30.0F});
    child->set_margin(Thickness {.left = 6.0F, .top = 8.0F, .right = 10.0F, .bottom = 4.0F});
    stack->set_padding(Thickness {.left = 12.0F, .top = 14.0F, .right = 16.0F, .bottom = 18.0F});
    ASSERT_TRUE(stack->add_child(child));

    stack->set_bounds(Rect {.x = 40.0F, .y = 24.0F, .width = 0.0F, .height = 0.0F});
    stack->measure(Size {.width = 180.0F, .height = 120.0F});
    stack->arrange(Size {.width = 180.0F, .height = 120.0F});

    const auto stack_rect = stack->bounds();
    const auto child_rect = child->bounds();
    const auto absolute_child_rect = child->absolute_bounds();

    EXPECT_FLOAT_EQ(stack_rect.x, 40.0F);
    EXPECT_FLOAT_EQ(stack_rect.y, 24.0F);
    EXPECT_FLOAT_EQ(child_rect.x, 18.0F);
    EXPECT_FLOAT_EQ(child_rect.y, 22.0F);
    EXPECT_FLOAT_EQ(child_rect.width, 136.0F);
    EXPECT_FLOAT_EQ(child_rect.height, 30.0F);
    EXPECT_FLOAT_EQ(absolute_child_rect.x, 58.0F);
    EXPECT_FLOAT_EQ(absolute_child_rect.y, 46.0F);
}

TEST(LayoutPanelsTests, NestedPanelsArrangeChildrenRecursively) {
    auto root = std::make_shared<FlexPanel>(FlexDirection::Column);
    auto grid = std::make_shared<FlexPanel>(FlexDirection::Row);
    auto left_stack = std::make_shared<FlexPanel>(FlexDirection::Column);
    auto right_stack = std::make_shared<FlexPanel>(FlexDirection::Column);
    auto left_child = std::make_shared<UIElement>("left_child");
    auto right_child = std::make_shared<UIElement>("right_child");

    root->set_row_gap(12.0F);
    grid->set_column_gap(10.0F);
    left_stack->set_row_gap(10.0F);
    right_stack->set_row_gap(10.0F);
    grid->set_desired_size(Size {.width = 300.0F, .height = 120.0F});
    left_stack->set_flex_grow(1.0F);
    left_stack->set_flex_basis(0.0F);
    right_stack->set_flex_grow(1.0F);
    right_stack->set_flex_basis(0.0F);
    left_child->set_margin(Thickness {.left = 5.0F, .top = 24.0F, .right = 7.0F, .bottom = 0.0F});
    right_child->set_margin(Thickness {.left = 3.0F, .top = 24.0F, .right = 9.0F, .bottom = 0.0F});
    left_child->set_desired_size(Size {.width = 0.0F, .height = 40.0F});
    right_child->set_desired_size(Size {.width = 0.0F, .height = 50.0F});

    ASSERT_TRUE(root->add_child(grid));
    ASSERT_TRUE(grid->add_child(left_stack));
    ASSERT_TRUE(grid->add_child(right_stack));
    ASSERT_TRUE(left_stack->add_child(left_child));
    ASSERT_TRUE(right_stack->add_child(right_child));

    root->measure(Size {.width = 300.0F, .height = 200.0F});
    root->arrange(Size {.width = 300.0F, .height = 200.0F});

    const auto left_absolute = left_child->absolute_bounds();
    const auto right_absolute = right_child->absolute_bounds();

    EXPECT_FLOAT_EQ(left_absolute.x, 5.0F);
    EXPECT_FLOAT_EQ(left_absolute.y, 24.0F);
    EXPECT_FLOAT_EQ(left_absolute.width, 133.0F);
    EXPECT_FLOAT_EQ(left_absolute.height, 40.0F);
    EXPECT_FLOAT_EQ(right_absolute.x, 158.0F);
    EXPECT_FLOAT_EQ(right_absolute.y, 24.0F);
    EXPECT_FLOAT_EQ(right_absolute.width, 133.0F);
    EXPECT_FLOAT_EQ(right_absolute.height, 50.0F);
}

TEST(FlexPanelTests, FlexGrowConsumesRemainingVerticalSpace) {
    auto stack = std::make_shared<FlexPanel>(FlexDirection::Column);
    auto fixed = std::make_shared<UIElement>("fixed");
    auto flexible = std::make_shared<UIElement>("flexible");

    stack->set_row_gap(10.0F);
    fixed->set_desired_size(Size {.width = 80.0F, .height = 40.0F});
    flexible->set_desired_size(Size {.width = 80.0F, .height = 30.0F});
    flexible->set_flex_grow(1.0F);

    ASSERT_TRUE(stack->add_child(fixed));
    ASSERT_TRUE(stack->add_child(flexible));

    stack->measure(Size {.width = 200.0F, .height = 200.0F});
    stack->arrange(Size {.width = 200.0F, .height = 200.0F});

    EXPECT_FLOAT_EQ(fixed->bounds().height, 40.0F);
    EXPECT_FLOAT_EQ(flexible->bounds().y, 50.0F);
    EXPECT_FLOAT_EQ(flexible->bounds().height, 150.0F);
}

TEST(FlexPanelTests, RowLayoutDistributesGrowBasisAndGap) {
    auto row = std::make_shared<FlexPanel>(FlexDirection::Row);
    auto left = std::make_shared<UIElement>("left");
    auto right = std::make_shared<UIElement>("right");

    row->set_column_gap(12.0F);
    left->set_flex_basis(60.0F);
    left->set_flex_grow(1.0F);
    right->set_flex_basis(60.0F);
    right->set_flex_grow(2.0F);

    ASSERT_TRUE(row->add_child(left));
    ASSERT_TRUE(row->add_child(right));

    row->measure(Size {.width = 300.0F, .height = 100.0F});
    row->arrange(Size {.width = 300.0F, .height = 100.0F});

    EXPECT_FLOAT_EQ(left->bounds().x, 0.0F);
    EXPECT_FLOAT_EQ(left->bounds().width, 116.0F);
    EXPECT_FLOAT_EQ(right->bounds().x, 128.0F);
    EXPECT_FLOAT_EQ(right->bounds().width, 172.0F);
}

TEST(FlexPanelTests, WrapMovesOverflowItemsToNextLine) {
    auto row = std::make_shared<FlexPanel>(FlexDirection::Row);
    auto a = std::make_shared<UIElement>("a");
    auto b = std::make_shared<UIElement>("b");
    auto c = std::make_shared<UIElement>("c");

    row->set_wrap(FlexWrap::Wrap);
    row->set_align_content(FlexAlignContent::Start);
    row->set_column_gap(10.0F);
    row->set_row_gap(8.0F);
    a->set_desired_size(Size {.width = 70.0F, .height = 20.0F});
    b->set_desired_size(Size {.width = 70.0F, .height = 20.0F});
    c->set_desired_size(Size {.width = 70.0F, .height = 20.0F});

    ASSERT_TRUE(row->add_child(a));
    ASSERT_TRUE(row->add_child(b));
    ASSERT_TRUE(row->add_child(c));

    row->measure(Size {.width = 170.0F, .height = 120.0F});
    row->arrange(Size {.width = 170.0F, .height = 120.0F});

    EXPECT_FLOAT_EQ(a->bounds().y, 0.0F);
    EXPECT_FLOAT_EQ(b->bounds().y, 0.0F);
    EXPECT_FLOAT_EQ(c->bounds().y, 28.0F);
}

TEST(FlexPanelTests, OrderAndMinMaxConstraintsProduceStableBounds) {
    auto row = std::make_shared<FlexPanel>(FlexDirection::Row);
    row->set_column_gap(10.0F);

    auto first = std::make_shared<UIElement>("first");
    auto second = std::make_shared<UIElement>("second");
    auto third = std::make_shared<UIElement>("third");

    first->set_flex_basis(120.0F);
    first->set_max_size(Size {.width = 80.0F, .height = -1.0F});
    second->set_flex_basis(40.0F);
    second->set_order(-1);
    third->set_flex_basis(50.0F);
    third->set_min_size(Size {.width = 70.0F, .height = 0.0F});

    ASSERT_TRUE(row->add_child(first));
    ASSERT_TRUE(row->add_child(second));
    ASSERT_TRUE(row->add_child(third));

    row->measure(Size {.width = 260.0F, .height = 100.0F});
    row->arrange(Size {.width = 260.0F, .height = 100.0F});

    EXPECT_FLOAT_EQ(second->bounds().x, 0.0F);
    EXPECT_FLOAT_EQ(first->bounds().x, 50.0F);
    EXPECT_FLOAT_EQ(first->bounds().width, 80.0F);
    EXPECT_FLOAT_EQ(third->bounds().width, 70.0F);
}

TEST(InputManagerTests, HitTestRoutingDispatchesCaptureTargetAndBubble) {
    auto root = std::make_shared<UIElement>("root");
    auto container = std::make_shared<UIElement>("container");
    auto leaf = std::make_shared<UIElement>("leaf");

    ASSERT_TRUE(root->add_child(container));
    ASSERT_TRUE(container->add_child(leaf));

    root->set_bounds(Rect {.x = 0.0F, .y = 0.0F, .width = 200.0F, .height = 200.0F});
    container->set_bounds(Rect {.x = 20.0F, .y = 20.0F, .width = 140.0F, .height = 140.0F});
    leaf->set_bounds(Rect {.x = 10.0F, .y = 10.0F, .width = 60.0F, .height = 40.0F});

    std::vector<std::string> route;
    int click_count = 0;

    root->set_event_handler([&](InputEvent&, EventPhase phase) {
        route.push_back("root-" + std::to_string(static_cast<int>(phase)));
    });
    container->set_event_handler([&](InputEvent&, EventPhase phase) {
        route.push_back("container-" + std::to_string(static_cast<int>(phase)));
    });
    leaf->set_event_handler([&](InputEvent& event, EventPhase phase) {
        route.push_back("leaf-" + std::to_string(static_cast<int>(phase)));
        if (phase == EventPhase::Target) {
            event.handled = true;
        }
    });

    InputManager input;
    input.set_click_handler([&](UIElement&) { ++click_count; });

    const bool handled = input.route_pointer_down(root, Point {.x = 40.0F, .y = 42.0F});

    EXPECT_TRUE(handled);
    EXPECT_EQ(click_count, 0);
    EXPECT_EQ(route, (std::vector<std::string> {
        "root-0",
        "container-0",
        "leaf-1",
    }));
}

TEST(FlexPanelTests, MeasureAggregatesWrappedRowsUsingLargestCrossContribution) {
    auto row = std::make_shared<FlexPanel>(FlexDirection::Row);
    auto top_left = std::make_shared<UIElement>("top_left");
    auto top_right = std::make_shared<UIElement>("top_right");
    auto bottom_left = std::make_shared<UIElement>("bottom_left");
    auto bottom_right = std::make_shared<UIElement>("bottom_right");

    row->set_wrap(FlexWrap::Wrap);
    row->set_column_gap(10.0F);

    top_left->set_desired_size(Size {.width = 80.0F, .height = 30.0F});
    top_right->set_desired_size(Size {.width = 110.0F, .height = 40.0F});
    bottom_left->set_desired_size(Size {.width = 90.0F, .height = 70.0F});
    bottom_right->set_desired_size(Size {.width = 60.0F, .height = 50.0F});

    ASSERT_TRUE(row->add_child(top_left));
    ASSERT_TRUE(row->add_child(top_right));
    ASSERT_TRUE(row->add_child(bottom_left));
    ASSERT_TRUE(row->add_child(bottom_right));

    const Size measured = row->measure(Size {.width = 220.0F, .height = 500.0F});

    EXPECT_FLOAT_EQ(measured.width, 200.0F);
    EXPECT_FLOAT_EQ(measured.height, 110.0F);
}

}  // namespace dcompframe::tests
