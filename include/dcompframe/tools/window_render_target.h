#pragma once

#include "dcompframe/render_manager.h"
#include "dcompframe/window_host.h"

namespace dcompframe {

class WindowRenderTarget {
public:
    WindowRenderTarget(RenderManager* render_manager, WindowHost* window_host);

    bool initialize();
    bool render_frame(bool has_dirty_changes = true);

    [[nodiscard]] bool is_ready() const;
    [[nodiscard]] int presented_frames() const;

private:
    RenderManager* render_manager_ = nullptr;
    WindowHost* window_host_ = nullptr;
    CompositionBridge bridge_ {nullptr};
    bool ready_ = false;
    int presented_frames_ = 0;
};

}  // namespace dcompframe
