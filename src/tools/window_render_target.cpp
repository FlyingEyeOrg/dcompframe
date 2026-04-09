#include "dcompframe/tools/window_render_target.h"

namespace dcompframe {

WindowRenderTarget::WindowRenderTarget(RenderManager* render_manager, WindowHost* window_host)
    : render_manager_(render_manager),
      window_host_(window_host),
      bridge_(render_manager != nullptr ? render_manager->create_composition_bridge() : CompositionBridge(nullptr)) {}

bool WindowRenderTarget::initialize() {
    if (render_manager_ == nullptr || window_host_ == nullptr || !render_manager_->is_initialized()) {
        return false;
    }

    ready_ = bridge_.bind_target_handle(window_host_->hwnd());
    return ready_;
}

bool WindowRenderTarget::render_frame(bool has_dirty_changes) {
    if (!ready_) {
        return false;
    }

    const bool committed = bridge_.commit_changes(has_dirty_changes);
    if (committed) {
        ++presented_frames_;
    }

    return committed;
}

bool WindowRenderTarget::is_ready() const {
    return ready_;
}

int WindowRenderTarget::presented_frames() const {
    return presented_frames_;
}

}  // namespace dcompframe
