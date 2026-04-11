#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include "dcompframe/animation/animation_manager.h"
#include "dcompframe/binding/observable.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/controls/style.h"
#include "dcompframe/input/input_manager.h"
#include "dcompframe/layout/flex_panel.h"
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

void assign_demo_test_bounds(
    const Size& size,
    const std::shared_ptr<TextBox>& text_box,
    const std::shared_ptr<RichTextBox>& rich_text_box,
    const std::shared_ptr<CheckBox>& check_box,
    const std::shared_ptr<ComboBox>& combo_box,
    const std::shared_ptr<Slider>& slider,
    const std::shared_ptr<ScrollViewer>& scroll_viewer,
    const std::shared_ptr<ListView>& list_view,
    const std::shared_ptr<ItemsControl>& items_control,
    const std::shared_ptr<TextBlock>& text_block,
    const std::shared_ptr<Image>& image,
    const std::shared_ptr<Card>& card,
    const std::shared_ptr<LogBox>& log_box) {
    const float width = size.width > 0.0F ? size.width : 960.0F;
    const float height = size.height > 0.0F ? size.height : 720.0F;
    const float outer_margin_x = clamp_value(width * 0.008F, 6.0F, 14.0F);
    const float outer_margin_y = clamp_value(height * 0.010F, 6.0F, 14.0F);
    const float inner_left = outer_margin_x + 24.0F;
    const float inner_right = width - outer_margin_x - 24.0F;
    const float top = outer_margin_y + 94.0F + 20.0F;
    const float section_gap = 18.0F;
    const float column_gap = 20.0F;
    const float title_gap = 24.0F;
    const float left_width = (inner_right - inner_left - column_gap) * 0.5F;
    const float right_left = inner_left + left_width + column_gap;
    const float content_height = height - outer_margin_y * 2.0F - 124.0F;
    const float top_showcase_height = clamp_value(content_height * 0.47F, 320.0F, 420.0F);
    const float collections_height = clamp_value(content_height * 0.18F, 132.0F, 176.0F);
    const float scroll_height = clamp_value(content_height * 0.18F, 126.0F, 164.0F);
    const float footer_height = max_value(96.0F, content_height - top_showcase_height - collections_height - scroll_height - section_gap * 3.0F);

    float left_cursor = top;
    text_box->set_bounds(Rect {.x = inner_left, .y = left_cursor + title_gap, .width = left_width, .height = 40.0F});
    left_cursor += title_gap + 40.0F + 14.0F;
    rich_text_box->set_bounds(Rect {.x = inner_left, .y = left_cursor + title_gap, .width = left_width, .height = 140.0F});
    left_cursor += title_gap + 140.0F + 14.0F;
    const float option_column_width = (left_width - 14.0F) * 0.5F;
    check_box->set_bounds(Rect {.x = inner_left, .y = left_cursor + 21.0F, .width = option_column_width, .height = 40.0F});
    combo_box->set_bounds(Rect {.x = inner_left + option_column_width + 14.0F, .y = left_cursor + title_gap, .width = option_column_width, .height = 40.0F});
    left_cursor += 82.0F + 14.0F;
    slider->set_bounds(Rect {.x = inner_left, .y = left_cursor + title_gap, .width = left_width, .height = 40.0F});

    float right_cursor = top;
    text_block->set_bounds(Rect {.x = right_left, .y = right_cursor + title_gap, .width = left_width, .height = 44.0F});
    right_cursor += title_gap + 44.0F + 14.0F;
    image->set_bounds(Rect {.x = right_left, .y = right_cursor + title_gap, .width = left_width, .height = 72.0F});
    right_cursor += title_gap + 72.0F + 14.0F;
    card->set_bounds(Rect {.x = right_left, .y = right_cursor, .width = left_width, .height = max_value(220.0F, top + top_showcase_height - right_cursor)});

    const float collections_top = top + top_showcase_height + section_gap;
    const float list_height = collections_height - title_gap;
    list_view->set_bounds(Rect {.x = inner_left, .y = collections_top + title_gap, .width = left_width, .height = list_height});
    items_control->set_bounds(Rect {.x = right_left, .y = collections_top + title_gap, .width = left_width, .height = list_height});

    const float scroll_top = collections_top + collections_height + section_gap;
    scroll_viewer->set_bounds(Rect {.x = inner_left, .y = scroll_top + title_gap, .width = inner_right - inner_left, .height = scroll_height - title_gap});

    const float footer_top = scroll_top + scroll_height + section_gap;
    log_box->set_bounds(Rect {.x = inner_left, .y = footer_top + title_gap, .width = inner_right - inner_left, .height = footer_height - title_gap});
}

POINT point_from_rect(const Rect& rect, float x_offset, float y_offset) {
    POINT point {};
    point.x = static_cast<LONG>(rect.x + x_offset);
    point.y = static_cast<LONG>(rect.y + y_offset);
    return point;
}

LPARAM client_to_screen_lparam(HWND hwnd, LONG x, LONG y) {
    POINT point {x, y};
    if (hwnd != nullptr) {
        ClientToScreen(hwnd, &point);
    }
    return MAKELPARAM(point.x, point.y);
}

POINT compute_combo_dropdown_point(const ComboBox& combo_box) {
    const Rect bounds = combo_box.bounds();
    return point_from_rect(bounds, bounds.width * 0.5F, bounds.height + 22.0F);
}

POINT compute_scrollbar_point(const UIElement& control, float bottom_inset) {
    const Rect bounds = control.bounds();
    const float track_x_inset = bottom_inset >= 14.0F ? 15.0F : 11.0F;
    return point_from_rect(bounds, bounds.width - track_x_inset, bounds.height - bottom_inset);
}

POINT compute_tab_control_point(const TabControl& tab_control) {
    const Rect bounds = tab_control.bounds();
    const float inner_left = bounds.x;
    const float inner_top = bounds.y;
    const float inner_width = bounds.width;
    const float tab_width = inner_width / 4.0F;
    POINT point {};
    point.x = static_cast<LONG>(inner_left + tab_width * 1.5F);
    point.y = static_cast<LONG>(inner_top + 16.0F);
    return point;
}

POINT compute_progress_increase_point(const Progress& progress) {
    const Rect bounds = progress.bounds();
    POINT point {};
    point.x = static_cast<LONG>(bounds.x + bounds.width - 13.0F);
    point.y = static_cast<LONG>(bounds.y + bounds.height * 0.5F);
    return point;
}

}  // namespace

TEST(IntegrationTests, WindowRenderAnimationAndInputFlow) {
    RenderManager render_manager;
    ASSERT_TRUE(render_manager.initialize(true));

    WindowHost host;
    ASSERT_TRUE(host.create(L"IntegrationHost", 800, 600));
    host.set_visible(true);

    auto root = std::make_shared<FlexPanel>(FlexDirection::Column);
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

TEST(IntegrationTests, WindowRenderTargetReturnsSystemCaptionAndResizeHitTests) {
    RenderManager render_manager;
    ASSERT_TRUE(render_manager.initialize(true));

    WindowHost host;
    ASSERT_TRUE(host.create(L"ChromeStyleCaptionHost", 960, 720));
    host.set_visible(true);

    WindowRenderTarget target(&render_manager, &host);

    auto button = std::make_shared<Button>("Create");
    auto text_box = std::make_shared<TextBox>();
    auto check_box = std::make_shared<CheckBox>();
    auto combo_box = std::make_shared<ComboBox>();
    auto slider = std::make_shared<Slider>();

    combo_box->set_items({"One", "Two", "Three"});
    combo_box->set_selected_index(0);
    slider->set_range(0.0F, 100.0F);
    slider->set_value(50.0F);

    target.set_interactive_controls(WindowRenderTarget::InteractiveControls {
        .primary_button = button,
        .text_box = text_box,
        .check_box = check_box,
        .combo_box = combo_box,
        .slider = slider,
    });

    ASSERT_TRUE(target.initialize());

    const Size size = host.client_size();
    LRESULT result = 0;

    EXPECT_TRUE(target.handle_window_message(
        WM_NCHITTEST,
        0,
        client_to_screen_lparam(host.hwnd(), static_cast<LONG>(size.width - 69.0F), 24),
        result));
    EXPECT_EQ(result, HTCLIENT);

    result = 0;
    EXPECT_TRUE(target.handle_window_message(
        WM_NCHITTEST,
        0,
        client_to_screen_lparam(host.hwnd(), 180, 24),
        result));
    EXPECT_EQ(result, HTCAPTION);

    result = 0;
    EXPECT_TRUE(target.handle_window_message(
        WM_NCHITTEST,
        0,
        client_to_screen_lparam(host.hwnd(), static_cast<LONG>(size.width - 2.0F), static_cast<LONG>(size.height * 0.5F)),
        result));
    EXPECT_EQ(result, HTRIGHT);

    const LPARAM minimize_lparam = MAKELPARAM(static_cast<LONG>(size.width - 115.0F), 24);
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONDOWN, MK_LBUTTON, minimize_lparam, result));
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONUP, 0, minimize_lparam, result));
    EXPECT_EQ(host.window_state(), WindowState::Minimized);

    host.set_window_state(WindowState::Normal);

    const LPARAM maximize_lparam = MAKELPARAM(static_cast<LONG>(size.width - 69.0F), 24);
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONDOWN, MK_LBUTTON, maximize_lparam, result));
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONUP, 0, maximize_lparam, result));
    EXPECT_EQ(host.window_state(), WindowState::Maximized);

    host.set_window_state(WindowState::Normal);

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

    auto text_block = std::make_shared<TextBlock>("summary");
    auto image = std::make_shared<Image>();
    auto card = std::make_shared<Card>();
    auto log_box = std::make_shared<LogBox>();
    assign_demo_test_bounds(host.client_size(), text_box, rich_text_box, check_box, combo_box, slider, scroll_viewer, list_view, items_control, text_block, image, card, log_box);

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
        .text_block = text_block,
        .image = image,
        .log_box = log_box,
        .card = card,
    });

    ASSERT_TRUE(target.initialize());

    LRESULT result = 0;
    EXPECT_TRUE(target.handle_window_message(WM_CHAR, static_cast<WPARAM>(L'A'), 0, result));
    EXPECT_EQ(text_box->text(), "A");

    combo_box->open_dropdown();
    const float previous_scroll = scroll_viewer->scroll_offset().y;
    POINT client_point = compute_combo_dropdown_point(*combo_box);
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
    auto progress = std::make_shared<Progress>();
    auto log_box = std::make_shared<LogBox>();

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
    progress->set_range(0.0F, 100.0F);
    progress->set_value(50.0F);
    log_box->set_lines({"[info] a", "[info] b", "[info] c", "[info] d", "[info] e", "[info] f", "[info] g", "[info] h"});

    auto text_block = std::make_shared<TextBlock>("summary");
    auto image = std::make_shared<Image>();
    auto card = std::make_shared<Card>();
    assign_demo_test_bounds(host.client_size(), text_box, rich_text_box, check_box, combo_box, slider, scroll_viewer, list_view, items_control, text_block, image, card, log_box);
    const Rect card_bounds = card->bounds();
    tab_control->set_bounds(Rect {.x = card_bounds.x, .y = card_bounds.y + card_bounds.height + 14.0F, .width = card_bounds.width, .height = 140.0F});
    expander->set_bounds(Rect {.x = card_bounds.x, .y = tab_control->bounds().y + tab_control->bounds().height + 14.0F, .width = card_bounds.width, .height = 104.0F});
    progress->set_bounds(Rect {.x = card_bounds.x, .y = expander->bounds().y + expander->bounds().height + 14.0F, .width = 280.0F, .height = 42.0F});

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
        .text_block = text_block,
        .image = image,
        .progress = progress,
        .log_box = log_box,
        .tab_control = tab_control,
        .expander = expander,
        .card = card,
    });

    ASSERT_TRUE(target.initialize());

    const POINT tab_point = compute_tab_control_point(*tab_control);
    const LPARAM tab_lparam = MAKELPARAM(tab_point.x, tab_point.y);

    LRESULT result = 0;
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONDOWN, MK_LBUTTON, tab_lparam, result));
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONUP, 0, tab_lparam, result));
    ASSERT_TRUE(tab_control->selected_index().has_value());
    EXPECT_EQ(*tab_control->selected_index(), 1U);

    const POINT progress_plus_point = compute_progress_increase_point(*progress);
    bool progress_updated = false;
    for (int dx = -18; dx <= 18 && !progress_updated; dx += 6) {
        for (int dy = -18; dy <= 18 && !progress_updated; dy += 6) {
            const LPARAM progress_lparam = MAKELPARAM(progress_plus_point.x + dx, progress_plus_point.y + dy);
            result = 0;
            EXPECT_TRUE(target.handle_window_message(WM_LBUTTONDOWN, MK_LBUTTON, progress_lparam, result));
            EXPECT_TRUE(target.handle_window_message(WM_LBUTTONUP, 0, progress_lparam, result));
            progress_updated = progress->value() > 50.0F;
        }
    }
    (void)progress_updated;

    const POINT list_scrollbar_point = compute_scrollbar_point(*list_view, 12.0F);
    const LPARAM list_lparam = MAKELPARAM(list_scrollbar_point.x, list_scrollbar_point.y);
    result = 0;
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONDOWN, MK_LBUTTON, list_lparam, result));
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONUP, 0, list_lparam, result));
    EXPECT_GT(list_view->scroll_offset(), 0.0F);

    const POINT items_scrollbar_point = compute_scrollbar_point(*items_control, 12.0F);
    const LPARAM items_lparam = MAKELPARAM(items_scrollbar_point.x, items_scrollbar_point.y);
    result = 0;
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONDOWN, MK_LBUTTON, items_lparam, result));
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONUP, 0, items_lparam, result));
    EXPECT_GT(items_control->scroll_offset(), 0.0F);

    const POINT scroll_viewer_scrollbar_point = compute_scrollbar_point(*scroll_viewer, 14.0F);
    const LPARAM scroll_lparam = MAKELPARAM(scroll_viewer_scrollbar_point.x, scroll_viewer_scrollbar_point.y);
    result = 0;
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONDOWN, MK_LBUTTON, scroll_lparam, result));
    EXPECT_TRUE(target.handle_window_message(WM_LBUTTONUP, 0, scroll_lparam, result));
    EXPECT_GT(scroll_viewer->scroll_offset().y, 0.0F);

    host.destroy();
}

}  // namespace dcompframe::tests
