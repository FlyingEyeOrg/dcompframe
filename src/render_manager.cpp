#include "dcompframe/render_manager.h"

namespace dcompframe {

CompositionBridge::CompositionBridge(RenderManager* owner) : owner_(owner) {}

bool CompositionBridge::bind_target_handle(HWND hwnd) {
    if (hwnd == nullptr) {
        return false;
    }

    bound_hwnd_ = hwnd;
    return true;
}

bool CompositionBridge::commit_changes(bool has_dirty_changes) {
    if (owner_ == nullptr || !owner_->is_initialized() || bound_hwnd_ == nullptr) {
        return false;
    }

    if (!has_dirty_changes) {
        return false;
    }

    ++commit_count_;
    owner_->notify_commit();
    return true;
}

int CompositionBridge::commit_count() const {
    return commit_count_;
}

HWND CompositionBridge::bound_hwnd() const {
    return bound_hwnd_;
}

bool RenderManager::initialize(bool simulate_device_available) {
    initialized_ = simulate_device_available;
    return initialized_;
}

void RenderManager::shutdown() {
    initialized_ = false;
    total_commit_count_ = 0;
}

bool RenderManager::is_initialized() const {
    return initialized_;
}

CompositionBridge RenderManager::create_composition_bridge() {
    return CompositionBridge(this);
}

int RenderManager::total_commit_count() const {
    return total_commit_count_;
}

void RenderManager::notify_commit() {
    ++total_commit_count_;
}

}  // namespace dcompframe
