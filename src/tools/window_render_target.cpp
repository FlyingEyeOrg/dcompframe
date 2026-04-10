#include "dcompframe/tools/window_render_target.h"

#include <d2d1helper.h>
#include <dxgi.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <iterator>
#include <string>
#include <vector>

#include <windowsx.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace dcompframe {

namespace {

template <typename T>
T min_value(const T& left, const T& right) {
    return left < right ? left : right;
}

template <typename T>
T max_value(const T& left, const T& right) {
    return left > right ? left : right;
}

template <typename T>
T clamp_value(const T& value, const T& low, const T& high) {
    return min_value(max_value(value, low), high);
}

constexpr std::size_t kPrimaryButtonFocusIndex = 0;
constexpr std::size_t kTextBoxFocusIndex = 1;
constexpr std::size_t kCheckBoxFocusIndex = 2;
constexpr std::size_t kComboBoxFocusIndex = 3;
constexpr std::size_t kSliderFocusIndex = 4;
constexpr std::size_t kInteractiveFocusCount = 5;
constexpr UINT_PTR kCaretTimerId = 1;

enum class HitTarget {
    None,
    PrimaryButton,
    TextBox,
    CheckBox,
    ComboBoxHeader,
    ComboBoxItem,
    Slider
};

struct HitResult {
    HitTarget target = HitTarget::None;
    int combo_index = -1;
};

struct OverlayLayout {
    D2D1_RECT_F card {};
    D2D1_RECT_F title {};
    D2D1_RECT_F subtitle {};
    D2D1_RECT_F button {};
    D2D1_RECT_F text_box {};
    D2D1_RECT_F text_box_text {};
    D2D1_RECT_F check_box {};
    D2D1_RECT_F combo_box {};
    D2D1_RECT_F slider {};
    D2D1_RECT_F scroll_viewer {};
    D2D1_RECT_F footer {};
    D2D1_RECT_F combo_dropdown {};
    D2D1_RECT_F slider_track {};
    std::vector<D2D1_RECT_F> combo_items;
};

template <typename T>
void safe_release(T*& pointer) {
    if (pointer != nullptr) {
        pointer->Release();
        pointer = nullptr;
    }
}

std::string hr_to_string(HRESULT hr) {
    char buffer[32] {};
    std::snprintf(buffer, sizeof(buffer), "0x%08X", static_cast<unsigned int>(hr));
    return std::string(buffer);
}

std::wstring utf8_to_wstring(const std::string& value) {
    if (value.empty()) {
        return {};
    }

    const int count = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (count <= 1) {
        return {};
    }

    std::vector<wchar_t> buffer(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, buffer.data(), count);
    return std::wstring(buffer.data());
}

std::string wstring_to_utf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }

    const int count = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (count <= 1) {
        return {};
    }

    std::vector<char> buffer(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, buffer.data(), count, nullptr, nullptr);
    return std::string(buffer.data());
}

D2D1_RECT_F inset_rect(const D2D1_RECT_F& rect, float inset_x, float inset_y) {
    return D2D1::RectF(rect.left + inset_x, rect.top + inset_y, rect.right - inset_x, rect.bottom - inset_y);
}

bool point_in_rect(const D2D1_RECT_F& rect, float x, float y) {
    return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
}

float rect_width(const D2D1_RECT_F& rect) {
    return rect.right - rect.left;
}

float rect_height(const D2D1_RECT_F& rect) {
    return rect.bottom - rect.top;
}

OverlayLayout compute_layout(float width, float height, const WindowRenderTarget::InteractiveControls& controls) {
    OverlayLayout layout;
    layout.card = D2D1::RectF(width * 0.08F, height * 0.08F, width * 0.92F, height * 0.90F);
    layout.title = D2D1::RectF(layout.card.left + 28.0F, layout.card.top + 20.0F, layout.card.right - 240.0F, layout.card.top + 72.0F);
    layout.subtitle = D2D1::RectF(layout.card.left + 28.0F, layout.card.top + 72.0F, layout.card.right - 28.0F, layout.card.top + 116.0F);
    layout.button = D2D1::RectF(layout.card.right - 188.0F, layout.card.top + 28.0F, layout.card.right - 28.0F, layout.card.top + 84.0F);

    const float left = layout.card.left + 32.0F;
    const float right = layout.card.right - 32.0F;
    const float row_height = 58.0F;
    const float gap = 18.0F;
    float top = layout.card.top + 138.0F;

    layout.text_box = D2D1::RectF(left, top, right, top + row_height);
    layout.text_box_text = inset_rect(layout.text_box, 16.0F, 12.0F);
    top += row_height + gap;

    layout.check_box = D2D1::RectF(left, top, right, top + row_height);
    top += row_height + gap;

    layout.combo_box = D2D1::RectF(left, top, right, top + row_height);
    if (controls.combo_box && controls.combo_box->is_dropdown_open()) {
        const float item_height = 42.0F;
        const std::size_t item_count = controls.combo_box->items().size();
        layout.combo_dropdown = D2D1::RectF(left, top + row_height + 6.0F, right, top + row_height + 6.0F + item_height * static_cast<float>(item_count) + 8.0F);
        layout.combo_items.reserve(item_count);
        for (std::size_t index = 0; index < item_count; ++index) {
            const float item_top = layout.combo_dropdown.top + 4.0F + item_height * static_cast<float>(index);
            layout.combo_items.push_back(D2D1::RectF(left + 6.0F, item_top, right - 6.0F, item_top + item_height));
        }
        top = layout.combo_dropdown.bottom + gap;
    } else {
        top += row_height + gap;
    }

    layout.slider = D2D1::RectF(left, top, right, top + row_height);
    layout.slider_track = D2D1::RectF(left + 180.0F, top + 26.0F, right - 80.0F, top + 32.0F);
    top += row_height + gap;

    layout.scroll_viewer = D2D1::RectF(left, top, right, top + row_height);
    top += row_height + gap;

    layout.footer = D2D1::RectF(left, top, right, min_value(layout.card.bottom - 20.0F, top + 86.0F));
    return layout;
}

HitResult hit_test(const OverlayLayout& layout, float x, float y) {
    if (point_in_rect(layout.button, x, y)) {
        return {.target = HitTarget::PrimaryButton};
    }
    if (point_in_rect(layout.text_box, x, y)) {
        return {.target = HitTarget::TextBox};
    }
    if (point_in_rect(layout.check_box, x, y)) {
        return {.target = HitTarget::CheckBox};
    }
    if (point_in_rect(layout.combo_box, x, y)) {
        return {.target = HitTarget::ComboBoxHeader};
    }
    for (std::size_t index = 0; index < layout.combo_items.size(); ++index) {
        if (point_in_rect(layout.combo_items[index], x, y)) {
            return {.target = HitTarget::ComboBoxItem, .combo_index = static_cast<int>(index)};
        }
    }
    if (point_in_rect(layout.slider, x, y)) {
        return {.target = HitTarget::Slider};
    }
    return {};
}

bool create_text_layout(
    IDWriteFactory* factory,
    IDWriteTextFormat* format,
    const std::wstring& text,
    float width,
    float height,
    IDWriteTextLayout** layout) {
    if (factory == nullptr || format == nullptr || layout == nullptr) {
        return false;
    }

    *layout = nullptr;
    const HRESULT hr = factory->CreateTextLayout(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        width,
        height,
        layout);
    return SUCCEEDED(hr) && *layout != nullptr;
}

std::size_t text_box_hit_index(
    IDWriteFactory* factory,
    IDWriteTextFormat* format,
    const std::wstring& text,
    const D2D1_RECT_F& text_rect,
    float x,
    float y) {
    if (text.empty()) {
        return 0;
    }

    IDWriteTextLayout* layout = nullptr;
    if (!create_text_layout(factory, format, text, rect_width(text_rect), rect_height(text_rect), &layout)) {
        return text.size();
    }

    BOOL is_trailing_hit = FALSE;
    BOOL is_inside = FALSE;
    DWRITE_HIT_TEST_METRICS metrics {};
    const HRESULT hr = layout->HitTestPoint(x - text_rect.left, y - text_rect.top, &is_trailing_hit, &is_inside, &metrics);
    safe_release(layout);
    if (FAILED(hr)) {
        return text.size();
    }

    const std::size_t offset = static_cast<std::size_t>(metrics.textPosition);
    return min_value(text.size(), offset + (is_trailing_hit != FALSE ? 1U : 0U));
}

std::wstring selected_text(const TextBox& text_box) {
    const auto [start, end] = text_box.selection();
    const std::wstring wide = utf8_to_wstring(text_box.text());
    if (start >= end || start >= wide.size()) {
        return {};
    }

    return wide.substr(start, min_value(end, wide.size()) - start);
}

bool write_clipboard_text(HWND hwnd, const std::wstring& text) {
    if (text.empty()) {
        return false;
    }

    if (!OpenClipboard(hwnd)) {
        return false;
    }

    EmptyClipboard();
    const std::size_t bytes = (text.size() + 1U) * sizeof(wchar_t);
    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (handle == nullptr) {
        CloseClipboard();
        return false;
    }

    void* buffer = GlobalLock(handle);
    if (buffer == nullptr) {
        GlobalFree(handle);
        CloseClipboard();
        return false;
    }

    std::memcpy(buffer, text.c_str(), bytes);
    GlobalUnlock(handle);
    SetClipboardData(CF_UNICODETEXT, handle);
    CloseClipboard();
    return true;
}

std::wstring read_clipboard_text(HWND hwnd) {
    if (!OpenClipboard(hwnd)) {
        return {};
    }

    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (data == nullptr) {
        CloseClipboard();
        return {};
    }

    const auto* text = static_cast<const wchar_t*>(GlobalLock(data));
    if (text == nullptr) {
        CloseClipboard();
        return {};
    }

    const std::wstring result(text);
    GlobalUnlock(data);
    CloseClipboard();
    return result;
}

void draw_centered_text(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteTextFormat* format,
    const std::wstring& text,
    const D2D1_RECT_F& rect) {
    if (context == nullptr || brush == nullptr || format == nullptr || text.empty()) {
        return;
    }

    const auto prev_align = format->GetTextAlignment();
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    context->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        rect,
        brush);
    format->SetTextAlignment(prev_align);
}

void draw_checkbox_glyph(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect, bool checked) {
    const float cx = rect.left + 28.0F; // Center of the checkbox
    const float cy = (rect.top + rect.bottom) / 2.0F;
    const D2D1_RECT_F box = D2D1::RectF(cx - 10.0F, cy - 10.0F, cx + 10.0F, cy + 10.0F);

    if (checked) {
        brush->SetColor(D2D1::ColorF(0.0F, 0.47F, 0.83F, 1.0F)); // WinUI3 checked background (accent color)
        context->FillRoundedRectangle(D2D1::RoundedRect(box, 4.0F, 4.0F), brush);
        
        brush->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F)); // White tick
        context->DrawLine(D2D1::Point2F(box.left + 5.0F, box.top + 10.0F), D2D1::Point2F(box.left + 9.0F, box.bottom - 6.0F), brush, 2.0F);
        context->DrawLine(D2D1::Point2F(box.left + 9.0F, box.bottom - 6.0F), D2D1::Point2F(box.right - 4.0F, box.top + 5.0F), brush, 2.0F);
    } else {
        brush->SetColor(D2D1::ColorF(0.13F, 0.18F, 0.28F, 1.0F));
        context->FillRoundedRectangle(D2D1::RoundedRect(box, 4.0F, 4.0F), brush);
        brush->SetColor(D2D1::ColorF(0.70F, 0.82F, 1.0F, 1.0F));
        context->DrawRoundedRectangle(D2D1::RoundedRect(box, 4.0F, 4.0F), brush, 1.0F);
    }
}

void draw_combobox_glyph(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect) {
    const float center_x = rect.right - 28.0F;
    const float center_y = (rect.top + rect.bottom) * 0.5F;
    brush->SetColor(D2D1::ColorF(0.72F, 0.82F, 1.0F, 1.0F));
    context->DrawLine(D2D1::Point2F(center_x - 6.0F, center_y - 3.0F), D2D1::Point2F(center_x, center_y + 3.0F), brush, 2.0F);
    context->DrawLine(D2D1::Point2F(center_x, center_y + 3.0F), D2D1::Point2F(center_x + 6.0F, center_y - 3.0F), brush, 2.0F);
}

void draw_slider_glyph(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect, float value) {
    const float track_left = rect.left + 18.0F;
    const float track_right = rect.right - 18.0F;
    const float center_y = (rect.top + rect.bottom) * 0.5F;
    const float thumb_x = track_left + (track_right - track_left) * clamp_value(value, 0.0F, 1.0F);

    brush->SetColor(D2D1::ColorF(0.18F, 0.24F, 0.34F, 1.0F));
    context->DrawLine(D2D1::Point2F(track_left, center_y), D2D1::Point2F(track_right, center_y), brush, 6.0F);
    brush->SetColor(D2D1::ColorF(0.13F, 0.80F, 0.96F, 1.0F));
    context->DrawLine(D2D1::Point2F(track_left, center_y), D2D1::Point2F(thumb_x, center_y), brush, 6.0F);
    brush->SetColor(D2D1::ColorF(0.94F, 0.97F, 1.0F, 1.0F));
    context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(thumb_x, center_y), 8.0F, 8.0F), brush);
}

void draw_scrollviewer_glyph(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect) {
    const D2D1_RECT_F viewport = D2D1::RectF(rect.left + 16.0F, rect.top + 10.0F, rect.right - 16.0F, rect.bottom - 10.0F);
    brush->SetColor(D2D1::ColorF(0.12F, 0.17F, 0.27F, 1.0F));
    context->FillRoundedRectangle(D2D1::RoundedRect(viewport, 6.0F, 6.0F), brush);
    brush->SetColor(D2D1::ColorF(0.68F, 0.78F, 0.98F, 1.0F));
    context->DrawRoundedRectangle(D2D1::RoundedRect(viewport, 6.0F, 6.0F), brush, 1.0F);

    const D2D1_RECT_F thumb = D2D1::RectF(viewport.right - 8.0F, viewport.top + 6.0F, viewport.right - 4.0F, viewport.top + 22.0F);
    brush->SetColor(D2D1::ColorF(0.12F, 0.80F, 0.96F, 1.0F));
    context->FillRoundedRectangle(D2D1::RoundedRect(thumb, 2.0F, 2.0F), brush);
}

void fill_rounded_rect(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float radius = 12.0F) {
    brush->SetColor(color);
    context->FillRoundedRectangle(D2D1::RoundedRect(rect, radius, radius), brush);
}

void stroke_rounded_rect(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float thickness = 1.5F, float radius = 12.0F) {
    brush->SetColor(color);
    context->DrawRoundedRectangle(D2D1::RoundedRect(rect, radius, radius), brush, thickness);
}

void draw_label_value(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteTextFormat* format,
    const std::wstring& label,
    const std::wstring& value,
    const D2D1_RECT_F& rect,
    float label_offset = 18.0F) {
    if (context == nullptr || brush == nullptr || format == nullptr) {
        return;
    }

    brush->SetColor(D2D1::ColorF(0.72F, 0.82F, 1.0F, 1.0F));
    context->DrawText(label.c_str(), static_cast<UINT32>(label.size()), format, D2D1::RectF(rect.left + label_offset, rect.top, rect.left + 170.0F + (label_offset - 18.0F), rect.bottom), brush);
    
    const auto prev_align = format->GetTextAlignment();
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    brush->SetColor(D2D1::ColorF(0.95F, 0.97F, 1.0F, 1.0F));
    context->DrawText(value.c_str(), static_cast<UINT32>(value.size()), format, D2D1::RectF(rect.right - 100.0F, rect.top, rect.right - 18.0F, rect.bottom), brush);
    format->SetTextAlignment(prev_align);
}

void draw_text_box_editor(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteFactory* dwrite_factory,
    IDWriteTextFormat* input_format,
    const TextBox& text_box,
    const D2D1_RECT_F& rect,
    bool hovered,
    bool focused,
    bool caret_visible) {
    fill_rounded_rect(context, brush, rect, hovered ? D2D1::ColorF(0.18F, 0.26F, 0.40F, 1.0F) : D2D1::ColorF(0.14F, 0.20F, 0.32F, 1.0F));
    stroke_rounded_rect(context, brush, rect, focused ? D2D1::ColorF(0.10F, 0.74F, 0.96F, 1.0F) : D2D1::ColorF(0.42F, 0.56F, 0.82F, 0.8F));

    const D2D1_RECT_F text_rect = inset_rect(rect, 16.0F, 12.0F);
    const std::wstring wide_text = utf8_to_wstring(text_box.text());
    const bool use_placeholder = wide_text.empty();
    const std::wstring display_text = use_placeholder ? utf8_to_wstring(text_box.placeholder()) : wide_text;

    IDWriteTextLayout* layout = nullptr;
    if (!create_text_layout(dwrite_factory, input_format, display_text, rect_width(text_rect), rect_height(text_rect), &layout)) {
        return;
    }

    if (!use_placeholder && text_box.has_selection()) {
        const auto [selection_start, selection_end] = text_box.selection();
        UINT32 actual_count = 0;
        layout->HitTestTextRange(
            static_cast<UINT32>(selection_start),
            static_cast<UINT32>(selection_end - selection_start),
            text_rect.left,
            text_rect.top,
            nullptr,
            0,
            &actual_count);
        if (actual_count > 0) {
            std::vector<DWRITE_HIT_TEST_METRICS> metrics(actual_count);
            if (SUCCEEDED(layout->HitTestTextRange(
                    static_cast<UINT32>(selection_start),
                    static_cast<UINT32>(selection_end - selection_start),
                    text_rect.left,
                    text_rect.top,
                    metrics.data(),
                    actual_count,
                    &actual_count))) {
                brush->SetColor(D2D1::ColorF(0.12F, 0.62F, 0.95F, 0.38F));
                for (UINT32 index = 0; index < actual_count; ++index) {
                    const auto& metric = metrics[index];
                    const D2D1_RECT_F selection_rect = D2D1::RectF(
                        metric.left,
                        metric.top,
                        metric.left + metric.width,
                        metric.top + metric.height);
                    context->FillRectangle(selection_rect, brush);
                }
            }
        }
    }

    brush->SetColor(use_placeholder ? D2D1::ColorF(0.58F, 0.66F, 0.80F, 1.0F) : D2D1::ColorF(0.96F, 0.98F, 1.0F, 1.0F));
    context->DrawTextLayout(D2D1::Point2F(text_rect.left, text_rect.top), layout, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

    if (!use_placeholder && focused && caret_visible) {
        FLOAT caret_x = text_rect.left;
        FLOAT caret_y = text_rect.top;
        DWRITE_HIT_TEST_METRICS caret_metrics {};
        if (SUCCEEDED(layout->HitTestTextPosition(static_cast<UINT32>(text_box.caret_position()), FALSE, &caret_x, &caret_y, &caret_metrics))) {
            brush->SetColor(D2D1::ColorF(0.96F, 0.98F, 1.0F, 1.0F));
            context->DrawLine(
                D2D1::Point2F(text_rect.left + caret_x, text_rect.top + 4.0F),
                D2D1::Point2F(text_rect.left + caret_x, text_rect.bottom - 4.0F),
                brush,
                1.6F);
        }
    }

    safe_release(layout);
}

}  // namespace

WindowRenderTarget::WindowRenderTarget(RenderManager* render_manager, WindowHost* window_host)
    : render_manager_(render_manager),
      window_host_(window_host),
      bridge_(render_manager != nullptr ? render_manager->create_composition_bridge() : CompositionBridge(nullptr)) {}

WindowRenderTarget::~WindowRenderTarget() {
    cleanup_dx11_dcomp_target();
}

void WindowRenderTarget::set_overlay_scene(OverlayScene scene) {
    overlay_scene_ = std::move(scene);
}

void WindowRenderTarget::set_interactive_controls(InteractiveControls controls) {
    interactive_controls_ = std::move(controls);
    interactive_mode_enabled_ = interactive_controls_.primary_button != nullptr
        && interactive_controls_.text_box != nullptr
        && interactive_controls_.check_box != nullptr
        && interactive_controls_.combo_box != nullptr
        && interactive_controls_.slider != nullptr;
    focused_control_index_ = interactive_mode_enabled_ ? kTextBoxFocusIndex : 0;
    sync_focus_state();
}

void WindowRenderTarget::set_primary_action_handler(std::function<void()> handler) {
    primary_action_handler_ = std::move(handler);
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

        if (!ensure_swap_chain_size()) {
            render_manager_->diagnostics().log(LogLevel::Error, "Swapchain resize/rebuild failed");
            return false;
        }

        const auto size = window_host_->client_size();
        const float width = size.width > 0.0F ? size.width : 1280.0F;
        const float height = size.height > 0.0F ? size.height : 720.0F;

        // Background tone responds to current window size so resize has immediate visual feedback.
        const float size_ratio = clamp_value((width + height) / 2600.0F, 0.4F, 1.8F);
        const float clear_color[4] {
            0.05F + 0.03F * size_ratio,
            0.08F + 0.05F * size_ratio,
            0.14F + 0.04F * size_ratio,
            1.0F,
        };
        d3d_context_->OMSetRenderTargets(1, &render_target_view_, nullptr);
        d3d_context_->ClearRenderTargetView(render_target_view_, clear_color);

        POINT cursor_point {};
        bool pointer_valid = false;
        if (window_host_->hwnd() != nullptr && GetCursorPos(&cursor_point)) {
            if (ScreenToClient(window_host_->hwnd(), &cursor_point)) {
                pointer_valid = true;
            }
        }

        const bool pointer_inside = pointer_valid && cursor_point.x >= 0 && cursor_point.y >= 0
            && cursor_point.x < static_cast<LONG>(width) && cursor_point.y < static_cast<LONG>(height);
        const float pointer_x = static_cast<float>(cursor_point.x);
        const float pointer_y = static_cast<float>(cursor_point.y);
        const std::vector<std::wstring> items = overlay_scene_.items;
        const OverlayLayout layout = compute_layout(width, height, interactive_controls_);
        const HitResult hovered = pointer_inside ? hit_test(layout, pointer_x, pointer_y) : HitResult {};
        button_hovered_ = hovered.target == HitTarget::PrimaryButton;
        text_box_hovered_ = hovered.target == HitTarget::TextBox;
        check_box_hovered_ = hovered.target == HitTarget::CheckBox;
        combo_box_hovered_ = hovered.target == HitTarget::ComboBoxHeader || hovered.target == HitTarget::ComboBoxItem;
        slider_hovered_ = hovered.target == HitTarget::Slider;
        hovered_combo_index_ = hovered.target == HitTarget::ComboBoxItem ? hovered.combo_index : -1;
        hovered_item_index_ = hovered_combo_index_;

        if (d2d_context_ == nullptr || d2d_target_bitmap_ == nullptr || d2d_brush_ == nullptr) {
            render_manager_->diagnostics().log(LogLevel::Error, "D2D overlay context unavailable");
            return false;
        }

            d2d_context_->BeginDraw();
            d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());

            d2d_brush_->SetColor(D2D1::ColorF(0.10F, 0.16F, 0.26F, 1.0F));
            const D2D1_RECT_F card_rect = interactive_mode_enabled_
                ? layout.card
                : D2D1::RectF(width * 0.1F, height * 0.1F, width * 0.9F, height * 0.65F);
            const D2D1_ROUNDED_RECT card = D2D1::RoundedRect(
                card_rect,
                20.0F,
                20.0F);
            d2d_context_->FillRoundedRectangle(card, d2d_brush_);

            d2d_brush_->SetColor(D2D1::ColorF(0.00F, 0.72F, 0.86F, 1.0F));
            d2d_context_->DrawRoundedRectangle(card, d2d_brush_, 2.0F);

            d2d_brush_->SetColor(D2D1::ColorF(0.96F, 0.98F, 1.0F, 1.0F));
            if (text_format_ != nullptr) {
                const std::wstring title = interactive_mode_enabled_ ? L"DCompFrame Interactive Demo" : overlay_scene_.title;
                const D2D1_RECT_F title_rect = interactive_mode_enabled_
                    ? layout.title
                    : D2D1::RectF(width * 0.13F, height * 0.14F, width * 0.85F, height * 0.22F);
                draw_centered_text(d2d_context_, d2d_brush_, text_format_, title, title_rect);
            }

            const D2D1_RECT_F content_clip_rect = inset_rect(card_rect, 14.0F, 14.0F);
            d2d_context_->PushAxisAlignedClip(content_clip_rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            if (interactive_mode_enabled_ && interactive_controls_.text_box != nullptr && interactive_controls_.check_box != nullptr
                && interactive_controls_.combo_box != nullptr && interactive_controls_.slider != nullptr
                && interactive_controls_.primary_button != nullptr) {
                if (item_text_format_ != nullptr) {
                    d2d_brush_->SetColor(D2D1::ColorF(0.68F, 0.78F, 0.98F, 1.0F));
                    draw_centered_text(
                        d2d_context_,
                        d2d_brush_,
                        item_text_format_,
                        L"点击右上角按钮可创建新窗口；TextBox 支持输入、选区、复制粘贴；其余控件支持键鼠交互。",
                        layout.subtitle);
                }

                D2D1_COLOR_F button_color = D2D1::ColorF(0.10F, 0.55F, 0.95F, 1.0F);
                if (button_pressed_) {
                    button_color = D2D1::ColorF(0.08F, 0.42F, 0.78F, 1.0F);
                } else if (button_hovered_) {
                    button_color = D2D1::ColorF(0.16F, 0.62F, 1.0F, 1.0F);
                }
                fill_rounded_rect(d2d_context_, d2d_brush_, layout.button, button_color, 14.0F);
                if (focused_control_index_ == kPrimaryButtonFocusIndex) {
                    stroke_rounded_rect(d2d_context_, d2d_brush_, inset_rect(layout.button, -2.0F, -2.0F), D2D1::ColorF(0.88F, 0.96F, 1.0F, 1.0F), 2.0F, 16.0F);
                }
                if (item_text_format_ != nullptr) {
                    draw_centered_text(
                        d2d_context_,
                        d2d_brush_,
                        item_text_format_,
                        utf8_to_wstring(interactive_controls_.primary_button->text()),
                        layout.button);
                }

                draw_text_box_editor(
                    d2d_context_,
                    d2d_brush_,
                    dwrite_factory_,
                    input_text_format_,
                    *interactive_controls_.text_box,
                    layout.text_box,
                    text_box_hovered_,
                    focused_control_index_ == kTextBoxFocusIndex,
                    ((GetTickCount64() - caret_blink_seed_) / 500ULL) % 2ULL == 0ULL);

                fill_rounded_rect(d2d_context_, d2d_brush_, layout.check_box, check_box_hovered_ ? D2D1::ColorF(0.18F, 0.26F, 0.40F, 1.0F) : D2D1::ColorF(0.14F, 0.20F, 0.32F, 1.0F));
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.check_box, focused_control_index_ == kCheckBoxFocusIndex ? D2D1::ColorF(0.10F, 0.74F, 0.96F, 1.0F) : D2D1::ColorF(0.42F, 0.56F, 0.82F, 0.8F));
                draw_checkbox_glyph(d2d_context_, d2d_brush_, layout.check_box, interactive_controls_.check_box->checked());
                draw_label_value(
                    d2d_context_,
                    d2d_brush_,
                    item_text_format_,
                    L"CheckBox",
                    interactive_controls_.check_box->checked() ? L"已启用扩展行为" : L"未启用扩展行为",
                    layout.check_box,
                    52.0F);

                fill_rounded_rect(d2d_context_, d2d_brush_, layout.combo_box, combo_box_hovered_ ? D2D1::ColorF(0.18F, 0.26F, 0.40F, 1.0F) : D2D1::ColorF(0.14F, 0.20F, 0.32F, 1.0F));
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.combo_box, focused_control_index_ == kComboBoxFocusIndex ? D2D1::ColorF(0.10F, 0.74F, 0.96F, 1.0F) : D2D1::ColorF(0.42F, 0.56F, 0.82F, 0.8F));
                draw_combobox_glyph(d2d_context_, d2d_brush_, layout.combo_box);
                draw_label_value(
                    d2d_context_,
                    d2d_brush_,
                    item_text_format_,
                    L"ComboBox",
                    utf8_to_wstring(interactive_controls_.combo_box->selected_text()),
                    layout.combo_box);

                if (interactive_controls_.combo_box->is_dropdown_open()) {
                    fill_rounded_rect(d2d_context_, d2d_brush_, layout.combo_dropdown, D2D1::ColorF(0.12F, 0.18F, 0.28F, 0.98F), 12.0F);
                    stroke_rounded_rect(d2d_context_, d2d_brush_, layout.combo_dropdown, D2D1::ColorF(0.38F, 0.52F, 0.76F, 1.0F), 1.0F, 12.0F);
                    for (std::size_t index = 0; index < layout.combo_items.size(); ++index) {
                        const bool item_hovered = hovered_combo_index_ == static_cast<int>(index);
                        const bool selected = interactive_controls_.combo_box->selected_index() && *interactive_controls_.combo_box->selected_index() == index;
                        fill_rounded_rect(
                            d2d_context_,
                            d2d_brush_,
                            inset_rect(layout.combo_items[index], 4.0F, 2.0F),
                            item_hovered ? D2D1::ColorF(0.20F, 0.34F, 0.54F, 1.0F) : (selected ? D2D1::ColorF(0.16F, 0.28F, 0.46F, 1.0F) : D2D1::ColorF(0.10F, 0.16F, 0.26F, 1.0F)),
                            6.0F);
                        draw_centered_text(
                            d2d_context_,
                            d2d_brush_,
                            item_text_format_,
                            utf8_to_wstring(interactive_controls_.combo_box->items()[index]),
                            layout.combo_items[index]);
                    }
                }

                fill_rounded_rect(d2d_context_, d2d_brush_, layout.slider, slider_hovered_ ? D2D1::ColorF(0.18F, 0.26F, 0.40F, 1.0F) : D2D1::ColorF(0.14F, 0.20F, 0.32F, 1.0F));
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.slider, focused_control_index_ == kSliderFocusIndex ? D2D1::ColorF(0.10F, 0.74F, 0.96F, 1.0F) : D2D1::ColorF(0.42F, 0.56F, 0.82F, 0.8F));
                draw_slider_glyph(d2d_context_, d2d_brush_, layout.slider_track, interactive_controls_.slider->normalized_value());
                draw_label_value(
                    d2d_context_,
                    d2d_brush_,
                    item_text_format_,
                    L"Slider",
                    utf8_to_wstring(std::to_string(interactive_controls_.slider->value())),
                    layout.slider);

                if (interactive_controls_.scroll_viewer != nullptr) {
                    fill_rounded_rect(d2d_context_, d2d_brush_, layout.scroll_viewer, D2D1::ColorF(0.14F, 0.20F, 0.32F, 1.0F));
                    stroke_rounded_rect(d2d_context_, d2d_brush_, layout.scroll_viewer, D2D1::ColorF(0.42F, 0.56F, 0.82F, 0.8F));
                    draw_scrollviewer_glyph(d2d_context_, d2d_brush_, layout.scroll_viewer);
                    draw_label_value(
                        d2d_context_,
                        d2d_brush_,
                        item_text_format_,
                        L"ScrollViewer",
                        L"滚动内容示例",
                        layout.scroll_viewer);
                }

                fill_rounded_rect(d2d_context_, d2d_brush_, layout.footer, D2D1::ColorF(0.12F, 0.18F, 0.28F, 1.0F), 14.0F);
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.footer, D2D1::ColorF(0.32F, 0.46F, 0.72F, 0.95F), 1.0F, 14.0F);
                const std::wstring footer = L"标题绑定: " + utf8_to_wstring(interactive_controls_.text_box->text())
                    + L"   |   Combo: " + utf8_to_wstring(interactive_controls_.combo_box->selected_text())
                    + L"   |   Slider: " + utf8_to_wstring(std::to_string(interactive_controls_.slider->value()))
                    + L"   |   CreateWindow 点击次数: " + std::to_wstring(button_click_count_);
                draw_centered_text(d2d_context_, d2d_brush_, item_text_format_, footer, layout.footer);
            } else {
                for (std::size_t i = 0; i < items.size(); ++i) {
                    const float top = height * 0.25F + static_cast<float>(i) * 52.0F;
                    const D2D1_RECT_F control_rect = D2D1::RectF(width * 0.14F, top, width * 0.70F, top + 40.0F);
                    const D2D1_ROUNDED_RECT control = D2D1::RoundedRect(
                        control_rect,
                        10.0F,
                        10.0F);
                    const bool hovered_item = static_cast<int>(i) == hovered_item_index_;
                    d2d_brush_->SetColor(
                        hovered_item
                            ? D2D1::ColorF(0.30F, 0.45F, 0.70F, 1.0F)
                            : (i == 0 ? D2D1::ColorF(0.24F, 0.34F, 0.54F, 1.0F) : D2D1::ColorF(0.22F, 0.28F, 0.38F, 1.0F)));
                    d2d_context_->FillRoundedRectangle(control, d2d_brush_);

                    if (items[i].find(L"CheckBox:") == 0) {
                        draw_checkbox_glyph(d2d_context_, d2d_brush_, control_rect, items[i].find(L"checked") != std::wstring::npos);
                    } else if (items[i].find(L"ComboBox:") == 0) {
                        draw_combobox_glyph(d2d_context_, d2d_brush_, control_rect);
                    } else if (items[i].find(L"Slider:") == 0) {
                        draw_slider_glyph(d2d_context_, d2d_brush_, control_rect, 0.75F);
                    } else if (items[i].find(L"ScrollViewer:") == 0) {
                        draw_scrollviewer_glyph(d2d_context_, d2d_brush_, control_rect);
                    }

                    if (item_text_format_ != nullptr) {
                        d2d_brush_->SetColor(D2D1::ColorF(0.92F, 0.95F, 1.0F, 1.0F));
                        draw_centered_text(d2d_context_, d2d_brush_, item_text_format_, items[i], D2D1::RectF(width * 0.16F, top + 4.0F, width * 0.68F, top + 36.0F));
                    }
                }

                const D2D1_RECT_F action_rect = D2D1::RectF(width * 0.73F, height * 0.50F, width * 0.86F, height * 0.58F);
                fill_rounded_rect(d2d_context_, d2d_brush_, action_rect, button_hovered_ ? D2D1::ColorF(0.16F, 0.62F, 1.0F, 1.0F) : D2D1::ColorF(0.10F, 0.55F, 0.95F, 1.0F), 12.0F);
                if (item_text_format_ != nullptr) {
                    d2d_brush_->SetColor(D2D1::ColorF(0.96F, 0.98F, 1.0F, 1.0F));
                    draw_centered_text(d2d_context_, d2d_brush_, item_text_format_, L"Start", action_rect);
                }
            }

            d2d_context_->PopAxisAlignedClip();

            const HRESULT end_draw_hr = d2d_context_->EndDraw();
            if (FAILED(end_draw_hr)) {
                if (end_draw_hr == D2DERR_RECREATE_TARGET) {
                    safe_release(d2d_brush_);
                    safe_release(d2d_target_bitmap_);
                    if (!recreate_d2d_target()) {
                        render_manager_->diagnostics().log(LogLevel::Error, "D2D target recreation failed after EndDraw");
                    }
                }
                render_manager_->diagnostics().log(LogLevel::Error, "D2D overlay draw failed (hr=" + hr_to_string(end_draw_hr) + ")");
                return false;
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

        render_manager_->notify_commit();
        render_manager_->enqueue_command(RenderCommand {.type = RenderCommandType::Present, .payload = "dx11-present"});
        render_manager_->diagnostics().record_commit();
        render_manager_->diagnostics().record_frame(std::chrono::milliseconds {16});
        render_manager_->diagnostics().update_resource_peak(render_manager_->resource_manager().total_bytes());
        ++presented_frames_;
        return true;
    }

    const bool committed = bridge_.commit_changes(has_dirty_changes);
    if (committed) {
        render_manager_->enqueue_command(RenderCommand {.type = RenderCommandType::Commit, .payload = "bridge-commit"});
        ++presented_frames_;
    }

    return committed;
}

bool WindowRenderTarget::handle_window_message(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& result) {
    if (!interactive_mode_enabled_ || window_host_ == nullptr || window_host_->hwnd() == nullptr || !is_ready()) {
        return false;
    }

    const auto size = window_host_->client_size();
    const OverlayLayout layout = compute_layout(size.width > 0.0F ? size.width : 1280.0F, size.height > 0.0F ? size.height : 720.0F, interactive_controls_);
    const auto update_focus = [&](std::size_t index) {
        focused_control_index_ = min_value(index, kInteractiveFocusCount - 1U);
        sync_focus_state();
        reset_caret_blink();
        window_host_->request_render();
    };
    const auto focus_next = [&](bool reverse) {
        const std::size_t count = kInteractiveFocusCount;
        focused_control_index_ = reverse
            ? (focused_control_index_ + count - 1U) % count
            : (focused_control_index_ + 1U) % count;
        sync_focus_state();
        reset_caret_blink();
        window_host_->request_render();
    };
    const auto update_slider_from_x = [&](float x) {
        const float track_width = max_value(1.0F, rect_width(layout.slider_track));
        const float ratio = clamp_value((x - layout.slider_track.left) / track_width, 0.0F, 1.0F);
        interactive_controls_.slider->set_value_from_ratio(ratio);
        window_host_->request_render();
    };
    const auto update_text_box_selection = [&](float x, float y, bool extend) {
        const std::wstring wide_text = utf8_to_wstring(interactive_controls_.text_box->text());
        const std::size_t position = text_box_hit_index(dwrite_factory_, input_text_format_, wide_text, layout.text_box_text, x, y);
        interactive_controls_.text_box->set_caret_position(position, extend);
        reset_caret_blink();
        window_host_->request_render();
    };

    switch (msg) {
    case WM_SETFOCUS:
        sync_focus_state();
        reset_caret_blink();
        result = 0;
        return true;
    case WM_KILLFOCUS:
        text_box_selecting_ = false;
        slider_dragging_ = false;
        button_pressed_ = false;
        combo_box_pressed_ = false;
        result = 0;
        return true;
    case WM_TIMER:
        if (wparam == kCaretTimerId) {
            window_host_->request_render();
        }
        return false;
    case WM_MOUSEMOVE: {
        const float x = static_cast<float>(GET_X_LPARAM(lparam));
        const float y = static_cast<float>(GET_Y_LPARAM(lparam));
        const HitResult hit = hit_test(layout, x, y);
        button_hovered_ = hit.target == HitTarget::PrimaryButton;
        text_box_hovered_ = hit.target == HitTarget::TextBox;
        check_box_hovered_ = hit.target == HitTarget::CheckBox;
        combo_box_hovered_ = hit.target == HitTarget::ComboBoxHeader || hit.target == HitTarget::ComboBoxItem;
        slider_hovered_ = hit.target == HitTarget::Slider;
        hovered_combo_index_ = hit.target == HitTarget::ComboBoxItem ? hit.combo_index : -1;
        if (text_box_selecting_) {
            update_text_box_selection(x, y, true);
        }
        if (slider_dragging_) {
            update_slider_from_x(x);
        }
        result = 0;
        return true;
    }
    case WM_LBUTTONDOWN: {
        const float x = static_cast<float>(GET_X_LPARAM(lparam));
        const float y = static_cast<float>(GET_Y_LPARAM(lparam));
        SetFocus(window_host_->hwnd());
        SetCapture(window_host_->hwnd());
        const HitResult hit = hit_test(layout, x, y);
        switch (hit.target) {
        case HitTarget::PrimaryButton:
            update_focus(kPrimaryButtonFocusIndex);
            button_pressed_ = true;
            close_combo_box();
            break;
        case HitTarget::TextBox:
            update_focus(kTextBoxFocusIndex);
            text_box_selecting_ = true;
            update_text_box_selection(x, y, false);
            close_combo_box();
            break;
        case HitTarget::CheckBox:
            update_focus(kCheckBoxFocusIndex);
            interactive_controls_.check_box->toggle();
            close_combo_box();
            break;
        case HitTarget::ComboBoxHeader:
            update_focus(kComboBoxFocusIndex);
            interactive_controls_.combo_box->toggle_dropdown();
            combo_box_pressed_ = true;
            break;
        case HitTarget::ComboBoxItem:
            update_focus(kComboBoxFocusIndex);
            interactive_controls_.combo_box->set_selected_index(static_cast<std::size_t>(hit.combo_index));
            close_combo_box();
            break;
        case HitTarget::Slider:
            update_focus(kSliderFocusIndex);
            slider_dragging_ = true;
            update_slider_from_x(x);
            close_combo_box();
            break;
        case HitTarget::None:
            close_combo_box();
            break;
        }
        result = 0;
        return true;
    }
    case WM_LBUTTONUP: {
        const float x = static_cast<float>(GET_X_LPARAM(lparam));
        const float y = static_cast<float>(GET_Y_LPARAM(lparam));
        const HitResult hit = hit_test(layout, x, y);
        if (button_pressed_ && hit.target == HitTarget::PrimaryButton) {
            perform_primary_action();
        }
        button_pressed_ = false;
        combo_box_pressed_ = false;
        text_box_selecting_ = false;
        slider_dragging_ = false;
        if (GetCapture() == window_host_->hwnd()) {
            ReleaseCapture();
        }
        result = 0;
        return true;
    }
    case WM_CHAR:
        if (focused_control_index_ == kTextBoxFocusIndex) {
            const wchar_t wchar = static_cast<wchar_t>(wparam);
            if (wchar >= 32 && wchar != 127) {
                interactive_controls_.text_box->insert_text(wstring_to_utf8(std::wstring(1, wchar)));
                reset_caret_blink();
                result = 0;
                return true;
            }
        }
        break;
    case WM_KEYDOWN: {
        const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        switch (wparam) {
        case VK_TAB:
            focus_next(shift);
            result = 0;
            return true;
        case VK_ESCAPE:
            close_combo_box();
            result = 0;
            return true;
        default:
            break;
        }

        if (focused_control_index_ == kPrimaryButtonFocusIndex) {
            if (wparam == VK_SPACE || wparam == VK_RETURN) {
                perform_primary_action();
                result = 0;
                return true;
            }
        }

        if (focused_control_index_ == kTextBoxFocusIndex) {
            if (ctrl && (wparam == 'A' || wparam == 'C' || wparam == 'V' || wparam == 'X')) {
                if (wparam == 'A') {
                    interactive_controls_.text_box->select_all();
                } else if (wparam == 'C') {
                    write_clipboard_text(window_host_->hwnd(), selected_text(*interactive_controls_.text_box));
                } else if (wparam == 'V') {
                    const std::wstring pasted = read_clipboard_text(window_host_->hwnd());
                    if (!pasted.empty()) {
                        interactive_controls_.text_box->insert_text(wstring_to_utf8(pasted));
                    }
                } else if (wparam == 'X') {
                    write_clipboard_text(window_host_->hwnd(), selected_text(*interactive_controls_.text_box));
                    interactive_controls_.text_box->delete_forward();
                }
                reset_caret_blink();
                result = 0;
                return true;
            }

            switch (wparam) {
            case VK_LEFT:
                interactive_controls_.text_box->move_caret_left(shift);
                break;
            case VK_RIGHT:
                interactive_controls_.text_box->move_caret_right(shift);
                break;
            case VK_HOME:
                interactive_controls_.text_box->move_caret_home(shift);
                break;
            case VK_END:
                interactive_controls_.text_box->move_caret_end(shift);
                break;
            case VK_BACK:
                interactive_controls_.text_box->backspace();
                break;
            case VK_DELETE:
                interactive_controls_.text_box->delete_forward();
                break;
            default:
                break;
            }
            reset_caret_blink();
            result = 0;
            return true;
        }

        if (focused_control_index_ == kCheckBoxFocusIndex) {
            if (wparam == VK_SPACE) {
                interactive_controls_.check_box->toggle();
                result = 0;
                return true;
            }
        }

        if (focused_control_index_ == kComboBoxFocusIndex) {
            if (wparam == VK_SPACE || wparam == VK_RETURN) {
                interactive_controls_.combo_box->toggle_dropdown();
                result = 0;
                return true;
            }
            if (wparam == VK_DOWN) {
                interactive_controls_.combo_box->open_dropdown();
                interactive_controls_.combo_box->select_next();
                result = 0;
                return true;
            }
            if (wparam == VK_UP) {
                interactive_controls_.combo_box->open_dropdown();
                interactive_controls_.combo_box->select_previous();
                result = 0;
                return true;
            }
        }

        if (focused_control_index_ == kSliderFocusIndex) {
            if (wparam == VK_LEFT) {
                interactive_controls_.slider->step_by(-1.0F);
                result = 0;
                return true;
            }
            if (wparam == VK_RIGHT) {
                interactive_controls_.slider->step_by(1.0F);
                result = 0;
                return true;
            }
            if (wparam == VK_HOME) {
                interactive_controls_.slider->set_value(interactive_controls_.slider->min_value());
                result = 0;
                return true;
            }
            if (wparam == VK_END) {
                interactive_controls_.slider->set_value(interactive_controls_.slider->max_value());
                result = 0;
                return true;
            }
        }
        break;
    }
    default:
        break;
    }

    return false;
}

void WindowRenderTarget::sync_focus_state() {
    if (interactive_controls_.primary_button) {
        interactive_controls_.primary_button->set_focused(focused_control_index_ == kPrimaryButtonFocusIndex);
    }
    if (interactive_controls_.text_box) {
        interactive_controls_.text_box->set_focused(focused_control_index_ == kTextBoxFocusIndex);
    }
    if (interactive_controls_.check_box) {
        interactive_controls_.check_box->set_focused(focused_control_index_ == kCheckBoxFocusIndex);
    }
    if (interactive_controls_.combo_box) {
        interactive_controls_.combo_box->set_focused(focused_control_index_ == kComboBoxFocusIndex);
    }
    if (interactive_controls_.slider) {
        interactive_controls_.slider->set_focused(focused_control_index_ == kSliderFocusIndex);
    }
}

void WindowRenderTarget::reset_caret_blink() {
    caret_blink_seed_ = GetTickCount64();
    if (window_host_ != nullptr && window_host_->hwnd() != nullptr) {
        SetTimer(window_host_->hwnd(), kCaretTimerId, 450U, nullptr);
    }
}

void WindowRenderTarget::close_combo_box() {
    if (interactive_controls_.combo_box && interactive_controls_.combo_box->is_dropdown_open()) {
        interactive_controls_.combo_box->close_dropdown();
        hovered_combo_index_ = -1;
    }
}

void WindowRenderTarget::perform_primary_action() {
    if (interactive_controls_.primary_button) {
        interactive_controls_.primary_button->click();
    }
    ++button_click_count_;
    if (primary_action_handler_) {
        primary_action_handler_();
    }
    window_host_->request_render();
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

    d3d_context_->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&d3d_context1_));

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
    swap_chain_width_ = swap_chain_desc.Width;
    swap_chain_height_ = swap_chain_desc.Height;

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
        render_manager_->diagnostics().log(LogLevel::Error, "D2D overlay initialization failed");
        cleanup_dx11_dcomp_target();
        return false;
    }

    return true;
}

bool WindowRenderTarget::ensure_swap_chain_size() {
    if (swap_chain_ == nullptr || d3d_context_ == nullptr || window_host_ == nullptr) {
        return false;
    }

    const Size size = window_host_->client_size();
    const UINT target_width = static_cast<UINT>(size.width > 0.0F ? size.width : 1.0F);
    const UINT target_height = static_cast<UINT>(size.height > 0.0F ? size.height : 1.0F);
    if (target_width == swap_chain_width_ && target_height == swap_chain_height_) {
        return true;
    }

    ID3D11RenderTargetView* null_target = nullptr;
    d3d_context_->OMSetRenderTargets(1, &null_target, nullptr);
    if (d2d_context_ != nullptr) {
        d2d_context_->SetTarget(nullptr);
    }

    safe_release(d2d_brush_);
    safe_release(d2d_target_bitmap_);
    safe_release(render_target_view_);

    const HRESULT resize_hr = swap_chain_->ResizeBuffers(0, target_width, target_height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(resize_hr)) {
        render_manager_->diagnostics().log(LogLevel::Error, "ResizeBuffers failed (hr=" + hr_to_string(resize_hr) + ")");
        return false;
    }

    swap_chain_width_ = target_width;
    swap_chain_height_ = target_height;

    if (!recreate_render_target_view()) {
        return false;
    }

    if (d2d_context_ != nullptr && !recreate_d2d_target()) {
        render_manager_->diagnostics().log(LogLevel::Warning, "D2D target recreation failed after ResizeBuffers");
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
            render_manager_->diagnostics().log(LogLevel::Info, "D2D1CreateFactory failed (hr=" + hr_to_string(factory_hr) + ")");
            return false;
        }
    }

    if (dwrite_factory_ == nullptr) {
        const HRESULT write_hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&dwrite_factory_));
        if (FAILED(write_hr) || dwrite_factory_ == nullptr) {
            dwrite_factory_ = nullptr;
            text_format_ = nullptr;
            item_text_format_ = nullptr;
        } else {
            const HRESULT format_hr = dwrite_factory_->CreateTextFormat(
                L"Segoe UI",
                nullptr,
                DWRITE_FONT_WEIGHT_SEMI_BOLD,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                20.0F,
                L"en-us",
                &text_format_);
            if (FAILED(format_hr) || text_format_ == nullptr) {
                text_format_ = nullptr;
            } else {
                text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }

            const HRESULT item_format_hr = dwrite_factory_->CreateTextFormat(
                L"Segoe UI",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                14.0F,
                L"en-us",
                &item_text_format_);
            if (FAILED(item_format_hr) || item_text_format_ == nullptr) {
                item_text_format_ = nullptr;
            } else {
                item_text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                item_text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }

            const HRESULT input_format_hr = dwrite_factory_->CreateTextFormat(
                L"Segoe UI",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                18.0F,
                L"en-us",
                &input_text_format_);
            if (FAILED(input_format_hr) || input_text_format_ == nullptr) {
                input_text_format_ = item_text_format_;
                if (input_text_format_ != nullptr) {
                    input_text_format_->AddRef();
                }
            } else {
                input_text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                input_text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }
        }
    }

    if (d2d_device_ == nullptr || d2d_context_ == nullptr) {
        IDXGIDevice* dxgi_device = nullptr;
        const HRESULT qhr = d3d_device_->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device));
        if (FAILED(qhr) || dxgi_device == nullptr) {
            safe_release(dxgi_device);
            render_manager_->diagnostics().log(LogLevel::Info, "IDXGIDevice query for D2D failed (hr=" + hr_to_string(qhr) + ")");
            return false;
        }

        const HRESULT device_hr = d2d_factory_->CreateDevice(dxgi_device, &d2d_device_);
        safe_release(dxgi_device);
        if (FAILED(device_hr) || d2d_device_ == nullptr) {
            render_manager_->diagnostics().log(LogLevel::Info, "ID2D1Factory1::CreateDevice failed (hr=" + hr_to_string(device_hr) + ")");
            return false;
        }

        const HRESULT context_hr = d2d_device_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2d_context_);
        if (FAILED(context_hr) || d2d_context_ == nullptr) {
            render_manager_->diagnostics().log(LogLevel::Info, "ID2D1Device::CreateDeviceContext failed (hr=" + hr_to_string(context_hr) + ")");
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

    const auto create_target_bitmap = [&](D2D1_ALPHA_MODE alpha_mode) {
        const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, alpha_mode),
            96.0F,
            96.0F);
        return d2d_context_->CreateBitmapFromDxgiSurface(surface, &props, &d2d_target_bitmap_);
    };

    HRESULT bitmap_hr = create_target_bitmap(D2D1_ALPHA_MODE_PREMULTIPLIED);
    if (FAILED(bitmap_hr) || d2d_target_bitmap_ == nullptr) {
        safe_release(d2d_target_bitmap_);
        bitmap_hr = create_target_bitmap(D2D1_ALPHA_MODE_IGNORE);
    }
    safe_release(surface);
    if (FAILED(bitmap_hr) || d2d_target_bitmap_ == nullptr) {
        render_manager_->diagnostics().log(LogLevel::Info, "CreateBitmapFromDxgiSurface failed (hr=" + hr_to_string(bitmap_hr) + ")");
        return false;
    }

    d2d_context_->SetTarget(d2d_target_bitmap_);
    const HRESULT brush_hr = d2d_context_->CreateSolidColorBrush(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), &d2d_brush_);
    return SUCCEEDED(brush_hr) && d2d_brush_ != nullptr;
}

void WindowRenderTarget::cleanup_dx11_dcomp_target() {
    safe_release(d2d_brush_);
    safe_release(d2d_target_bitmap_);
    safe_release(input_text_format_);
    safe_release(item_text_format_);
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
    safe_release(d3d_context1_);
    safe_release(d3d_context_);
    safe_release(d3d_device_);
    using_dx11_dcomp_ = false;
}

}  // namespace dcompframe
