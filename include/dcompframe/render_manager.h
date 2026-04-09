#pragma once

#include <cstdint>
#include <windows.h>

namespace dcompframe {

class RenderManager;

class CompositionBridge {
public:
    explicit CompositionBridge(RenderManager* owner);

    bool bind_target_handle(HWND hwnd);
    bool commit_changes(bool has_dirty_changes = true);

    [[nodiscard]] int commit_count() const;
    [[nodiscard]] HWND bound_hwnd() const;

private:
    RenderManager* owner_ = nullptr;
    HWND bound_hwnd_ = nullptr;
    int commit_count_ = 0;
};

class RenderManager {
public:
    bool initialize(bool simulate_device_available = true);
    void shutdown();

    [[nodiscard]] bool is_initialized() const;
    [[nodiscard]] CompositionBridge create_composition_bridge();
    [[nodiscard]] int total_commit_count() const;

    void notify_commit();

private:
    bool initialized_ = false;
    int total_commit_count_ = 0;
};

}  // namespace dcompframe
