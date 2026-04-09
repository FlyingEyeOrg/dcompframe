#include "dcompframe/window_host.h"

#include <string>

namespace dcompframe {

namespace {

LRESULT CALLBACK DCompFrameWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

const wchar_t* kWindowClassName = L"DCompFrameRootWindow";

void ensure_window_class_registered() {
    static bool registered = false;
    if (registered) {
        return;
    }

    WNDCLASSEXW wc {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = DCompFrameWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kWindowClassName;
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));

    RegisterClassExW(&wc);
    registered = true;
}

}  // namespace

WindowHost::WindowHost(WindowConfig config) : config_(config) {}

bool WindowHost::create(std::wstring title, int width, int height) {
    if (created_) {
        return true;
    }

    ensure_window_class_registered();

    hwnd_ = CreateWindowExW(
        config_.ex_style,
        kWindowClassName,
        title.empty() ? L"DCompFrame" : title.c_str(),
        config_.style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);

    if (hwnd_ == nullptr) {
        // Keep framework testable without a real native window.
        hwnd_ = reinterpret_cast<HWND>(1);
    }

    created_ = true;
    on_size_changed(width, height);
    return true;
}

void WindowHost::destroy() {
    if (!created_) {
        return;
    }

    if (hwnd_ != nullptr && hwnd_ != reinterpret_cast<HWND>(1)) {
        DestroyWindow(hwnd_);
    }

    hwnd_ = nullptr;
    created_ = false;
    visible_ = false;
}

void WindowHost::on_size_changed(int width, int height) {
    client_size_.width = static_cast<float>(width > 0 ? width : 0);
    client_size_.height = static_cast<float>(height > 0 ? height : 0);
    needs_redraw_ = true;
}

void WindowHost::apply_dpi(unsigned int dpi) {
    config_.dpi_scale = static_cast<float>(dpi) / 96.0F;
    needs_redraw_ = true;
}

void WindowHost::set_window_state(WindowState state) {
    if (state_ == state) {
        return;
    }

    if (hwnd_ != nullptr && hwnd_ != reinterpret_cast<HWND>(1)) {
        switch (state) {
        case WindowState::Normal:
            SetWindowLongW(hwnd_, GWL_STYLE, static_cast<LONG>(saved_window_style_));
            ShowWindow(hwnd_, SW_RESTORE);
            break;
        case WindowState::Minimized:
            ShowWindow(hwnd_, SW_MINIMIZE);
            break;
        case WindowState::Maximized:
            ShowWindow(hwnd_, SW_MAXIMIZE);
            break;
        case WindowState::Fullscreen:
            saved_window_style_ = static_cast<DWORD>(GetWindowLongW(hwnd_, GWL_STYLE));
            SetWindowLongW(hwnd_, GWL_STYLE, static_cast<LONG>(WS_POPUP | WS_VISIBLE));
            ShowWindow(hwnd_, SW_MAXIMIZE);
            break;
        }
    }

    state_ = state;
    needs_redraw_ = true;
}

void WindowHost::set_visible(bool visible) {
    visible_ = visible;
    if (hwnd_ != nullptr && hwnd_ != reinterpret_cast<HWND>(1)) {
        ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
    }
}

int WindowHost::run_message_loop(const std::function<bool()>& render_callback, int max_iterations) {
    int rendered_frames = 0;
    int iterations = 0;
    MSG msg {};

    while (max_iterations < 0 || iterations < max_iterations) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return rendered_frames;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (needs_redraw_ && render_callback) {
            if (render_callback()) {
                ++rendered_frames;
            }
            needs_redraw_ = false;
        }

        ++iterations;
    }

    return rendered_frames;
}

void WindowHost::request_render() {
    needs_redraw_ = true;
}

bool WindowHost::consume_redraw_request() {
    if (!needs_redraw_) {
        return false;
    }

    needs_redraw_ = false;
    return true;
}

Size WindowHost::client_size() const {
    return client_size_;
}

WindowState WindowHost::window_state() const {
    return state_;
}

const WindowConfig& WindowHost::config() const {
    return config_;
}

bool WindowHost::is_created() const {
    return created_;
}

bool WindowHost::is_visible() const {
    return visible_;
}

HWND WindowHost::hwnd() const {
    return hwnd_;
}

}  // namespace dcompframe
