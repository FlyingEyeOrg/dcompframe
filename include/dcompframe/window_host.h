#pragma once

#include <cstdint>
#include <functional>
#include <string>
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

    bool create(std::wstring title, int width, int height);
    void destroy();

    void on_size_changed(int width, int height);
    void apply_dpi(unsigned int dpi);
    void set_window_state(WindowState state);
    void set_visible(bool visible);

    int run_message_loop(const std::function<bool()>& render_callback, int max_iterations = -1);
    void request_render();

    [[nodiscard]] bool consume_redraw_request();
    [[nodiscard]] Size client_size() const;
    [[nodiscard]] WindowState window_state() const;
    [[nodiscard]] const WindowConfig& config() const;
    [[nodiscard]] bool is_created() const;
    [[nodiscard]] bool is_visible() const;
    [[nodiscard]] HWND hwnd() const;

private:
    WindowConfig config_;
    Size client_size_ {0.0F, 0.0F};
    WindowState state_ = WindowState::Normal;
    bool needs_redraw_ = false;
    bool created_ = false;
    bool visible_ = false;
    HWND hwnd_ = nullptr;
    DWORD saved_window_style_ = WS_OVERLAPPEDWINDOW;
};

}  // namespace dcompframe
