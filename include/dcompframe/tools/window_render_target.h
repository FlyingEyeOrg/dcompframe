#pragma once

#include <d3d11.h>
#include <d2d1_1.h>
#include <dcomp.h>
#include <dwrite.h>
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
    bool initialize_d2d_overlay();
    bool recreate_d2d_target();
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

    ID2D1Factory1* d2d_factory_ = nullptr;
    ID2D1Device* d2d_device_ = nullptr;
    ID2D1DeviceContext* d2d_context_ = nullptr;
    ID2D1Bitmap1* d2d_target_bitmap_ = nullptr;
    ID2D1SolidColorBrush* d2d_brush_ = nullptr;
    IDWriteFactory* dwrite_factory_ = nullptr;
    IDWriteTextFormat* text_format_ = nullptr;
};

}  // namespace dcompframe
