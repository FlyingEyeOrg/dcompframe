#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include "dcompframe/animation/animation_manager.h"
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
