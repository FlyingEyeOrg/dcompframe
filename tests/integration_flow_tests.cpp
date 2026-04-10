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

namespace {

template <typename T>
T min_value(const T& left, const T& right) {
    return left < right ? left : right;
}

template <typename T>
T max_value(const T& left, const T& right) {
    return left > right ? left : right;
}

template <typename T>
T clamp_value(const T& value, const T& low, const T& high) {
    return min_value(max_value(value, low), high);
}

POINT compute_combo_dropdown_point(const Size& size) {
    const float width = size.width > 0.0F ? size.width : 1280.0F;
    const float height = size.height > 0.0F ? size.height : 720.0F;
    const float outer_margin_x = clamp_value(width * 0.008F, 6.0F, 14.0F);
    const float outer_margin_y = clamp_value(height * 0.010F, 6.0F, 14.0F);
    const float card_top = outer_margin_y;
    const float card_bottom = height - outer_margin_y;
    const float inner_left = outer_margin_x + 24.0F;
    const float inner_right = width - outer_margin_x - 24.0F;
    const float content_top = card_top + 96.0F;
    const float top = content_top + 8.0F;
    const float column_gap = 20.0F;
    const float left_width = (inner_right - inner_left - column_gap) * 0.47F;
    const float label_height = 18.0F;
    const float control_height = 40.0F;
    const float control_gap = 14.0F;
    const float section_gap = 14.0F;
    const float footer_height = 26.0F;
    const float content_height = (card_bottom - 14.0F) - content_top - 8.0F;
    const float scroll_section_height = clamp_value(content_height * 0.18F, 120.0F, 168.0F);
    const float list_section_height = clamp_value(content_height * 0.24F, 140.0F, 190.0F);
    const float upper_bottom = top + max_value(300.0F, content_height - scroll_section_height - list_section_height - footer_height - section_gap * 3.0F);

    float left_cursor = top;
    left_cursor += label_height + 6.0F + control_height + control_gap;
    left_cursor += label_height + 6.0F;
    const float left_remaining_fixed = control_height + control_gap + (label_height + 6.0F) + control_height + control_gap + (label_height + 6.0F) + control_height;
    const float rich_text_height = max_value(108.0F, upper_bottom - left_cursor - left_remaining_fixed);
    left_cursor += rich_text_height + control_gap;
    left_cursor += control_height + control_gap;
    left_cursor += label_height + 6.0F;

    const float combo_left = inner_left;
    const float combo_top = left_cursor;
    const float combo_right = inner_left + left_width;
    const float dropdown_top = combo_top + control_height + 4.0F;

    POINT point {};
    point.x = static_cast<LONG>((combo_left + combo_right) * 0.5F);
    point.y = static_cast<LONG>(dropdown_top + 18.0F);
    return point;
}

POINT compute_list_scrollbar_point(const Size& size) {
    const float width = size.width > 0.0F ? size.width : 1280.0F;
    const float height = size.height > 0.0F ? size.height : 720.0F;
    const float outer_margin_x = clamp_value(width * 0.008F, 6.0F, 14.0F);
    const float outer_margin_y = clamp_value(height * 0.010F, 6.0F, 14.0F);
    const float card_top = outer_margin_y;
    const float card_bottom = height - outer_margin_y;
    const float inner_left = outer_margin_x + 24.0F;
    const float inner_right = width - outer_margin_x - 24.0F;
    const float content_top = card_top + 96.0F;
    const float top = content_top + 8.0F;
    const float column_gap = 20.0F;
    const float section_gap = 14.0F;
    const float label_height = 18.0F;
    const float footer_height = 26.0F;
    const float content_height = (card_bottom - 14.0F) - content_top - 8.0F;
    const float scroll_section_height = clamp_value(content_height * 0.18F, 120.0F, 168.0F);
    const float list_section_height = clamp_value(content_height * 0.24F, 140.0F, 190.0F);
    const float upper_bottom = top + max_value(300.0F, content_height - scroll_section_height - list_section_height - footer_height - section_gap * 3.0F);
    const float list_section_top = upper_bottom + section_gap;
    const float list_section_bottom = list_section_top + list_section_height;
    const float list_column_width = (inner_right - inner_left - column_gap) * 0.5F;
    const float list_top = list_section_top + label_height + 8.0F;
    const float viewport_top = list_top + 8.0F;
    const float viewport_right = inner_left + list_column_width - 8.0F;

    POINT point {};
    point.x = static_cast<LONG>(viewport_right - 3.0F);
    point.y = static_cast<LONG>(list_section_bottom - 12.0F);
    return point;
}

POINT compute_items_scrollbar_point(const Size& size) {
    const POINT list_point = compute_list_scrollbar_point(size);
    const float width = size.width > 0.0F ? size.width : 1280.0F;
    const float outer_margin_x = clamp_value(width * 0.008F, 6.0F, 14.0F);
    const float inner_left = outer_margin_x + 24.0F;
    const float inner_right = width - outer_margin_x - 24.0F;
    const float column_gap = 20.0F;
    const float list_column_width = (inner_right - inner_left - column_gap) * 0.5F;

    POINT point = list_point;
    point.x = static_cast<LONG>(inner_right - 8.0F - 3.0F);
    return point;
}

POINT compute_scroll_viewer_scrollbar_point(const Size& size) {
    const float width = size.width > 0.0F ? size.width : 1280.0F;
    const float height = size.height > 0.0F ? size.height : 720.0F;
    const float outer_margin_x = clamp_value(width * 0.008F, 6.0F, 14.0F);
    const float outer_margin_y = clamp_value(height * 0.010F, 6.0F, 14.0F);
    const float card_top = outer_margin_y;
    const float card_bottom = height - outer_margin_y;
    const float inner_left = outer_margin_x + 24.0F;
    const float inner_right = width - outer_margin_x - 24.0F;
    const float content_top = card_top + 96.0F;
    const float top = content_top + 8.0F;
    const float section_gap = 14.0F;
    const float label_height = 18.0F;
    const float footer_height = 26.0F;
    const float content_height = (card_bottom - 14.0F) - content_top - 8.0F;
    const float scroll_section_height = clamp_value(content_height * 0.18F, 120.0F, 168.0F);
    const float list_section_height = clamp_value(content_height * 0.24F, 140.0F, 190.0F);
    const float upper_bottom = top + max_value(300.0F, content_height - scroll_section_height - list_section_height - footer_height - section_gap * 3.0F);
    const float list_section_top = upper_bottom + section_gap;
    const float list_section_bottom = list_section_top + list_section_height;
    const float scroll_section_top = list_section_bottom + section_gap;
    const float scroll_viewer_top = scroll_section_top + label_height + 8.0F;
    const float viewport_right = inner_right - 12.0F;

    POINT point {};
    point.x = static_cast<LONG>(viewport_right - 3.0F);
    point.y = static_cast<LONG>(scroll_viewer_top + scroll_section_height - 14.0F);
    return point;
}

POINT compute_tab_control_point(const Size& size) {
    const float width = size.width > 0.0F ? size.width : 1280.0F;
    const float height = size.height > 0.0F ? size.height : 720.0F;
    const float outer_margin_x = clamp_value(width * 0.008F, 6.0F, 14.0F);
    const float outer_margin_y = clamp_value(height * 0.010F, 6.0F, 14.0F);
    const float card_top = outer_margin_y;
    const float card_bottom = height - outer_margin_y;
    const float inner_left = outer_margin_x + 24.0F;
    const float inner_right = width - outer_margin_x - 24.0F;
    const float content_top = card_top + 96.0F;
    const float top = content_top + 8.0F;
    const float column_gap = 20.0F;
    const float left_width = (inner_right - inner_left - column_gap) * 0.47F;
    const float label_height = 18.0F;
    const float control_gap = 14.0F;
    const float section_gap = 14.0F;
    const float footer_height = 26.0F;
    const float content_height = (card_bottom - 14.0F) - content_top - 8.0F;
    const float scroll_section_height = clamp_value(content_height * 0.18F, 120.0F, 168.0F);
    const float list_section_height = clamp_value(content_height * 0.24F, 140.0F, 190.0F);
    const float upper_section_height = max_value(300.0F, content_height - scroll_section_height - list_section_height - footer_height - section_gap * 3.0F);
    const float right_column_left = inner_left + left_width + column_gap;
    const float fixed_right_spacing = (label_height + 6.0F) * 2.0F + control_gap * 2.0F;
    const float right_body_height = max_value(196.0F, upper_section_height - fixed_right_spacing);
    const float text_block_height = clamp_value(right_body_height * 0.14F, 36.0F, 56.0F);
    const float image_height = clamp_value(right_body_height * 0.18F, 48.0F, 78.0F);
    const float card_preview_top = top + (label_height + 6.0F) + text_block_height + control_gap + (label_height + 6.0F) + image_height + control_gap;
    const float tab_top = card_preview_top + 10.0F;
    const float tab_left = right_column_left + 10.0F;
    const float tab_right = inner_right - 10.0F;
    const float tab_width = (tab_right - tab_left) / 4.0F;

    POINT point {};
    point.x = static_cast<LONG>(tab_left + tab_width * 1.5F);
    point.y = static_cast<LONG>(tab_top + 15.0F);
    return point;
}

}  // namespace

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

TEST(IntegrationTests, WindowRenderTargetProcessesTextInputAndConsumesComboWheel) {
    RenderManager render_manager;
    ASSERT_TRUE(render_manager.initialize(true));

    WindowHost host;
    ASSERT_TRUE(host.create(L"InteractiveRoutingHost", 960, 720));
    host.set_visible(true);

    WindowRenderTarget target(&render_manager, &host);

    auto button = std::make_shared<Button>("Create");
    auto text_box = std::make_shared<TextBox>();
    auto rich_text_box = std::make_shared<RichTextBox>();
    auto check_box = std::make_shared<CheckBox>();
    auto combo_box = std::make_shared<ComboBox>();
    auto slider = std::make_shared<Slider>();
    auto scroll_viewer = std::make_shared<ScrollViewer>();
    auto list_view = std::make_shared<ListView>();
    auto items_control = std::make_shared<ItemsControl>();

    combo_box->set_items({
        "Option 01", "Option 02", "Option 03", "Option 04", "Option 05",
        "Option 06", "Option 07", "Option 08", "Option 09", "Option 10"});
    combo_box->set_selected_index(0);
    slider->set_range(0.0F, 100.0F);
    slider->set_value(50.0F);

    auto scroll_content = std::make_shared<ItemsControl>();
    scroll_content->set_items({
        "Scroll 01", "Scroll 02", "Scroll 03", "Scroll 04", "Scroll 05",
        "Scroll 06", "Scroll 07", "Scroll 08", "Scroll 09", "Scroll 10"});
    scroll_viewer->set_content(scroll_content);
    scroll_viewer->set_scroll_offset(0.0F, 32.0F);

    target.set_interactive_controls(WindowRenderTarget::InteractiveControls {
        .primary_button = button,
        .text_box = text_box,
        .rich_text_box = rich_text_box,
        .check_box = check_box,
        .combo_box = combo_box,
        .slider = slider,
        .scroll_viewer = scroll_viewer,
        .list_view = list_view,
        .items_control = items_control,
    });

    ASSERT_TRUE(target.initialize());

    LRESULT result = 0;
    EXPECT_TRUE(target.handle_window_message(WM_CHAR, static_cast<WPARAM>(L'A'), 0, result));
    EXPECT_EQ(text_box->text(), "A");

    combo_box->open_dropdown();
    const float previous_scroll = scroll_viewer->scroll_offset().y;
    POINT client_point = compute_combo_dropdown_point(host.client_size());
    ASSERT_TRUE(ClientToScreen(host.hwnd(), &client_point));
    const LPARAM lparam = MAKELPARAM(client_point.x, client_point.y);
    result = 0;
    EXPECT_TRUE(target.handle_window_message(WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), lparam, result));
    EXPECT_FLOAT_EQ(scroll_viewer->scroll_offset().y, previous_scroll);

    host.destroy();
}

TEST(IntegrationTests, WindowRenderTargetProcessesTabExpanderAndScrollbarTrackClicks) {
    RenderManager render_manager;
    ASSERT_TRUE(render_manager.initialize(true));

    WindowHost host;
    ASSERT_TRUE(host.create(L"InteractiveControlsHost", 960, 720));
    host.set_visible(true);

    WindowRenderTarget target(&render_manager, &host);

    auto button = std::make_shared<Button>("Create");
    auto text_box = std::make_shared<TextBox>();
    auto rich_text_box = std::make_shared<RichTextBox>();
    auto check_box = std::make_shared<CheckBox>();
    auto combo_box = std::make_shared<ComboBox>();
    auto slider = std::make_shared<Slider>();
    auto scroll_viewer = std::make_shared<ScrollViewer>();
    auto list_view = std::make_shared<ListView>();
    auto items_control = std::make_shared<ItemsControl>();
    auto tab_control = std::make_shared<TabControl>();
    auto expander = std::make_shared<Expander>();

    combo_box->set_items({"One", "Two", "Three", "Four"});
    combo_box->set_selected_index(0);
    slider->set_range(0.0F, 100.0F);
    slider->set_value(50.0F);

    std::vector<std::string> dense_items;
    dense_items.reserve(24);
    for (int index = 1; index <= 24; ++index) {
        dense_items.push_back("Item " + std::to_string(index));
    }
    list_view->set_items(dense_items);
    items_control->set_items(dense_items);
    items_control->set_item_spacing(6.0F);

    auto scroll_content = std::make_shared<ItemsControl>();
    scroll_content->set_items(dense_items);
    scroll_content->set_item_spacing(6.0F);
    scroll_viewer->set_content(scroll_content);

    tab_control->set_tabs({"概览", "动画", "数据", "诊断"});
    tab_control->set_selected_index(0);
    expander->set_header("说明");
    expander->set_content_text("用于测试展开/收起交互");
    expander->set_expanded(false);

    target.set_interactive_controls(WindowRenderTarget::InteractiveControls {
        .primary_button = button,
        .text_box = text_box,
        .rich_text_box = rich_text_box,
        .check_box = check_box,
        .combo_box = combo_box,
        .slider = slider,
        .scroll_viewer = scroll_viewer,
        .list_view = list_view,
        .items_control = items_control,
        .tab_control = tab_control,
        .expander = expander,
    });

    ASSERT_TRUE(target.initialize());

    const Size client_size = host.client_size();
    const POINT tab_point = compute_tab_control_point(client_size);
    const LPARAM tab_lparam = MAKELPARAM(tab_point.x, tab_point.y);

    LRESULT result = 0;
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONDOWN, MK_LBUTTON, tab_lparam, result));
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONUP, 0, tab_lparam, result));
    ASSERT_TRUE(tab_control->selected_index().has_value());
    EXPECT_EQ(*tab_control->selected_index(), 1U);

    const POINT list_scrollbar_point = compute_list_scrollbar_point(client_size);
    const LPARAM list_lparam = MAKELPARAM(list_scrollbar_point.x, list_scrollbar_point.y);
    result = 0;
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONDOWN, MK_LBUTTON, list_lparam, result));
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONUP, 0, list_lparam, result));
    EXPECT_GT(list_view->scroll_offset(), 0.0F);

    const POINT items_scrollbar_point = compute_items_scrollbar_point(client_size);
    const LPARAM items_lparam = MAKELPARAM(items_scrollbar_point.x, items_scrollbar_point.y);
    result = 0;
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONDOWN, MK_LBUTTON, items_lparam, result));
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONUP, 0, items_lparam, result));
    EXPECT_GT(items_control->scroll_offset(), 0.0F);

    const POINT scroll_viewer_scrollbar_point = compute_scroll_viewer_scrollbar_point(client_size);
    const LPARAM scroll_lparam = MAKELPARAM(scroll_viewer_scrollbar_point.x, scroll_viewer_scrollbar_point.y);
    result = 0;
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONDOWN, MK_LBUTTON, scroll_lparam, result));
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONUP, 0, scroll_lparam, result));
    EXPECT_GT(scroll_viewer->scroll_offset().y, 0.0F);

    host.destroy();
}

}  // namespace dcompframe::tests
