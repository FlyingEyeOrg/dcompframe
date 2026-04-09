#include "dcompframe/tools/window_render_target.h"

#include <d2d1helper.h>
#include <dxgi.h>
#include <iterator>
#include <string>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace dcompframe {

namespace {

template <typename T>
void safe_release(T*& pointer) {
    if (pointer != nullptr) {
        pointer->Release();
        pointer = nullptr;
    }
}

void draw_gdi_controls(HWND hwnd) {
    if (hwnd == nullptr) {
        return;
    }

    HDC dc = GetDC(hwnd);
    if (dc == nullptr) {
        return;
    }

    RECT rect {};
    GetClientRect(hwnd, &rect);
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;

    HBRUSH card_brush = CreateSolidBrush(RGB(34, 52, 74));
    RECT card_rect {width / 10, height / 10, width * 9 / 10, height * 7 / 10};
    FillRect(dc, &card_rect, card_brush);
    DeleteObject(card_brush);

    HPEN border_pen = CreatePen(PS_SOLID, 2, RGB(0, 166, 200));
    HGDIOBJ old_pen = SelectObject(dc, border_pen);
    HGDIOBJ old_brush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, card_rect.left, card_rect.top, card_rect.right, card_rect.bottom);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(border_pen);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(238, 244, 255));
    const std::wstring title = L"DCompFrame Demo - Controls Visible";
    TextOutW(dc, card_rect.left + 20, card_rect.top + 20, title.c_str(), static_cast<int>(title.size()));

    HBRUSH ctl_brush = CreateSolidBrush(RGB(70, 84, 102));
    for (int i = 0; i < 5; ++i) {
        RECT ctl_rect {
            card_rect.left + 28,
            card_rect.top + 60 + i * 48,
            card_rect.left + 360,
            card_rect.top + 92 + i * 48,
        };
        FillRect(dc, &ctl_rect, ctl_brush);
    }
    DeleteObject(ctl_brush);

    HBRUSH action_brush = CreateSolidBrush(RGB(30, 130, 255));
    RECT action_rect {card_rect.right - 180, card_rect.bottom - 90, card_rect.right - 40, card_rect.bottom - 48};
    FillRect(dc, &action_rect, action_brush);
    DeleteObject(action_brush);
    const std::wstring action = L"Start";
    TextOutW(dc, action_rect.left + 40, action_rect.top + 12, action.c_str(), static_cast<int>(action.size()));

    ReleaseDC(hwnd, dc);
}

}  // namespace

WindowRenderTarget::WindowRenderTarget(RenderManager* render_manager, WindowHost* window_host)
    : render_manager_(render_manager),
      window_host_(window_host),
      bridge_(render_manager != nullptr ? render_manager->create_composition_bridge() : CompositionBridge(nullptr)) {}

WindowRenderTarget::~WindowRenderTarget() {
    cleanup_dx11_dcomp_target();
}

bool WindowRenderTarget::initialize() {
    if (render_manager_ == nullptr || window_host_ == nullptr || !render_manager_->is_initialized()) {
        return false;
    }

    if (render_manager_->backend() == RenderBackend::DirectX && initialize_dx11_dcomp_target()) {
        using_dx11_dcomp_ = true;
        ready_ = true;
        return true;
    }

    ready_ = bridge_.bind_target_handle(window_host_->hwnd());
    return ready_;
}

bool WindowRenderTarget::render_frame(bool has_dirty_changes) {
    if (!ready_) {
        return false;
    }

    if (render_manager_->device_recovery().is_device_lost()) {
        if (!render_manager_->device_recovery().try_recover()) {
            return false;
        }

        if (using_dx11_dcomp_ && !recreate_render_target_view()) {
            render_manager_->diagnostics().log(LogLevel::Error, "Render target recovery failed");
            return false;
        }
        if (using_dx11_dcomp_ && !recreate_d2d_target()) {
            render_manager_->diagnostics().log(LogLevel::Warning, "D2D target recovery failed");
        }
    }

    if (using_dx11_dcomp_) {
        if (!has_dirty_changes || d3d_context_ == nullptr || render_target_view_ == nullptr || swap_chain_ == nullptr) {
            return false;
        }

        const float clear_color[4] {
            0.08F,
            0.10F + static_cast<float>((presented_frames_ % 20) * 0.01F),
            0.16F,
            1.0F,
        };
        d3d_context_->OMSetRenderTargets(1, &render_target_view_, nullptr);
        d3d_context_->ClearRenderTargetView(render_target_view_, clear_color);

        bool overlay_drawn = false;
        if (d2d_context_ != nullptr && d2d_target_bitmap_ != nullptr && d2d_brush_ != nullptr) {
            const auto size = window_host_->client_size();
            const float width = size.width > 0.0F ? size.width : 1280.0F;
            const float height = size.height > 0.0F ? size.height : 720.0F;

            d2d_context_->BeginDraw();
            d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());

            d2d_brush_->SetColor(D2D1::ColorF(0.14F, 0.20F, 0.30F, 0.95F));
            const D2D1_ROUNDED_RECT card = D2D1::RoundedRect(
                D2D1::RectF(width * 0.1F, height * 0.1F, width * 0.9F, height * 0.65F),
                20.0F,
                20.0F);
            d2d_context_->FillRoundedRectangle(card, d2d_brush_);

            d2d_brush_->SetColor(D2D1::ColorF(0.02F, 0.65F, 0.75F, 1.0F));
            d2d_context_->DrawRoundedRectangle(card, d2d_brush_, 2.0F);

            d2d_brush_->SetColor(D2D1::ColorF(0.95F, 0.97F, 1.0F, 1.0F));
            if (text_format_ != nullptr) {
                const wchar_t* title = L"DCompFrame Demo - Visible UI Content";
                d2d_context_->DrawText(
                    title,
                    static_cast<UINT32>(wcslen(title)),
                    text_format_,
                    D2D1::RectF(width * 0.13F, height * 0.14F, width * 0.85F, height * 0.22F),
                    d2d_brush_);
            }

            d2d_brush_->SetColor(D2D1::ColorF(0.25F, 0.30F, 0.38F, 0.96F));
            for (int i = 0; i < 5; ++i) {
                const float top = height * 0.25F + static_cast<float>(i) * 52.0F;
                const D2D1_ROUNDED_RECT control = D2D1::RoundedRect(
                    D2D1::RectF(width * 0.14F, top, width * 0.70F, top + 40.0F),
                    10.0F,
                    10.0F);
                d2d_context_->FillRoundedRectangle(control, d2d_brush_);
            }

            d2d_brush_->SetColor(D2D1::ColorF(0.10F, 0.55F, 0.95F, 1.0F));
            const D2D1_ROUNDED_RECT action = D2D1::RoundedRect(
                D2D1::RectF(width * 0.73F, height * 0.50F, width * 0.86F, height * 0.58F),
                12.0F,
                12.0F);
            d2d_context_->FillRoundedRectangle(action, d2d_brush_);

            const HRESULT end_draw_hr = d2d_context_->EndDraw();
            if (FAILED(end_draw_hr)) {
                render_manager_->diagnostics().log(LogLevel::Warning, "D2D overlay draw failed");
            } else {
                overlay_drawn = true;
            }
        }

        const HRESULT present_hr = swap_chain_->Present(0, 0);
        if (present_hr == DXGI_ERROR_DEVICE_REMOVED || present_hr == DXGI_ERROR_DEVICE_RESET) {
            render_manager_->device_recovery().notify_device_lost();
            render_manager_->diagnostics().log(LogLevel::Warning, "DXGI present reported device lost");
            return false;
        }

        if (FAILED(present_hr)) {
            render_manager_->diagnostics().log(LogLevel::Error, "DXGI present failed");
            return false;
        }

        if (dcomp_device_ != nullptr) {
            const HRESULT commit_hr = dcomp_device_->Commit();
            if (FAILED(commit_hr)) {
                render_manager_->diagnostics().log(LogLevel::Error, "DirectComposition commit failed");
                return false;
            }
        }

        if (!overlay_drawn && window_host_ != nullptr) {
            draw_gdi_controls(window_host_->hwnd());
            render_manager_->diagnostics().log(LogLevel::Info, "GDI overlay fallback rendered controls");
        }

        render_manager_->notify_commit();
        render_manager_->enqueue_command(RenderCommand {.type = RenderCommandType::Present, .payload = "dx11-present"});
        render_manager_->diagnostics().record_commit();
        render_manager_->diagnostics().record_frame(std::chrono::milliseconds {16});
        render_manager_->diagnostics().update_resource_peak(render_manager_->resource_manager().total_bytes());
        ++presented_frames_;
        return true;
    }

    if (has_dirty_changes && window_host_ != nullptr && window_host_->hwnd() != nullptr) {
        HDC dc = GetDC(window_host_->hwnd());
        if (dc != nullptr) {
            RECT rect {};
            GetClientRect(window_host_->hwnd(), &rect);
            const COLORREF color = RGB(24, static_cast<int>(80 + (presented_frames_ % 120)), 140);
            HBRUSH brush = CreateSolidBrush(color);
            FillRect(dc, &rect, brush);
            DeleteObject(brush);

            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(255, 255, 255));
            const std::wstring text = L"DCompFrame fallback renderer active";
            TextOutW(dc, 16, 16, text.c_str(), static_cast<int>(text.size()));
            ReleaseDC(window_host_->hwnd(), dc);
        }
    }

    const bool committed = bridge_.commit_changes(has_dirty_changes);
    if (committed) {
        render_manager_->enqueue_command(RenderCommand {.type = RenderCommandType::Commit, .payload = "bridge-commit"});
        ++presented_frames_;
    }

    return committed;
}

bool WindowRenderTarget::is_ready() const {
    return ready_;
}

int WindowRenderTarget::presented_frames() const {
    return presented_frames_;
}

bool WindowRenderTarget::initialize_dx11_dcomp_target() {
    if (window_host_ == nullptr || window_host_->hwnd() == nullptr) {
        return false;
    }

    UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_FEATURE_LEVEL feature_levels[] {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    D3D_FEATURE_LEVEL used_feature_level = D3D_FEATURE_LEVEL_11_0;

    const auto create_device = [&](D3D_DRIVER_TYPE driver_type, UINT flags) {
        return D3D11CreateDevice(
            nullptr,
            driver_type,
            nullptr,
            flags,
            feature_levels,
            static_cast<UINT>(std::size(feature_levels)),
            D3D11_SDK_VERSION,
            &d3d_device_,
            &used_feature_level,
            &d3d_context_);
    };

    HRESULT hr = create_device(D3D_DRIVER_TYPE_HARDWARE, create_flags);
    if (FAILED(hr) && (create_flags & D3D11_CREATE_DEVICE_DEBUG) != 0U) {
        hr = create_device(D3D_DRIVER_TYPE_HARDWARE, D3D11_CREATE_DEVICE_BGRA_SUPPORT);
    }
    if (FAILED(hr)) {
        hr = create_device(D3D_DRIVER_TYPE_WARP, D3D11_CREATE_DEVICE_BGRA_SUPPORT);
    }

    if (FAILED(hr) || d3d_device_ == nullptr || d3d_context_ == nullptr) {
        cleanup_dx11_dcomp_target();
        return false;
    }

    IDXGIDevice* dxgi_device = nullptr;
    IDXGIAdapter* dxgi_adapter = nullptr;
    IDXGIFactory2* dxgi_factory = nullptr;
    hr = d3d_device_->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device));
    if (FAILED(hr) || dxgi_device == nullptr) {
        safe_release(dxgi_device);
        cleanup_dx11_dcomp_target();
        return false;
    }

    hr = dxgi_device->GetAdapter(&dxgi_adapter);
    if (FAILED(hr) || dxgi_adapter == nullptr) {
        safe_release(dxgi_adapter);
        safe_release(dxgi_device);
        cleanup_dx11_dcomp_target();
        return false;
    }

    hr = dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgi_factory));
    if (FAILED(hr) || dxgi_factory == nullptr) {
        safe_release(dxgi_factory);
        safe_release(dxgi_adapter);
        safe_release(dxgi_device);
        cleanup_dx11_dcomp_target();
        return false;
    }

    const Size size = window_host_->client_size();
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc {};
    swap_chain_desc.Width = static_cast<UINT>(size.width > 0 ? size.width : 1);
    swap_chain_desc.Height = static_cast<UINT>(size.height > 0 ? size.height : 1);
    swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_chain_desc.Stereo = FALSE;
    swap_chain_desc.SampleDesc = DXGI_SAMPLE_DESC {1, 0};
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    hr = dxgi_factory->CreateSwapChainForComposition(d3d_device_, &swap_chain_desc, nullptr, &swap_chain_);
    if (FAILED(hr) || swap_chain_ == nullptr) {
        safe_release(dxgi_factory);
        safe_release(dxgi_adapter);
        safe_release(dxgi_device);
        cleanup_dx11_dcomp_target();
        return false;
    }

    hr = DCompositionCreateDevice(dxgi_device, __uuidof(IDCompositionDevice), reinterpret_cast<void**>(&dcomp_device_));
    if (FAILED(hr) || dcomp_device_ == nullptr) {
        safe_release(dxgi_factory);
        safe_release(dxgi_adapter);
        safe_release(dxgi_device);
        cleanup_dx11_dcomp_target();
        return false;
    }

    hr = dcomp_device_->CreateTargetForHwnd(window_host_->hwnd(), TRUE, &dcomp_target_);
    if (FAILED(hr) || dcomp_target_ == nullptr) {
        safe_release(dxgi_factory);
        safe_release(dxgi_adapter);
        safe_release(dxgi_device);
        cleanup_dx11_dcomp_target();
        return false;
    }

    hr = dcomp_device_->CreateVisual(&dcomp_visual_);
    if (FAILED(hr) || dcomp_visual_ == nullptr) {
        safe_release(dxgi_factory);
        safe_release(dxgi_adapter);
        safe_release(dxgi_device);
        cleanup_dx11_dcomp_target();
        return false;
    }

    hr = dcomp_visual_->SetContent(swap_chain_);
    if (FAILED(hr)) {
        safe_release(dxgi_factory);
        safe_release(dxgi_adapter);
        safe_release(dxgi_device);
        cleanup_dx11_dcomp_target();
        return false;
    }

    hr = dcomp_target_->SetRoot(dcomp_visual_);
    if (FAILED(hr)) {
        safe_release(dxgi_factory);
        safe_release(dxgi_adapter);
        safe_release(dxgi_device);
        cleanup_dx11_dcomp_target();
        return false;
    }

    hr = dcomp_device_->Commit();
    safe_release(dxgi_factory);
    safe_release(dxgi_adapter);
    safe_release(dxgi_device);
    if (FAILED(hr)) {
        cleanup_dx11_dcomp_target();
        return false;
    }

    if (!recreate_render_target_view()) {
        cleanup_dx11_dcomp_target();
        return false;
    }

    if (!initialize_d2d_overlay()) {
        render_manager_->diagnostics().log(LogLevel::Warning, "D2D overlay initialization failed");
    }

    return true;
}

bool WindowRenderTarget::recreate_render_target_view() {
    safe_release(render_target_view_);
    if (swap_chain_ == nullptr || d3d_device_ == nullptr) {
        return false;
    }

    ID3D11Texture2D* back_buffer = nullptr;
    const HRESULT hr = swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
    if (FAILED(hr) || back_buffer == nullptr) {
        safe_release(back_buffer);
        return false;
    }

    const HRESULT rtv_hr = d3d_device_->CreateRenderTargetView(back_buffer, nullptr, &render_target_view_);
    safe_release(back_buffer);
    return SUCCEEDED(rtv_hr) && render_target_view_ != nullptr;
}

bool WindowRenderTarget::initialize_d2d_overlay() {
    if (d3d_device_ == nullptr) {
        return false;
    }

    if (d2d_factory_ == nullptr) {
        const D2D1_FACTORY_OPTIONS options {};
        const HRESULT factory_hr = D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory1),
            &options,
            reinterpret_cast<void**>(&d2d_factory_));
        if (FAILED(factory_hr) || d2d_factory_ == nullptr) {
            return false;
        }
    }

    if (dwrite_factory_ == nullptr) {
        const HRESULT write_hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&dwrite_factory_));
        if (FAILED(write_hr) || dwrite_factory_ == nullptr) {
            return false;
        }

        const HRESULT format_hr = dwrite_factory_->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            20.0F,
            L"zh-CN",
            &text_format_);
        if (FAILED(format_hr) || text_format_ == nullptr) {
            return false;
        }
    }

    if (d2d_device_ == nullptr || d2d_context_ == nullptr) {
        IDXGIDevice* dxgi_device = nullptr;
        const HRESULT qhr = d3d_device_->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device));
        if (FAILED(qhr) || dxgi_device == nullptr) {
            safe_release(dxgi_device);
            return false;
        }

        const HRESULT device_hr = d2d_factory_->CreateDevice(dxgi_device, &d2d_device_);
        safe_release(dxgi_device);
        if (FAILED(device_hr) || d2d_device_ == nullptr) {
            return false;
        }

        const HRESULT context_hr = d2d_device_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2d_context_);
        if (FAILED(context_hr) || d2d_context_ == nullptr) {
            return false;
        }
    }

    return recreate_d2d_target();
}

bool WindowRenderTarget::recreate_d2d_target() {
    safe_release(d2d_target_bitmap_);
    safe_release(d2d_brush_);
    if (swap_chain_ == nullptr || d2d_context_ == nullptr) {
        return false;
    }

    IDXGISurface* surface = nullptr;
    const HRESULT surface_hr = swap_chain_->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(&surface));
    if (FAILED(surface_hr) || surface == nullptr) {
        safe_release(surface);
        return false;
    }

    const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0F,
        96.0F);
    const HRESULT bitmap_hr = d2d_context_->CreateBitmapFromDxgiSurface(surface, &props, &d2d_target_bitmap_);
    safe_release(surface);
    if (FAILED(bitmap_hr) || d2d_target_bitmap_ == nullptr) {
        return false;
    }

    d2d_context_->SetTarget(d2d_target_bitmap_);
    const HRESULT brush_hr = d2d_context_->CreateSolidColorBrush(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), &d2d_brush_);
    return SUCCEEDED(brush_hr) && d2d_brush_ != nullptr;
}

void WindowRenderTarget::cleanup_dx11_dcomp_target() {
    safe_release(d2d_brush_);
    safe_release(d2d_target_bitmap_);
    safe_release(text_format_);
    safe_release(dwrite_factory_);
    safe_release(d2d_context_);
    safe_release(d2d_device_);
    safe_release(d2d_factory_);
    safe_release(render_target_view_);
    safe_release(swap_chain_);
    safe_release(dcomp_visual_);
    safe_release(dcomp_target_);
    safe_release(dcomp_device_);
    safe_release(d3d_context_);
    safe_release(d3d_device_);
    using_dx11_dcomp_ = false;
}

}  // namespace dcompframe
