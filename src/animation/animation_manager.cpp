#include "dcompframe/animation/animation_manager.h"

#include <algorithm>

namespace dcompframe {

void AnimationManager::add(AnimationClip clip) {
    clip.elapsed = std::chrono::milliseconds {0};
    clip.completed = false;
    clips_.push_back(std::move(clip));
}

void AnimationManager::tick(std::chrono::milliseconds delta) {
    for (auto& clip : clips_) {
        if (clip.completed) {
            continue;
        }

        auto target = clip.target.lock();
        if (!target) {
            clip.completed = true;
            ++completed_count_;
            continue;
        }

        clip.elapsed += delta;
        if (clip.elapsed >= clip.duration) {
            clip.elapsed = clip.duration;
            clip.completed = true;
            ++completed_count_;
        }

        const float t = clip.duration.count() == 0
            ? 1.0F
            : static_cast<float>(clip.elapsed.count()) / static_cast<float>(clip.duration.count());
        const float eased = eval_easing(clip.easing, std::clamp(t, 0.0F, 1.0F));
        const float value = clip.from + (clip.to - clip.from) * eased;
        apply_animation_value(*target, clip.property, value);
    }

    clips_.erase(
        std::remove_if(clips_.begin(), clips_.end(), [](const AnimationClip& clip) { return clip.completed; }),
        clips_.end());
}

std::size_t AnimationManager::active_count() const {
    return clips_.size();
}

std::size_t AnimationManager::completed_count() const {
    return completed_count_;
}

float AnimationManager::eval_easing(EasingType easing, float t) {
    switch (easing) {
    case EasingType::Linear:
        return t;
    case EasingType::EaseIn:
        return t * t;
    case EasingType::EaseOut:
        return 1.0F - (1.0F - t) * (1.0F - t);
    case EasingType::EaseInOut:
        if (t < 0.5F) {
            return 2.0F * t * t;
        }

        return 1.0F - 2.0F * (1.0F - t) * (1.0F - t);
    }

    return t;
}

void AnimationManager::apply_animation_value(UIElement& element, AnimatedProperty property, float value) {
    const Rect bounds = element.bounds();
    const Rect clip = element.clip_rect();

    switch (property) {
    case AnimatedProperty::PositionX:
        element.set_bounds(Rect {.x = value, .y = bounds.y, .width = bounds.width, .height = bounds.height});
        break;
    case AnimatedProperty::PositionY:
        element.set_bounds(Rect {.x = bounds.x, .y = value, .width = bounds.width, .height = bounds.height});
        break;
    case AnimatedProperty::Opacity:
        element.set_opacity(value);
        break;
    case AnimatedProperty::Rotation:
        element.set_transform(element.translation().x, element.translation().y, element.scale().x, element.scale().y, value);
        break;
    case AnimatedProperty::ClipWidth:
        element.set_clip_rect(Rect {.x = clip.x, .y = clip.y, .width = value, .height = clip.height});
        break;
    case AnimatedProperty::ClipHeight:
        element.set_clip_rect(Rect {.x = clip.x, .y = clip.y, .width = clip.width, .height = value});
        break;
    }
}

}  // namespace dcompframe
