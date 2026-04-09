#pragma once

#include <cstdint>
#include <windows.h>

#include "dcompframe/geometry.h"

namespace dcompframe {

enum class WindowState {
    Normal,
    Minimized,
    Maximized,
    Fullscreen
};

struct WindowConfig {
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD ex_style = WS_EX_NOREDIRECTIONBITMAP | WS_EX_APPWINDOW;
    float dpi_scale = 1.0F;
};

class WindowHost {
public:
    explicit WindowHost(WindowConfig config = {});

    void on_size_changed(int width, int height);
    void apply_dpi(unsigned int dpi);
    void set_window_state(WindowState state);

    [[nodiscard]] bool consume_redraw_request();
    [[nodiscard]] Size client_size() const;
    [[nodiscard]] WindowState window_state() const;
    [[nodiscard]] const WindowConfig& config() const;

private:
    WindowConfig config_;
    Size client_size_ {0.0F, 0.0F};
    WindowState state_ = WindowState::Normal;
    bool needs_redraw_ = false;
};

}  // namespace dcompframe
