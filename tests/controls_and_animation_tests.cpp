#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include "dcompframe/animation/animation_manager.h"
#include "dcompframe/binding/observable.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/controls/style.h"

namespace dcompframe::tests {

TEST(ControlsTests, ThemeResolvesAndButtonClickStateWorks) {
    Theme theme;
    Style accent {};
    accent.corner_radius = 12.0F;
    theme.set_style("button.primary", accent);

    const Style resolved = theme.resolve("button.primary");
    EXPECT_FLOAT_EQ(resolved.corner_radius, 12.0F);

    auto button = std::make_shared<Button>("Save");
    button->set_style(resolved);

    int click_count = 0;
    button->set_on_click([&click_count] { ++click_count; });

    EXPECT_TRUE(button->click());
    EXPECT_EQ(click_count, 1);

    button->set_state(ControlState::Disabled);
    EXPECT_FALSE(button->click());
    EXPECT_EQ(click_count, 1);
}

TEST(ControlsTests, CardStoresMetadataAndAction) {
    auto card = std::make_shared<Card>();
    auto action = std::make_shared<Button>("Open");

    card->set_title("Task");
    card->set_body("Complete the DirectComposition migration");
    card->set_icon("rocket");
    card->set_tags({"core", "ui", "high"});
    card->set_primary_action(action);

    EXPECT_EQ(card->title(), "Task");
    EXPECT_EQ(card->body(), "Complete the DirectComposition migration");
    EXPECT_EQ(card->icon(), "rocket");
    ASSERT_EQ(card->tags().size(), 3U);
    EXPECT_EQ(card->primary_action(), action);
    EXPECT_EQ(card->children().size(), 1U);
}

TEST(ControlsTests, ComboBoxStoresItemsAndSelectedText) {
    ComboBox combo_box;
    combo_box.set_items({"A", "B", "C"});
    combo_box.set_selected_index(1);

    ASSERT_TRUE(combo_box.selected_index().has_value());
    EXPECT_EQ(*combo_box.selected_index(), 1U);
    EXPECT_EQ(combo_box.selected_text(), "B");

    combo_box.set_selected_index(42U);
    EXPECT_FALSE(combo_box.selected_index().has_value());
    EXPECT_TRUE(combo_box.selected_text().empty());
}

TEST(ControlsTests, ItemsControlStoresItemsSelectionAndVisibleRange) {
    ItemsControl items_control;
    items_control.set_items({"One", "Two", "Three", "Four"});
    items_control.set_selected_index(2);
    items_control.set_item_spacing(6.0F);

    ASSERT_TRUE(items_control.selected_index().has_value());
    EXPECT_EQ(*items_control.selected_index(), 2U);

    const auto visible = items_control.visible_range(12.0F, 44.0F, 16.0F);
    EXPECT_EQ(visible.first, 0U);
    EXPECT_GE(visible.second, 2U);

    items_control.clear_items();
    EXPECT_TRUE(items_control.items().empty());
    EXPECT_FALSE(items_control.selected_index().has_value());
}

TEST(ControlsTests, TextBoxSupportsEditingSelectionAndTwoWayBinding) {
    TextBox text_box;
    Observable<std::string> title {"Alpha"};

    text_box.bind_text(title);
    text_box.move_caret_end();
    EXPECT_TRUE(text_box.insert_text(" Beta"));
    EXPECT_EQ(text_box.text(), "Alpha Beta");
    EXPECT_EQ(title.get(), "Alpha Beta");

    text_box.set_selection(6, 10);
    EXPECT_TRUE(text_box.has_selection());
    EXPECT_TRUE(text_box.insert_text("Gamma"));
    EXPECT_EQ(text_box.text(), "Alpha Gamma");
    EXPECT_EQ(text_box.caret_position(), 11U);

    text_box.move_caret_left();
    EXPECT_TRUE(text_box.backspace());
    EXPECT_EQ(text_box.text(), "Alpha Gama");

    text_box.select_all();
    EXPECT_TRUE(text_box.has_selection());
    text_box.clear_selection();
    EXPECT_FALSE(text_box.has_selection());
}

TEST(ControlsTests, RichTextBoxSupportsEditingSelectionAndCaretMovement) {
    RichTextBox rich_text_box;
    rich_text_box.set_rich_text("Line1\nLine2");
    rich_text_box.move_caret_end();
    EXPECT_TRUE(rich_text_box.insert_text("\nLine3"));
    EXPECT_EQ(rich_text_box.rich_text(), "Line1\nLine2\nLine3");

    rich_text_box.set_selection(6, 11);
    EXPECT_TRUE(rich_text_box.has_selection());
    EXPECT_TRUE(rich_text_box.insert_text("Body"));
    EXPECT_EQ(rich_text_box.rich_text(), "Line1\nBody\nLine3");

    rich_text_box.move_caret_left();
    EXPECT_TRUE(rich_text_box.backspace());
    EXPECT_EQ(rich_text_box.rich_text(), "Line1\nBoy\nLine3");

    rich_text_box.scroll_by(36.0F);
    EXPECT_FLOAT_EQ(rich_text_box.scroll_offset(), 36.0F);
}

TEST(ControlsTests, ListViewAndItemsControlTrackScrollOffsets) {
    ListView list_view;
    list_view.set_items({"A", "B", "C", "D", "E", "F"});
    list_view.scroll_by(24.0F);
    EXPECT_FLOAT_EQ(list_view.scroll_offset(), 24.0F);

    ItemsControl items_control;
    items_control.set_items({"One", "Two", "Three", "Four", "Five", "Six"});
    items_control.set_item_spacing(4.0F);
    items_control.scroll_by(18.0F);
    EXPECT_FLOAT_EQ(items_control.scroll_offset(), 18.0F);

    ScrollViewer scroll_viewer;
    EXPECT_FALSE(scroll_viewer.is_focusable());

    const auto visible = items_control.visible_range(items_control.scroll_offset(), 56.0F, 24.0F);
    EXPECT_LE(visible.first, visible.second);
}

TEST(ControlsTests, CheckBoxComboBoxAndSliderSupportInteractiveStateChanges) {
    CheckBox check_box;
    ComboBox combo_box;
    Slider slider;

    bool latest_checked = false;
    std::string latest_combo;
    float latest_value = 0.0F;

    check_box.set_on_checked_changed([&latest_checked](bool checked) { latest_checked = checked; });
    combo_box.set_items({"Overview", "Diagnostics", "Preview"});
    combo_box.set_on_selection_changed([&latest_combo](std::optional<std::size_t>, const std::string& value) { latest_combo = value; });
    slider.set_range(0.0F, 100.0F);
    slider.set_step(10.0F);
    slider.set_on_value_changed([&latest_value](float value) { latest_value = value; });

    EXPECT_TRUE(check_box.toggle());
    EXPECT_TRUE(latest_checked);

    combo_box.open_dropdown();
    EXPECT_TRUE(combo_box.is_dropdown_open());
    EXPECT_TRUE(combo_box.select_next());
    EXPECT_EQ(combo_box.selected_text(), "Overview");
    EXPECT_EQ(latest_combo, "Overview");
    EXPECT_TRUE(combo_box.select_next());
    EXPECT_EQ(combo_box.selected_text(), "Diagnostics");
    combo_box.close_dropdown();
    EXPECT_FALSE(combo_box.is_dropdown_open());

    slider.set_value_from_ratio(0.5F);
    EXPECT_FLOAT_EQ(slider.value(), 50.0F);
    EXPECT_TRUE(slider.step_by(1.0F));
    EXPECT_FLOAT_EQ(slider.value(), 60.0F);
    EXPECT_FLOAT_EQ(latest_value, 60.0F);
}

TEST(ControlsTests, TextAlignmentDefaultsToCenterExceptRichTextBox) {
    TextBox text_box;
    CheckBox check_box;
    ComboBox combo_box;
    RichTextBox rich_text_box;

    EXPECT_EQ(text_box.text_horizontal_alignment(), TextHorizontalAlignment::Center);
    EXPECT_EQ(text_box.text_vertical_alignment(), TextVerticalAlignment::Center);
    EXPECT_EQ(check_box.text_horizontal_alignment(), TextHorizontalAlignment::Center);
    EXPECT_EQ(check_box.text_vertical_alignment(), TextVerticalAlignment::Center);
    EXPECT_EQ(combo_box.text_horizontal_alignment(), TextHorizontalAlignment::Center);
    EXPECT_EQ(combo_box.text_vertical_alignment(), TextVerticalAlignment::Center);

    EXPECT_EQ(rich_text_box.text_horizontal_alignment(), TextHorizontalAlignment::Left);
    EXPECT_EQ(rich_text_box.text_vertical_alignment(), TextVerticalAlignment::Top);
}

TEST(ControlsTests, AdditionalControlsSupportCoreStateTransitions) {
    Label label("状态标签");
    Progress progress;
    Loading loading;
    LogBox log_box;
    TabControl tab_control;
    Popup popup;
    Expander expander;

    label.set_text("运行中");
    EXPECT_EQ(label.text(), "运行中");

    progress.set_range(0.0F, 100.0F);
    progress.set_value(135.0F);
    EXPECT_FLOAT_EQ(progress.value(), 100.0F);
    progress.set_indeterminate(true);
    EXPECT_TRUE(progress.is_indeterminate());

    loading.set_text("加载数据中...");
    loading.set_active(true);
    loading.set_overlay_mode(true);
    EXPECT_TRUE(loading.active());
    EXPECT_TRUE(loading.overlay_mode());
    EXPECT_EQ(loading.text(), "加载数据中...");

    log_box.set_max_lines(3);
    log_box.append_line("line-1");
    log_box.append_line("line-2");
    log_box.append_line("line-3");
    log_box.append_line("line-4");
    ASSERT_EQ(log_box.lines().size(), 3U);
    EXPECT_EQ(log_box.lines().front(), "line-2");
    log_box.set_auto_scroll(false);
    EXPECT_FALSE(log_box.auto_scroll());

    tab_control.set_tabs({"概览", "交互", "诊断"});
    ASSERT_TRUE(tab_control.selected_index().has_value());
    EXPECT_EQ(tab_control.selected_tab(), "概览");
    EXPECT_TRUE(tab_control.select_next());
    EXPECT_EQ(tab_control.selected_tab(), "交互");
    EXPECT_TRUE(tab_control.select_previous());
    EXPECT_EQ(tab_control.selected_tab(), "概览");

    popup.set_title("规范弹层");
    popup.set_body("用于验证 Popup 的基础状态和展示信息。");
    popup.set_modal(true);
    popup.set_open(true);
    EXPECT_TRUE(popup.is_modal());
    EXPECT_TRUE(popup.is_open());
    EXPECT_EQ(popup.title(), "规范弹层");

    expander.set_header("展开详情");
    expander.set_content_text("此区域用于展示更多控件说明");
    EXPECT_FALSE(expander.expanded());
    EXPECT_TRUE(expander.toggle());
    EXPECT_TRUE(expander.expanded());
}

TEST(ControlsTests, PanelArrangeStretchesChildrenToAvailableSize) {
    auto panel = std::make_shared<Panel>();
    auto child_a = std::make_shared<TextBlock>("A");
    auto child_b = std::make_shared<Button>("B");

    ASSERT_TRUE(panel->add_child(child_a));
    ASSERT_TRUE(panel->add_child(child_b));

    panel->arrange(Size {.width = 320.0F, .height = 180.0F});

    const auto panel_rect = panel->bounds();
    EXPECT_FLOAT_EQ(panel_rect.width, 320.0F);
    EXPECT_FLOAT_EQ(panel_rect.height, 180.0F);

    const auto a_rect = child_a->bounds();
    const auto b_rect = child_b->bounds();
    EXPECT_FLOAT_EQ(a_rect.x, 0.0F);
    EXPECT_FLOAT_EQ(a_rect.y, 0.0F);
    EXPECT_FLOAT_EQ(a_rect.width, 320.0F);
    EXPECT_FLOAT_EQ(a_rect.height, 180.0F);
    EXPECT_FLOAT_EQ(b_rect.width, 320.0F);
    EXPECT_FLOAT_EQ(b_rect.height, 180.0F);
}

TEST(AnimationTests, PropertyAnimationUpdatesElementAndCompletes) {
    AnimationManager animation_manager;
    auto element = std::make_shared<UIElement>("anim_target");

    element->set_bounds(Rect {.x = 0.0F, .y = 0.0F, .width = 100.0F, .height = 50.0F});
    element->set_opacity(0.0F);

    animation_manager.add(AnimationClip {
        .target = element,
        .property = AnimatedProperty::Opacity,
        .from = 0.0F,
        .to = 1.0F,
        .duration = std::chrono::milliseconds {100},
        .elapsed = std::chrono::milliseconds {0},
        .easing = EasingType::EaseInOut,
        .completed = false,
    });

    animation_manager.tick(std::chrono::milliseconds {50});
    EXPECT_GT(element->opacity(), 0.0F);
    EXPECT_LT(element->opacity(), 1.0F);

    animation_manager.tick(std::chrono::milliseconds {50});
    EXPECT_NEAR(element->opacity(), 1.0F, 0.0001F);
    EXPECT_EQ(animation_manager.active_count(), 0U);
    EXPECT_EQ(animation_manager.completed_count(), 1U);
}

TEST(AnimationTests, PositionAnimationChangesBounds) {
    AnimationManager animation_manager;
    auto element = std::make_shared<UIElement>("move_target");
    element->set_bounds(Rect {.x = 0.0F, .y = 20.0F, .width = 80.0F, .height = 40.0F});

    animation_manager.add(AnimationClip {
        .target = element,
        .property = AnimatedProperty::PositionX,
        .from = 0.0F,
        .to = 120.0F,
        .duration = std::chrono::milliseconds {120},
        .elapsed = std::chrono::milliseconds {0},
        .easing = EasingType::Linear,
        .completed = false,
    });

    animation_manager.tick(std::chrono::milliseconds {120});
    EXPECT_FLOAT_EQ(element->bounds().x, 120.0F);
}

}  // namespace dcompframe::tests
