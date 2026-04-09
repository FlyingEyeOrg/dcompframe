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
