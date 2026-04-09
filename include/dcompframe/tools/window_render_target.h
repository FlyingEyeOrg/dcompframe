#pragma once

#include <d3d11.h>
#include <dcomp.h>
#include <dxgi1_2.h>

#include "dcompframe/render_manager.h"
#include "dcompframe/window_host.h"

namespace dcompframe {

class WindowRenderTarget {
public:
    WindowRenderTarget(RenderManager* render_manager, WindowHost* window_host);
    ~WindowRenderTarget();

    bool initialize();
    bool render_frame(bool has_dirty_changes = true);

    [[nodiscard]] bool is_ready() const;
    [[nodiscard]] int presented_frames() const;

private:
    bool initialize_dx11_dcomp_target();
    bool recreate_render_target_view();
    void cleanup_dx11_dcomp_target();

    RenderManager* render_manager_ = nullptr;
    WindowHost* window_host_ = nullptr;
    CompositionBridge bridge_ {nullptr};
    bool ready_ = false;
    int presented_frames_ = 0;
    bool using_dx11_dcomp_ = false;

    ID3D11Device* d3d_device_ = nullptr;
    ID3D11DeviceContext* d3d_context_ = nullptr;
    IDXGISwapChain1* swap_chain_ = nullptr;
    ID3D11RenderTargetView* render_target_view_ = nullptr;
    IDCompositionDevice* dcomp_device_ = nullptr;
    IDCompositionTarget* dcomp_target_ = nullptr;
    IDCompositionVisual* dcomp_visual_ = nullptr;
};

}  // namespace dcompframe
