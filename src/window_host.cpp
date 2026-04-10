#include "dcompframe/window_host.h"

#include <atomic>
#include <string>

namespace dcompframe {

namespace {

std::atomic<int> g_window_count = 0;

LRESULT CALLBACK DCompFrameWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<WindowHost*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        const auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = reinterpret_cast<WindowHost*>(create_struct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }

    if (self != nullptr) {
        LRESULT handler_result = 0;
        switch (msg) {
        case WM_SIZE:
            self->on_size_changed(LOWORD(lparam), HIWORD(lparam));
            if (self->dispatch_message(msg, wparam, lparam, handler_result)) {
                return handler_result;
            }
            return 0;
        case WM_DPICHANGED:
            self->apply_dpi(LOWORD(wparam));
            if (self->dispatch_message(msg, wparam, lparam, handler_result)) {
                return handler_result;
            }
            return 0;
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_CAPTURECHANGED:
        case WM_CANCELMODE:
        case WM_CHAR:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_GETDLGCODE:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_TIMER:
            self->request_render();
            if (self->dispatch_message(msg, wparam, lparam, handler_result)) {
                return handler_result;
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            return 0;
        case WM_NCDESTROY: {
            self->on_destroyed();
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            const int remained = --g_window_count;
            if (remained <= 0) {
                PostQuitMessage(0);
            }
            return 0;
        }
        default:
            break;
        }
    }

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
        this);

    if (hwnd_ == nullptr) {
        return false;
    }

    ++g_window_count;
    created_ = true;
    RECT client_rect {};
    if (GetClientRect(hwnd_, &client_rect)) {
        on_size_changed(client_rect.right - client_rect.left, client_rect.bottom - client_rect.top);
    } else {
        on_size_changed(width, height);
    }
    return true;
}

void WindowHost::destroy() {
    if (!created_) {
        return;
    }

    if (hwnd_ != nullptr) {
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

void WindowHost::on_destroyed() {
    hwnd_ = nullptr;
    created_ = false;
    visible_ = false;
    needs_redraw_ = false;
}

void WindowHost::set_window_state(WindowState state) {
    if (state_ == state) {
        return;
    }

    if (hwnd_ != nullptr) {
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
    if (hwnd_ != nullptr) {
        ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
    }
}

void WindowHost::set_message_handler(MessageHandler handler) {
    message_handler_ = std::move(handler);
}

int WindowHost::run_message_loop(const std::function<bool()>& render_callback, int max_iterations) {
    if (!created_) {
        return 0;
    }

    int rendered_frames = 0;
    int iterations = 0;
    MSG msg {};

    if (max_iterations < 0) {
        while (true) {
            const BOOL result = GetMessageW(&msg, nullptr, 0, 0);
            if (result <= 0) {
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);

            if (needs_redraw_ && render_callback) {
                if (render_callback()) {
                    ++rendered_frames;
                }
                needs_redraw_ = false;
            }
        }

        return rendered_frames;
    }

    while (iterations < max_iterations) {
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

bool WindowHost::dispatch_message(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& result) {
    if (!message_handler_) {
        return false;
    }

    return message_handler_(msg, wparam, lparam, result);
}

}  // namespace dcompframe
