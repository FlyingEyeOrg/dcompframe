#pragma once

#include <chrono>
#include <vector>

#include "dcompframe/ui_element.h"

namespace dcompframe {

enum class AnimatedProperty {
    PositionX,
    PositionY,
    Opacity,
    Rotation,
    ClipWidth,
    ClipHeight
};

enum class EasingType {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut
};

struct AnimationClip {
    std::weak_ptr<UIElement> target;
    AnimatedProperty property = AnimatedProperty::Opacity;
    float from = 0.0F;
    float to = 1.0F;
    std::chrono::milliseconds duration {250};
    std::chrono::milliseconds elapsed {0};
    EasingType easing = EasingType::Linear;
    bool completed = false;
};

class AnimationManager {
public:
    void add(AnimationClip clip);
    void tick(std::chrono::milliseconds delta);

    [[nodiscard]] std::size_t active_count() const;
    [[nodiscard]] std::size_t completed_count() const;

private:
    static float eval_easing(EasingType easing, float t);
    static void apply_animation_value(UIElement& element, AnimatedProperty property, float value);

    std::vector<AnimationClip> clips_;
    std::size_t completed_count_ = 0;
};

}  // namespace dcompframe
