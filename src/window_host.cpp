#include "dcompframe/window_host.h"

namespace dcompframe {

WindowHost::WindowHost(WindowConfig config) : config_(config) {}

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
    state_ = state;
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

}  // namespace dcompframe
