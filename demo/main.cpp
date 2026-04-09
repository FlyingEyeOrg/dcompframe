#include <fmt/core.h>

#include <memory>

#include "dcompframe/layout/grid_panel.h"
#include "dcompframe/layout/stack_panel.h"
#include "dcompframe/render_manager.h"
#include "dcompframe/ui_element.h"
#include "dcompframe/window_host.h"

int main() {
    dcompframe::RenderManager render_manager;
    if (!render_manager.initialize(true)) {
        fmt::print("RenderManager initialization failed.\n");
        return 1;
    }

    dcompframe::WindowHost host;
    host.on_size_changed(1280, 720);
    host.apply_dpi(144);

    auto root = std::make_shared<dcompframe::GridPanel>(2, 2);
    auto card = std::make_shared<dcompframe::UIElement>("card");
    card->set_desired_size(dcompframe::Size {.width = 480.0F, .height = 240.0F});

    root->add_child(card);
    root->set_grid_position(card, dcompframe::GridPanel::Cell {.row = 0, .col = 0, .row_span = 1, .col_span = 2});
    root->arrange(dcompframe::Size {.width = 1280.0F, .height = 720.0F});

    auto bridge = render_manager.create_composition_bridge();
    bridge.bind_target_handle(reinterpret_cast<HWND>(1));
    const bool committed = bridge.commit_changes(true);

    fmt::print(
        "DCompFrame demo initialized. dpi_scale={:.2f}, committed={}, total_commits={}\n",
        host.config().dpi_scale,
        committed,
        render_manager.total_commit_count());

    return 0;
}
