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
constexpr std::size_t kRichTextBoxFocusIndex = 2;
constexpr std::size_t kCheckBoxFocusIndex = 3;
constexpr std::size_t kComboBoxFocusIndex = 4;
constexpr std::size_t kSliderFocusIndex = 5;
constexpr std::size_t kInteractiveFocusCount = 6;
constexpr UINT_PTR kCaretTimerId = 1;

enum class HitTarget {
    None,
    PrimaryButton,
    TextBox,
    RichTextBox,
    CheckBox,
    ComboBoxHeader,
    ComboBoxItem,
    Slider,
    ScrollViewerThumb,
    ListViewThumb,
    ItemsControlThumb,
    ScrollViewer,
    ListView,
    ItemsControl
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
    D2D1_RECT_F left_column {};
    D2D1_RECT_F right_column {};
    D2D1_RECT_F text_box_label {};
    D2D1_RECT_F text_box {};
    D2D1_RECT_F text_box_text {};
    D2D1_RECT_F rich_text_label {};
    D2D1_RECT_F rich_text_box {};
    D2D1_RECT_F rich_text_viewport {};
    D2D1_RECT_F check_box {};
    D2D1_RECT_F combo_label {};
    D2D1_RECT_F combo_box {};
    D2D1_RECT_F slider_label {};
    D2D1_RECT_F slider {};
    D2D1_RECT_F text_block_label {};
    D2D1_RECT_F text_block {};
    D2D1_RECT_F image_label {};
    D2D1_RECT_F image {};
    D2D1_RECT_F card_preview {};
    D2D1_RECT_F list_view_label {};
    D2D1_RECT_F list_view {};
    D2D1_RECT_F list_viewport {};
    D2D1_RECT_F list_view_thumb {};
    D2D1_RECT_F items_control_label {};
    D2D1_RECT_F items_control {};
    D2D1_RECT_F items_control_viewport {};
    D2D1_RECT_F items_control_thumb {};
    D2D1_RECT_F scroll_viewer_label {};
    D2D1_RECT_F scroll_viewer {};
    D2D1_RECT_F scroll_viewport {};
    D2D1_RECT_F scroll_viewer_thumb {};
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

bool rect_has_area(const D2D1_RECT_F& rect) {
    return rect.right > rect.left && rect.bottom > rect.top;
}

float compute_items_total_height(std::size_t item_count, float item_height, float item_spacing) {
    if (item_count == 0U) {
        return 0.0F;
    }

    return static_cast<float>(item_count) * item_height + static_cast<float>(item_count > 0U ? item_count - 1U : 0U) * item_spacing;
}

D2D1_RECT_F compute_vertical_thumb(const D2D1_RECT_F& viewport, float total_height, float scroll_offset, float width = 6.0F) {
    if (!rect_has_area(viewport) || total_height <= rect_height(viewport)) {
        return D2D1::RectF(0.0F, 0.0F, 0.0F, 0.0F);
    }

    const float track_height = rect_height(viewport);
    const float thumb_height = max_value(28.0F, track_height * (track_height / max_value(track_height, total_height)));
    const float max_offset = max_value(1.0F, total_height - track_height);
    const float thumb_top = viewport.top + (track_height - thumb_height) * (clamp_value(scroll_offset, 0.0F, max_offset) / max_offset);
    return D2D1::RectF(viewport.right - width, thumb_top, viewport.right, thumb_top + thumb_height);
}

OverlayLayout compute_layout(float width, float height, const WindowRenderTarget::InteractiveControls& controls) {
    OverlayLayout layout;
    const float outer_margin_x = clamp_value(width * 0.008F, 6.0F, 14.0F);
    const float outer_margin_y = clamp_value(height * 0.010F, 6.0F, 14.0F);
    layout.card = D2D1::RectF(outer_margin_x, outer_margin_y, width - outer_margin_x, height - outer_margin_y);
    layout.title = D2D1::RectF(layout.card.left + 28.0F, layout.card.top + 16.0F, layout.card.right - 220.0F, layout.card.top + 54.0F);
    layout.subtitle = D2D1::RectF(layout.card.left + 28.0F, layout.card.top + 54.0F, layout.card.right - 220.0F, layout.card.top + 92.0F);
    layout.button = D2D1::RectF(layout.card.right - 172.0F, layout.card.top + 18.0F, layout.card.right - 28.0F, layout.card.top + 58.0F);

    const float inner_left = layout.card.left + 24.0F;
    const float inner_right = layout.card.right - 24.0F;
    const float top = layout.card.top + 104.0F;
    const float column_gap = 20.0F;
    const float left_width = (inner_right - inner_left - column_gap) * 0.47F;
    const float label_height = 18.0F;
    const float control_height = 40.0F;
    const float control_gap = 14.0F;
    const float card_height = rect_height(layout.card);
    const float scroll_section_height = clamp_value(card_height * 0.28F, 180.0F, 320.0F);

    layout.footer = D2D1::RectF(inner_left, layout.card.bottom - 42.0F, inner_right, layout.card.bottom - 16.0F);
    layout.scroll_viewer = D2D1::RectF(inner_left, layout.footer.top - scroll_section_height, inner_right, layout.footer.top - 12.0F);
    layout.scroll_viewport = inset_rect(layout.scroll_viewer, 12.0F, 12.0F);
    layout.scroll_viewer_label = D2D1::RectF(inner_left, layout.scroll_viewer.top - label_height - 8.0F, inner_right, layout.scroll_viewer.top - 8.0F);

    const float upper_bottom = layout.scroll_viewer_label.top - 18.0F;
    const float right_column_height = max_value(240.0F, upper_bottom - top);
    const float fixed_right_spacing = (label_height + 6.0F) * 4.0F + control_gap * 4.0F;
    const float right_body_height = max_value(220.0F, right_column_height - fixed_right_spacing);
    const float text_block_height = clamp_value(right_body_height * 0.12F, 48.0F, 72.0F);
    const float image_height = clamp_value(right_body_height * 0.20F, 84.0F, 124.0F);
    const float preview_card_height = clamp_value(right_body_height * 0.28F, 116.0F, 176.0F);
    const float remaining_list_height = max_value(190.0F, right_body_height - text_block_height - image_height - preview_card_height);
    const float list_view_height = clamp_value(remaining_list_height * 0.54F, 96.0F, 188.0F);
    const float items_control_height = max_value(88.0F, remaining_list_height - list_view_height);

    layout.left_column = D2D1::RectF(inner_left, top, inner_left + left_width, upper_bottom);
    layout.right_column = D2D1::RectF(layout.left_column.right + column_gap, top, inner_right, upper_bottom);

    float left_cursor = layout.left_column.top;
    layout.text_box_label = D2D1::RectF(layout.left_column.left, left_cursor, layout.left_column.right, left_cursor + label_height);
    left_cursor += label_height + 6.0F;
    layout.text_box = D2D1::RectF(layout.left_column.left, left_cursor, layout.left_column.right, left_cursor + control_height);
    layout.text_box_text = inset_rect(layout.text_box, 14.0F, 9.0F);
    left_cursor += control_height + control_gap;

    layout.rich_text_label = D2D1::RectF(layout.left_column.left, left_cursor, layout.left_column.right, left_cursor + label_height);
    left_cursor += label_height + 6.0F;
    const float left_remaining_fixed = control_height + control_gap + (label_height + 6.0F) + control_height + control_gap + (label_height + 6.0F) + control_height;
    const float rich_text_height = max_value(108.0F, upper_bottom - left_cursor - left_remaining_fixed);
    layout.rich_text_box = D2D1::RectF(layout.left_column.left, left_cursor, layout.left_column.right, left_cursor + rich_text_height);
    layout.rich_text_viewport = inset_rect(layout.rich_text_box, 14.0F, 12.0F);
    left_cursor = layout.rich_text_box.bottom + control_gap;

    layout.check_box = D2D1::RectF(layout.left_column.left, left_cursor, layout.left_column.right, left_cursor + control_height);
    left_cursor += control_height + control_gap;

    layout.combo_label = D2D1::RectF(layout.left_column.left, left_cursor, layout.left_column.right, left_cursor + label_height);
    left_cursor += label_height + 6.0F;
    layout.combo_box = D2D1::RectF(layout.left_column.left, left_cursor, layout.left_column.right, left_cursor + control_height);
    if (controls.combo_box && controls.combo_box->is_dropdown_open()) {
        const float item_height = 36.0F;
        const std::size_t visible_count = min_value<std::size_t>(controls.combo_box->items().size(), 8U);
        layout.combo_dropdown = D2D1::RectF(
            layout.combo_box.left,
            layout.combo_box.bottom + 4.0F,
            layout.combo_box.right,
            layout.combo_box.bottom + 12.0F + item_height * static_cast<float>(visible_count));
        layout.combo_items.reserve(controls.combo_box->items().size());
        for (std::size_t index = 0; index < controls.combo_box->items().size(); ++index) {
            const float item_top = layout.combo_dropdown.top + 4.0F + item_height * static_cast<float>(index);
            layout.combo_items.push_back(D2D1::RectF(layout.combo_dropdown.left + 4.0F, item_top, layout.combo_dropdown.right - 4.0F, item_top + item_height));
        }
    }
    left_cursor += control_height + control_gap;

    layout.slider_label = D2D1::RectF(layout.left_column.left, left_cursor, layout.left_column.right, left_cursor + label_height);
    left_cursor += label_height + 6.0F;
    layout.slider = D2D1::RectF(layout.left_column.left, left_cursor, layout.left_column.right, left_cursor + control_height);
    layout.slider_track = D2D1::RectF(layout.slider.left + 150.0F, layout.slider.top + 19.0F, layout.slider.right - 18.0F, layout.slider.top + 25.0F);
    left_cursor += control_height + control_gap;

    float right_cursor = layout.right_column.top;
    layout.text_block_label = D2D1::RectF(layout.right_column.left, right_cursor, layout.right_column.right, right_cursor + label_height);
    right_cursor += label_height + 6.0F;
    layout.text_block = D2D1::RectF(layout.right_column.left, right_cursor, layout.right_column.right, right_cursor + text_block_height);
    right_cursor += text_block_height + control_gap;

    layout.image_label = D2D1::RectF(layout.right_column.left, right_cursor, layout.right_column.right, right_cursor + label_height);
    right_cursor += label_height + 6.0F;
    layout.image = D2D1::RectF(layout.right_column.left, right_cursor, layout.right_column.right, right_cursor + image_height);
    right_cursor += image_height + control_gap;

    layout.card_preview = D2D1::RectF(layout.right_column.left, right_cursor, layout.right_column.right, right_cursor + preview_card_height);
    right_cursor += preview_card_height + control_gap;

    layout.list_view_label = D2D1::RectF(layout.right_column.left, right_cursor, layout.right_column.right, right_cursor + label_height);
    right_cursor += label_height + 6.0F;
    layout.list_view = D2D1::RectF(layout.right_column.left, right_cursor, layout.right_column.right, right_cursor + list_view_height);
    layout.list_viewport = inset_rect(layout.list_view, 8.0F, 8.0F);
    right_cursor += list_view_height + control_gap;

    layout.items_control_label = D2D1::RectF(layout.right_column.left, right_cursor, layout.right_column.right, right_cursor + label_height);
    right_cursor += label_height + 6.0F;
    layout.items_control = D2D1::RectF(layout.right_column.left, right_cursor, layout.right_column.right, right_cursor + items_control_height);
    layout.items_control_viewport = inset_rect(layout.items_control, 8.0F, 8.0F);

    if (controls.list_view != nullptr) {
        const float total_height = compute_items_total_height(controls.list_view->items().size(), 28.0F, 0.0F);
        layout.list_view_thumb = compute_vertical_thumb(layout.list_viewport, total_height, controls.list_view->scroll_offset());
    }
    if (controls.items_control != nullptr) {
        const float total_height = compute_items_total_height(controls.items_control->items().size(), 28.0F, controls.items_control->item_spacing());
        layout.items_control_thumb = compute_vertical_thumb(layout.items_control_viewport, total_height, controls.items_control->scroll_offset());
    }
    if (controls.scroll_viewer != nullptr && controls.scroll_viewer->content() != nullptr) {
        const auto content = controls.scroll_viewer->content();
        const float total_height = compute_items_total_height(content->items().size(), 28.0F, content->item_spacing());
        layout.scroll_viewer_thumb = compute_vertical_thumb(layout.scroll_viewport, total_height, controls.scroll_viewer->scroll_offset().y);
    }

    return layout;
}

HitResult hit_test(const OverlayLayout& layout, float x, float y) {
    if (point_in_rect(layout.button, x, y)) {
        return {.target = HitTarget::PrimaryButton};
    }
    if (point_in_rect(layout.text_box, x, y)) {
        return {.target = HitTarget::TextBox};
    }
    if (point_in_rect(layout.rich_text_box, x, y)) {
        return {.target = HitTarget::RichTextBox};
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
    if (point_in_rect(layout.scroll_viewer_thumb, x, y)) {
        return {.target = HitTarget::ScrollViewerThumb};
    }
    if (point_in_rect(layout.list_view_thumb, x, y)) {
        return {.target = HitTarget::ListViewThumb};
    }
    if (point_in_rect(layout.items_control_thumb, x, y)) {
        return {.target = HitTarget::ItemsControlThumb};
    }
    if (point_in_rect(layout.slider, x, y)) {
        return {.target = HitTarget::Slider};
    }
    if (point_in_rect(layout.scroll_viewer, x, y)) {
        return {.target = HitTarget::ScrollViewer};
    }
    if (point_in_rect(layout.list_view, x, y)) {
        return {.target = HitTarget::ListView};
    }
    if (point_in_rect(layout.items_control, x, y)) {
        return {.target = HitTarget::ItemsControl};
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
    float y,
    float scroll_offset = 0.0F,
    float layout_height = 0.0F) {
    if (text.empty()) {
        return 0;
    }

    IDWriteTextLayout* layout = nullptr;
    const float target_height = layout_height > 0.0F ? layout_height : rect_height(text_rect);
    if (!create_text_layout(factory, format, text, rect_width(text_rect), target_height, &layout)) {
        return text.size();
    }

    BOOL is_trailing_hit = FALSE;
    BOOL is_inside = FALSE;
    DWRITE_HIT_TEST_METRICS metrics {};
    const HRESULT hr = layout->HitTestPoint(x - text_rect.left, y - text_rect.top + scroll_offset, &is_trailing_hit, &is_inside, &metrics);
    safe_release(layout);
    if (FAILED(hr)) {
        return text.size();
    }

    const std::size_t offset = static_cast<std::size_t>(metrics.textPosition);
    return min_value(text.size(), offset + (is_trailing_hit != FALSE ? 1U : 0U));
}

float measure_text_layout_height(
    IDWriteFactory* factory,
    IDWriteTextFormat* format,
    const std::wstring& text,
    float width) {
    if (factory == nullptr || format == nullptr || width <= 0.0F) {
        return 0.0F;
    }

    IDWriteTextLayout* layout = nullptr;
    if (!create_text_layout(factory, format, text, width, 100000.0F, &layout)) {
        return 0.0F;
    }

    DWRITE_TEXT_METRICS metrics {};
    const HRESULT hr = layout->GetMetrics(&metrics);
    safe_release(layout);
    return SUCCEEDED(hr) ? metrics.height : 0.0F;
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

void fill_rounded_rect(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float radius);
void stroke_rounded_rect(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float thickness, float radius);

void draw_text_line(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteTextFormat* format,
    const std::wstring& text,
    const D2D1_RECT_F& rect,
    const D2D1_COLOR_F& color,
    DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING) {
    if (context == nullptr || brush == nullptr || format == nullptr || text.empty()) {
        return;
    }

    const auto prev_align = format->GetTextAlignment();
    format->SetTextAlignment(alignment);
    brush->SetColor(color);
    context->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        rect,
        brush,
        D2D1_DRAW_TEXT_OPTIONS_CLIP);
    format->SetTextAlignment(prev_align);
}

void draw_wrapped_text(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteFactory* factory,
    IDWriteTextFormat* format,
    const std::wstring& text,
    const D2D1_RECT_F& rect,
    const D2D1_COLOR_F& color) {
    if (context == nullptr || brush == nullptr || factory == nullptr || format == nullptr || text.empty()) {
        return;
    }

    IDWriteTextLayout* layout = nullptr;
    if (!create_text_layout(factory, format, text, rect_width(rect), rect_height(rect), &layout)) {
        return;
    }

    brush->SetColor(color);
    context->DrawTextLayout(D2D1::Point2F(rect.left, rect.top), layout, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    safe_release(layout);
}

void draw_checkbox_glyph(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect, bool checked) {
    const float cx = rect.left + 28.0F; // Center of the checkbox
    const float cy = (rect.top + rect.bottom) / 2.0F;
    const D2D1_RECT_F box = D2D1::RectF(cx - 10.0F, cy - 10.0F, cx + 10.0F, cy + 10.0F);

    if (checked) {
        brush->SetColor(D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F));
        context->FillRoundedRectangle(D2D1::RoundedRect(box, 4.0F, 4.0F), brush);

        brush->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F));
        context->DrawLine(D2D1::Point2F(box.left + 5.0F, box.top + 10.0F), D2D1::Point2F(box.left + 9.0F, box.bottom - 6.0F), brush, 2.0F);
        context->DrawLine(D2D1::Point2F(box.left + 9.0F, box.bottom - 6.0F), D2D1::Point2F(box.right - 4.0F, box.top + 5.0F), brush, 2.0F);
    } else {
        brush->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F));
        context->FillRoundedRectangle(D2D1::RoundedRect(box, 4.0F, 4.0F), brush);
        brush->SetColor(D2D1::ColorF(0.86F, 0.88F, 0.91F, 1.0F));
        context->DrawRoundedRectangle(D2D1::RoundedRect(box, 4.0F, 4.0F), brush, 1.0F);
    }
}

void draw_combobox_glyph(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect) {
    const float center_x = rect.right - 28.0F;
    const float center_y = (rect.top + rect.bottom) * 0.5F;
    brush->SetColor(D2D1::ColorF(0.57F, 0.60F, 0.65F, 1.0F));
    context->DrawLine(D2D1::Point2F(center_x - 6.0F, center_y - 3.0F), D2D1::Point2F(center_x, center_y + 3.0F), brush, 2.0F);
    context->DrawLine(D2D1::Point2F(center_x, center_y + 3.0F), D2D1::Point2F(center_x + 6.0F, center_y - 3.0F), brush, 2.0F);
}

void draw_slider_glyph(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect, float value) {
    const float track_left = rect.left + 18.0F;
    const float track_right = rect.right - 18.0F;
    const float center_y = (rect.top + rect.bottom) * 0.5F;
    const float thumb_x = track_left + (track_right - track_left) * clamp_value(value, 0.0F, 1.0F);

    brush->SetColor(D2D1::ColorF(0.89F, 0.91F, 0.94F, 1.0F));
    context->DrawLine(D2D1::Point2F(track_left, center_y), D2D1::Point2F(track_right, center_y), brush, 6.0F);
    brush->SetColor(D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F));
    context->DrawLine(D2D1::Point2F(track_left, center_y), D2D1::Point2F(thumb_x, center_y), brush, 6.0F);
    brush->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F));
    context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(thumb_x, center_y), 8.0F, 8.0F), brush);
}

void draw_scrollviewer_glyph(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect) {
    const D2D1_RECT_F viewport = D2D1::RectF(rect.left + 16.0F, rect.top + 10.0F, rect.right - 16.0F, rect.bottom - 10.0F);
    brush->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F));
    context->FillRoundedRectangle(D2D1::RoundedRect(viewport, 4.0F, 4.0F), brush);
    brush->SetColor(D2D1::ColorF(0.86F, 0.88F, 0.91F, 1.0F));
    context->DrawRoundedRectangle(D2D1::RoundedRect(viewport, 4.0F, 4.0F), brush, 1.0F);

    const D2D1_RECT_F thumb = D2D1::RectF(viewport.right - 8.0F, viewport.top + 6.0F, viewport.right - 4.0F, viewport.top + 22.0F);
    brush->SetColor(D2D1::ColorF(0.75F, 0.77F, 0.80F, 1.0F));
    context->FillRoundedRectangle(D2D1::RoundedRect(thumb, 4.0F, 4.0F), brush);
}

void draw_image_placeholder(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteTextFormat* format,
    const D2D1_RECT_F& rect) {
    fill_rounded_rect(context, brush, rect, D2D1::ColorF(0.98F, 0.99F, 1.0F, 1.0F), 6.0F);
    stroke_rounded_rect(context, brush, rect, D2D1::ColorF(0.90F, 0.92F, 0.95F, 1.0F), 1.0F, 6.0F);

    const D2D1_RECT_F image_box = D2D1::RectF(rect.left + 16.0F, rect.top + 14.0F, rect.left + 92.0F, rect.bottom - 14.0F);
    brush->SetColor(D2D1::ColorF(0.88F, 0.94F, 1.0F, 1.0F));
    context->FillRoundedRectangle(D2D1::RoundedRect(image_box, 6.0F, 6.0F), brush);
    brush->SetColor(D2D1::ColorF(0.75F, 0.84F, 0.95F, 1.0F));
    context->DrawLine(D2D1::Point2F(image_box.left + 10.0F, image_box.bottom - 14.0F), D2D1::Point2F(image_box.left + 28.0F, image_box.top + 28.0F), brush, 2.0F);
    context->DrawLine(D2D1::Point2F(image_box.left + 28.0F, image_box.top + 28.0F), D2D1::Point2F(image_box.left + 44.0F, image_box.bottom - 24.0F), brush, 2.0F);
    context->DrawLine(D2D1::Point2F(image_box.left + 44.0F, image_box.bottom - 24.0F), D2D1::Point2F(image_box.right - 10.0F, image_box.top + 20.0F), brush, 2.0F);
    context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(image_box.right - 18.0F, image_box.top + 18.0F), 5.0F, 5.0F), brush);

    draw_text_line(context, brush, format, L"Image 控件示例", D2D1::RectF(image_box.right + 16.0F, rect.top + 16.0F, rect.right - 16.0F, rect.top + 38.0F), D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
    draw_text_line(context, brush, format, L"Element Plus 风格图片区块", D2D1::RectF(image_box.right + 16.0F, rect.top + 42.0F, rect.right - 16.0F, rect.top + 64.0F), D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));
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

    brush->SetColor(D2D1::ColorF(0.38F, 0.40F, 0.43F, 1.0F));
    context->DrawText(label.c_str(), static_cast<UINT32>(label.size()), format, D2D1::RectF(rect.left + label_offset, rect.top, rect.left + 170.0F + (label_offset - 18.0F), rect.bottom), brush);
    
    const auto prev_align = format->GetTextAlignment();
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    brush->SetColor(D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
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
    fill_rounded_rect(context, brush, rect, D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), 4.0F);
    stroke_rounded_rect(
        context,
        brush,
        rect,
        focused ? D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F) : (hovered ? D2D1::ColorF(0.75F, 0.77F, 0.80F, 1.0F) : D2D1::ColorF(0.86F, 0.88F, 0.91F, 1.0F)),
        focused ? 2.0F : 1.0F,
        4.0F);

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

    brush->SetColor(use_placeholder ? D2D1::ColorF(0.64F, 0.67F, 0.72F, 1.0F) : D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
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

void draw_rich_text_panel(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteFactory* factory,
    IDWriteTextFormat* format,
    const RichTextBox& rich_text_box,
    const D2D1_RECT_F& rect,
    bool hovered,
    bool focused,
    bool caret_visible) {
    fill_rounded_rect(context, brush, rect, D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), 4.0F);
    stroke_rounded_rect(
        context,
        brush,
        rect,
        focused ? D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F) : (hovered ? D2D1::ColorF(0.75F, 0.77F, 0.80F, 1.0F) : D2D1::ColorF(0.86F, 0.88F, 0.91F, 1.0F)),
        focused ? 2.0F : 1.0F,
        4.0F);

    const D2D1_RECT_F text_rect = inset_rect(rect, 14.0F, 12.0F);
    const std::wstring wide_text = utf8_to_wstring(rich_text_box.rich_text());
    const float content_height = max_value(rect_height(text_rect) + 8.0F, measure_text_layout_height(factory, format, wide_text, rect_width(text_rect)) + 8.0F);
    IDWriteTextLayout* layout = nullptr;
    if (!create_text_layout(factory, format, wide_text, rect_width(text_rect), content_height, &layout)) {
        return;
    }

    context->PushAxisAlignedClip(text_rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    if (rich_text_box.has_selection()) {
        const auto [selection_start, selection_end] = rich_text_box.selection();
        UINT32 actual_count = 0;
        layout->HitTestTextRange(
            static_cast<UINT32>(selection_start),
            static_cast<UINT32>(selection_end - selection_start),
            text_rect.left,
            text_rect.top - rich_text_box.scroll_offset(),
            nullptr,
            0,
            &actual_count);
        if (actual_count > 0) {
            std::vector<DWRITE_HIT_TEST_METRICS> metrics(actual_count);
            if (SUCCEEDED(layout->HitTestTextRange(
                    static_cast<UINT32>(selection_start),
                    static_cast<UINT32>(selection_end - selection_start),
                    text_rect.left,
                    text_rect.top - rich_text_box.scroll_offset(),
                    metrics.data(),
                    actual_count,
                    &actual_count))) {
                brush->SetColor(D2D1::ColorF(0.12F, 0.62F, 0.95F, 0.22F));
                for (UINT32 index = 0; index < actual_count; ++index) {
                    const auto& metric = metrics[index];
                    context->FillRectangle(D2D1::RectF(metric.left, metric.top, metric.left + metric.width, metric.top + metric.height), brush);
                }
            }
        }
    }

    brush->SetColor(D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
    context->DrawTextLayout(D2D1::Point2F(text_rect.left, text_rect.top - rich_text_box.scroll_offset()), layout, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

    if (focused && caret_visible) {
        FLOAT caret_x = text_rect.left;
        FLOAT caret_y = text_rect.top;
        DWRITE_HIT_TEST_METRICS caret_metrics {};
        if (SUCCEEDED(layout->HitTestTextPosition(static_cast<UINT32>(rich_text_box.caret_position()), FALSE, &caret_x, &caret_y, &caret_metrics))) {
            brush->SetColor(D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F));
            context->DrawLine(
                D2D1::Point2F(text_rect.left + caret_x, text_rect.top - rich_text_box.scroll_offset() + caret_y),
                D2D1::Point2F(text_rect.left + caret_x, text_rect.top - rich_text_box.scroll_offset() + caret_y + max_value(18.0F, caret_metrics.height)),
                brush,
                1.5F);
        }
    }

    context->PopAxisAlignedClip();
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
        rich_text_box_hovered_ = hovered.target == HitTarget::RichTextBox;
        check_box_hovered_ = hovered.target == HitTarget::CheckBox;
        combo_box_hovered_ = hovered.target == HitTarget::ComboBoxHeader || hovered.target == HitTarget::ComboBoxItem;
        slider_hovered_ = hovered.target == HitTarget::Slider;
        scroll_viewer_hovered_ = hovered.target == HitTarget::ScrollViewer || hovered.target == HitTarget::ScrollViewerThumb;
        list_view_hovered_ = hovered.target == HitTarget::ListView || hovered.target == HitTarget::ListViewThumb;
        items_control_hovered_ = hovered.target == HitTarget::ItemsControl || hovered.target == HitTarget::ItemsControlThumb;
        hovered_combo_index_ = hovered.target == HitTarget::ComboBoxItem ? hovered.combo_index : -1;
        hovered_item_index_ = hovered_combo_index_;

        if (d2d_context_ == nullptr || d2d_target_bitmap_ == nullptr || d2d_brush_ == nullptr) {
            render_manager_->diagnostics().log(LogLevel::Error, "D2D overlay context unavailable");
            return false;
        }

            d2d_context_->BeginDraw();
            d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());

            d2d_brush_->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F));
            const D2D1_RECT_F card_rect = interactive_mode_enabled_
                ? layout.card
                : D2D1::RectF(width * 0.1F, height * 0.1F, width * 0.9F, height * 0.65F);
            const D2D1_ROUNDED_RECT card = D2D1::RoundedRect(card_rect, 4.0F, 4.0F);
            d2d_context_->FillRoundedRectangle(card, d2d_brush_);

            d2d_brush_->SetColor(D2D1::ColorF(0.89F, 0.91F, 0.94F, 1.0F));
            d2d_context_->DrawRoundedRectangle(card, d2d_brush_, 1.0F);

            d2d_brush_->SetColor(D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
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
                if (helper_text_format_ != nullptr) {
                    draw_wrapped_text(
                        d2d_context_,
                        d2d_brush_,
                        dwrite_factory_,
                        helper_text_format_,
                        L"保持 Grid / StackPanel / Panel 的 WPF 容器语义，其他控件统一收敛到 Element Plus 的白底、细边框、蓝色 focus ring 和浮层下拉交互。",
                        layout.subtitle,
                        D2D1::ColorF(0.38F, 0.40F, 0.43F, 1.0F));
                }

                D2D1_COLOR_F button_color = D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F);
                if (button_pressed_) {
                    button_color = D2D1::ColorF(0.20F, 0.49F, 0.80F, 1.0F);
                } else if (button_hovered_) {
                    button_color = D2D1::ColorF(0.47F, 0.73F, 1.0F, 1.0F);
                }
                fill_rounded_rect(d2d_context_, d2d_brush_, layout.button, button_color, 4.0F);
                if (focused_control_index_ == kPrimaryButtonFocusIndex) {
                    stroke_rounded_rect(d2d_context_, d2d_brush_, inset_rect(layout.button, -2.0F, -2.0F), D2D1::ColorF(0.75F, 0.84F, 1.0F, 1.0F), 2.0F, 4.0F);
                }
                if (item_text_format_ != nullptr) {
                    d2d_brush_->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F));
                    draw_centered_text(
                        d2d_context_,
                        d2d_brush_,
                        item_text_format_,
                        utf8_to_wstring(interactive_controls_.primary_button->text()),
                        layout.button);
                }

                draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"TextBox", layout.text_box_label, D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));

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

                if (interactive_controls_.rich_text_box != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"RichTextBox", layout.rich_text_label, D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));
                    draw_rich_text_panel(
                        d2d_context_,
                        d2d_brush_,
                        dwrite_factory_,
                        helper_text_format_ != nullptr ? helper_text_format_ : item_text_format_,
                        *interactive_controls_.rich_text_box,
                        layout.rich_text_box,
                        rich_text_box_hovered_,
                        focused_control_index_ == kRichTextBoxFocusIndex,
                        ((GetTickCount64() - caret_blink_seed_) / 500ULL) % 2ULL == 0ULL);
                }

                fill_rounded_rect(d2d_context_, d2d_brush_, layout.check_box, D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), 4.0F);
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.check_box, focused_control_index_ == kCheckBoxFocusIndex ? D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F) : (check_box_hovered_ ? D2D1::ColorF(0.75F, 0.77F, 0.80F, 1.0F) : D2D1::ColorF(0.86F, 0.88F, 0.91F, 1.0F)), focused_control_index_ == kCheckBoxFocusIndex ? 2.0F : 1.0F, 4.0F);
                draw_checkbox_glyph(d2d_context_, d2d_brush_, layout.check_box, interactive_controls_.check_box->checked());
                draw_text_line(
                    d2d_context_,
                    d2d_brush_,
                    item_text_format_,
                    interactive_controls_.check_box->checked() ? L"启用 Element Plus 表单增强行为" : L"关闭 Element Plus 表单增强行为",
                    D2D1::RectF(layout.check_box.left + 50.0F, layout.check_box.top, layout.check_box.right - 16.0F, layout.check_box.bottom),
                    D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));

                draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"ComboBox", layout.combo_label, D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));
                fill_rounded_rect(d2d_context_, d2d_brush_, layout.combo_box, D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), 4.0F);
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.combo_box, focused_control_index_ == kComboBoxFocusIndex ? D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F) : (combo_box_hovered_ ? D2D1::ColorF(0.75F, 0.77F, 0.80F, 1.0F) : D2D1::ColorF(0.86F, 0.88F, 0.91F, 1.0F)), focused_control_index_ == kComboBoxFocusIndex ? 2.0F : 1.0F, 4.0F);
                draw_combobox_glyph(d2d_context_, d2d_brush_, layout.combo_box);
                draw_text_line(
                    d2d_context_,
                    d2d_brush_,
                    item_text_format_,
                    utf8_to_wstring(interactive_controls_.combo_box->selected_text()),
                    D2D1::RectF(layout.combo_box.left + 14.0F, layout.combo_box.top, layout.combo_box.right - 42.0F, layout.combo_box.bottom),
                    D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));

                if (interactive_controls_.combo_box->is_dropdown_open()) {
                    fill_rounded_rect(d2d_context_, d2d_brush_, layout.combo_dropdown, D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), 4.0F);
                    stroke_rounded_rect(d2d_context_, d2d_brush_, layout.combo_dropdown, D2D1::ColorF(0.86F, 0.88F, 0.91F, 1.0F), 1.0F, 4.0F);
                    for (std::size_t index = 0; index < layout.combo_items.size(); ++index) {
                        const bool item_hovered = hovered_combo_index_ == static_cast<int>(index);
                        const bool selected = interactive_controls_.combo_box->selected_index() && *interactive_controls_.combo_box->selected_index() == index;
                        fill_rounded_rect(
                            d2d_context_,
                            d2d_brush_,
                            inset_rect(layout.combo_items[index], 4.0F, 4.0F),
                            item_hovered ? D2D1::ColorF(0.93F, 0.96F, 1.0F, 1.0F) : (selected ? D2D1::ColorF(0.90F, 0.95F, 1.0F, 1.0F) : D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F)),
                            4.0F);
                        draw_text_line(
                            d2d_context_,
                            d2d_brush_,
                            item_text_format_,
                            utf8_to_wstring(interactive_controls_.combo_box->items()[index]),
                            D2D1::RectF(layout.combo_items[index].left + 8.0F, layout.combo_items[index].top, layout.combo_items[index].right - 8.0F, layout.combo_items[index].bottom),
                            D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
                    }
                }

                draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"Slider", layout.slider_label, D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));
                fill_rounded_rect(d2d_context_, d2d_brush_, layout.slider, D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), 4.0F);
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.slider, focused_control_index_ == kSliderFocusIndex ? D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F) : (slider_hovered_ ? D2D1::ColorF(0.75F, 0.77F, 0.80F, 1.0F) : D2D1::ColorF(0.86F, 0.88F, 0.91F, 1.0F)), focused_control_index_ == kSliderFocusIndex ? 2.0F : 1.0F, 4.0F);
                draw_slider_glyph(d2d_context_, d2d_brush_, layout.slider_track, interactive_controls_.slider->normalized_value());
                draw_text_line(
                    d2d_context_,
                    d2d_brush_,
                    item_text_format_,
                    utf8_to_wstring(std::to_string(static_cast<int>(interactive_controls_.slider->value())) + "%"),
                    D2D1::RectF(layout.slider.left + 12.0F, layout.slider.top, layout.slider.left + 90.0F, layout.slider.bottom),
                    D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));

                if (interactive_controls_.text_block != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"TextBlock", layout.text_block_label, D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));
                    fill_rounded_rect(d2d_context_, d2d_brush_, layout.text_block, D2D1::ColorF(0.99F, 0.99F, 1.0F, 1.0F), 4.0F);
                    stroke_rounded_rect(d2d_context_, d2d_brush_, layout.text_block, D2D1::ColorF(0.91F, 0.92F, 0.94F, 1.0F), 1.0F, 4.0F);
                    draw_wrapped_text(
                        d2d_context_,
                        d2d_brush_,
                        dwrite_factory_,
                        helper_text_format_ != nullptr ? helper_text_format_ : item_text_format_,
                        utf8_to_wstring(interactive_controls_.text_block->text()),
                        inset_rect(layout.text_block, 12.0F, 10.0F),
                        D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
                }

                draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"Image", layout.image_label, D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));
                draw_image_placeholder(d2d_context_, d2d_brush_, item_text_format_, layout.image);

                if (interactive_controls_.card != nullptr) {
                    fill_rounded_rect(d2d_context_, d2d_brush_, layout.card_preview, D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), 8.0F);
                    stroke_rounded_rect(d2d_context_, d2d_brush_, layout.card_preview, D2D1::ColorF(0.89F, 0.92F, 0.96F, 1.0F), 1.0F, 8.0F);
                    const D2D1_RECT_F card_banner = D2D1::RectF(layout.card_preview.left, layout.card_preview.top, layout.card_preview.right, layout.card_preview.top + 30.0F);
                    fill_rounded_rect(d2d_context_, d2d_brush_, card_banner, D2D1::ColorF(0.95F, 0.97F, 1.0F, 1.0F), 8.0F);
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, utf8_to_wstring(interactive_controls_.card->title()), D2D1::RectF(layout.card_preview.left + 14.0F, layout.card_preview.top + 8.0F, layout.card_preview.right - 14.0F, layout.card_preview.top + 28.0F), D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
                    draw_wrapped_text(d2d_context_, d2d_brush_, dwrite_factory_, helper_text_format_ != nullptr ? helper_text_format_ : item_text_format_, utf8_to_wstring(interactive_controls_.card->body()), D2D1::RectF(layout.card_preview.left + 14.0F, layout.card_preview.top + 40.0F, layout.card_preview.right - 14.0F, layout.card_preview.top + 96.0F), D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));
                    float tag_left = layout.card_preview.left + 14.0F;
                    for (const auto& tag : interactive_controls_.card->tags()) {
                        const float tag_width = 54.0F + static_cast<float>(tag.size()) * 4.0F;
                        const D2D1_RECT_F tag_rect = D2D1::RectF(tag_left, layout.card_preview.bottom - 32.0F, min_value(layout.card_preview.right - 14.0F, tag_left + tag_width), layout.card_preview.bottom - 10.0F);
                        fill_rounded_rect(d2d_context_, d2d_brush_, tag_rect, D2D1::ColorF(0.93F, 0.96F, 1.0F, 1.0F), 11.0F);
                        draw_text_line(d2d_context_, d2d_brush_, item_text_format_, utf8_to_wstring(tag), tag_rect, D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F), DWRITE_TEXT_ALIGNMENT_CENTER);
                        tag_left = tag_rect.right + 8.0F;
                        if (tag_left >= layout.card_preview.right - 60.0F) {
                            break;
                        }
                    }
                }

                if (interactive_controls_.list_view != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"ListView", layout.list_view_label, D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));
                    fill_rounded_rect(d2d_context_, d2d_brush_, layout.list_view, D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), 4.0F);
                    stroke_rounded_rect(d2d_context_, d2d_brush_, layout.list_view, list_view_hovered_ ? D2D1::ColorF(0.78F, 0.82F, 0.89F, 1.0F) : D2D1::ColorF(0.90F, 0.92F, 0.95F, 1.0F), 1.0F, 4.0F);
                    const float item_height = 28.0F;
                    const float scroll_offset = interactive_controls_.list_view->scroll_offset();
                    const auto visible = interactive_controls_.list_view->visible_range(scroll_offset, rect_height(layout.list_viewport), item_height);
                    const float content_right = rect_has_area(layout.list_view_thumb) ? layout.list_view_thumb.left - 6.0F : layout.list_viewport.right;
                    d2d_context_->PushAxisAlignedClip(layout.list_viewport, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                    for (std::size_t index = visible.first; index < visible.second; ++index) {
                        const float item_top = layout.list_viewport.top + static_cast<float>(index) * item_height - scroll_offset;
                        const D2D1_RECT_F item_rect = D2D1::RectF(layout.list_viewport.left, item_top, content_right, item_top + item_height - 3.0F);
                        const bool selected = interactive_controls_.list_view->selected_index() && *interactive_controls_.list_view->selected_index() == index;
                        if (selected) {
                            fill_rounded_rect(d2d_context_, d2d_brush_, item_rect, D2D1::ColorF(0.93F, 0.96F, 1.0F, 1.0F), 4.0F);
                        }
                        draw_text_line(d2d_context_, d2d_brush_, item_text_format_, utf8_to_wstring(interactive_controls_.list_view->items()[index]), D2D1::RectF(item_rect.left + 8.0F, item_rect.top, item_rect.right - 8.0F, item_rect.bottom), selected ? D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F) : D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
                    }
                    d2d_context_->PopAxisAlignedClip();
                    if (rect_has_area(layout.list_view_thumb)) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.list_view_thumb, list_view_hovered_ ? D2D1::ColorF(0.62F, 0.66F, 0.74F, 1.0F) : D2D1::ColorF(0.78F, 0.80F, 0.84F, 1.0F), 3.0F);
                    }
                }

                if (interactive_controls_.items_control != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"ItemsControl", layout.items_control_label, D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));
                    fill_rounded_rect(d2d_context_, d2d_brush_, layout.items_control, D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), 4.0F);
                    stroke_rounded_rect(d2d_context_, d2d_brush_, layout.items_control, items_control_hovered_ ? D2D1::ColorF(0.78F, 0.82F, 0.89F, 1.0F) : D2D1::ColorF(0.90F, 0.92F, 0.95F, 1.0F), 1.0F, 4.0F);
                    const float item_height = 28.0F;
                    const float stride = item_height + interactive_controls_.items_control->item_spacing();
                    const float scroll_offset = interactive_controls_.items_control->scroll_offset();
                    const auto visible = interactive_controls_.items_control->visible_range(scroll_offset, rect_height(layout.items_control_viewport), item_height);
                    const float content_right = rect_has_area(layout.items_control_thumb) ? layout.items_control_thumb.left - 6.0F : layout.items_control_viewport.right;
                    d2d_context_->PushAxisAlignedClip(layout.items_control_viewport, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                    for (std::size_t index = visible.first; index < visible.second; ++index) {
                        const auto& item = interactive_controls_.items_control->items()[index];
                        const float item_top = layout.items_control_viewport.top + static_cast<float>(index) * stride - scroll_offset;
                        const D2D1_RECT_F tag_rect = D2D1::RectF(layout.items_control_viewport.left, item_top, content_right, item_top + item_height);
                        const bool selected = interactive_controls_.items_control->selected_index() && *interactive_controls_.items_control->selected_index() == index;
                        fill_rounded_rect(d2d_context_, d2d_brush_, tag_rect, selected ? D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F) : D2D1::ColorF(0.95F, 0.96F, 0.98F, 1.0F), 6.0F);
                        draw_text_line(d2d_context_, d2d_brush_, item_text_format_, utf8_to_wstring(item), D2D1::RectF(tag_rect.left + 10.0F, tag_rect.top, tag_rect.right - 10.0F, tag_rect.bottom), selected ? D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F) : D2D1::ColorF(0.38F, 0.40F, 0.43F, 1.0F));
                    }
                    d2d_context_->PopAxisAlignedClip();
                    if (rect_has_area(layout.items_control_thumb)) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.items_control_thumb, items_control_hovered_ ? D2D1::ColorF(0.62F, 0.66F, 0.74F, 1.0F) : D2D1::ColorF(0.78F, 0.80F, 0.84F, 1.0F), 3.0F);
                    }
                }

                if (interactive_controls_.scroll_viewer != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"ScrollViewer", layout.scroll_viewer_label, D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));
                    fill_rounded_rect(d2d_context_, d2d_brush_, layout.scroll_viewer, D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), 4.0F);
                    stroke_rounded_rect(d2d_context_, d2d_brush_, layout.scroll_viewer, scroll_viewer_hovered_ ? D2D1::ColorF(0.78F, 0.82F, 0.89F, 1.0F) : D2D1::ColorF(0.86F, 0.88F, 0.91F, 1.0F), 1.0F, 4.0F);

                    d2d_context_->PushAxisAlignedClip(layout.scroll_viewport, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                    if (auto content = interactive_controls_.scroll_viewer->content()) {
                        const float item_height = 28.0F;
                        const float stride = item_height + content->item_spacing();
                        const float scroll_offset = interactive_controls_.scroll_viewer->scroll_offset().y;
                        const auto visible = content->visible_range(scroll_offset, rect_height(layout.scroll_viewport), item_height);
                        const float content_right = rect_has_area(layout.scroll_viewer_thumb) ? layout.scroll_viewer_thumb.left - 8.0F : layout.scroll_viewport.right - 4.0F;
                        for (std::size_t index = visible.first; index < visible.second; ++index) {
                            const float item_top = layout.scroll_viewport.top + static_cast<float>(index) * stride - scroll_offset;
                            const D2D1_RECT_F item_rect = D2D1::RectF(layout.scroll_viewport.left + 4.0F, item_top, content_right, item_top + item_height);
                            const bool selected = content->selected_index() && *content->selected_index() == index;
                            fill_rounded_rect(d2d_context_, d2d_brush_, item_rect, selected ? D2D1::ColorF(0.93F, 0.96F, 1.0F, 1.0F) : D2D1::ColorF(0.99F, 0.99F, 1.0F, 1.0F), 4.0F);
                            stroke_rounded_rect(d2d_context_, d2d_brush_, item_rect, D2D1::ColorF(0.94F, 0.95F, 0.97F, 1.0F), 1.0F, 4.0F);
                            draw_text_line(d2d_context_, d2d_brush_, item_text_format_, utf8_to_wstring(content->items()[index]), D2D1::RectF(item_rect.left + 10.0F, item_rect.top, item_rect.right - 10.0F, item_rect.bottom), D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
                        }
                    }
                    d2d_context_->PopAxisAlignedClip();
                    if (rect_has_area(layout.scroll_viewer_thumb)) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.scroll_viewer_thumb, scroll_viewer_hovered_ ? D2D1::ColorF(0.62F, 0.66F, 0.74F, 1.0F) : D2D1::ColorF(0.75F, 0.77F, 0.80F, 1.0F), 3.0F);
                    }
                }

                fill_rounded_rect(d2d_context_, d2d_brush_, layout.footer, D2D1::ColorF(0.98F, 0.98F, 0.99F, 1.0F), 4.0F);
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.footer, D2D1::ColorF(0.91F, 0.92F, 0.94F, 1.0F), 1.0F, 4.0F);
                const std::wstring footer = L"TextBox: " + utf8_to_wstring(interactive_controls_.text_box->text())
                    + L" | Combo: " + utf8_to_wstring(interactive_controls_.combo_box->selected_text())
                    + L" | Slider: " + utf8_to_wstring(std::to_string(static_cast<int>(interactive_controls_.slider->value())))
                    + L" | New Window: " + std::to_wstring(button_click_count_);
                draw_text_line(d2d_context_, d2d_brush_, item_text_format_, footer, layout.footer, D2D1::ColorF(0.38F, 0.40F, 0.43F, 1.0F), DWRITE_TEXT_ALIGNMENT_CENTER);
            } else {
                for (std::size_t i = 0; i < items.size(); ++i) {
                    const float top = height * 0.25F + static_cast<float>(i) * 52.0F;
                    const D2D1_RECT_F control_rect = D2D1::RectF(width * 0.14F, top, width * 0.70F, top + 40.0F);
                    const D2D1_ROUNDED_RECT control = D2D1::RoundedRect(control_rect, 4.0F, 4.0F);
                    const bool hovered_item = static_cast<int>(i) == hovered_item_index_;
                    d2d_brush_->SetColor(
                        hovered_item
                            ? D2D1::ColorF(0.93F, 0.96F, 1.0F, 1.0F)
                            : (i == 0 ? D2D1::ColorF(0.96F, 0.98F, 1.0F, 1.0F) : D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F)));
                    d2d_context_->FillRoundedRectangle(control, d2d_brush_);
                    d2d_brush_->SetColor(D2D1::ColorF(0.86F, 0.88F, 0.91F, 1.0F));
                    d2d_context_->DrawRoundedRectangle(control, d2d_brush_, 1.0F);

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
                        d2d_brush_->SetColor(D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
                        draw_centered_text(d2d_context_, d2d_brush_, item_text_format_, items[i], D2D1::RectF(width * 0.16F, top + 4.0F, width * 0.68F, top + 36.0F));
                    }
                }

                const D2D1_RECT_F action_rect = D2D1::RectF(width * 0.73F, height * 0.50F, width * 0.86F, height * 0.58F);
                fill_rounded_rect(d2d_context_, d2d_brush_, action_rect, button_hovered_ ? D2D1::ColorF(0.47F, 0.73F, 1.0F, 1.0F) : D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F), 4.0F);
                if (item_text_format_ != nullptr) {
                    d2d_brush_->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F));
                    draw_centered_text(d2d_context_, d2d_brush_, item_text_format_, L"Start", action_rect);
                }
            }

            d2d_context_->PopAxisAlignedClip();

            if (interactive_mode_enabled_ && interactive_controls_.combo_box != nullptr && interactive_controls_.combo_box->is_dropdown_open()) {
                fill_rounded_rect(d2d_context_, d2d_brush_, layout.combo_dropdown, D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), 6.0F);
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.combo_dropdown, D2D1::ColorF(0.82F, 0.87F, 0.95F, 1.0F), 1.0F, 6.0F);
                for (std::size_t index = 0; index < layout.combo_items.size(); ++index) {
                    const bool item_hovered = hovered_combo_index_ == static_cast<int>(index);
                    const bool selected = interactive_controls_.combo_box->selected_index() && *interactive_controls_.combo_box->selected_index() == index;
                    const D2D1_RECT_F item_rect = inset_rect(layout.combo_items[index], 4.0F, 3.0F);
                    fill_rounded_rect(
                        d2d_context_,
                        d2d_brush_,
                        item_rect,
                        selected ? D2D1::ColorF(0.25F, 0.62F, 1.0F, 0.12F) : (item_hovered ? D2D1::ColorF(0.94F, 0.97F, 1.0F, 1.0F) : D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F)),
                        4.0F);
                    draw_text_line(
                        d2d_context_,
                        d2d_brush_,
                        item_text_format_,
                        utf8_to_wstring(interactive_controls_.combo_box->items()[index]),
                        D2D1::RectF(item_rect.left + 10.0F, item_rect.top, item_rect.right - 28.0F, item_rect.bottom),
                        selected ? D2D1::ColorF(0.21F, 0.53F, 0.93F, 1.0F) : D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
                }
            }

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
    const auto clamp_scroll_offset = [&](float candidate) {
        if (interactive_controls_.scroll_viewer == nullptr || interactive_controls_.scroll_viewer->content() == nullptr) {
            return 0.0F;
        }

        const auto content = interactive_controls_.scroll_viewer->content();
        const float total_height = compute_items_total_height(content->items().size(), 28.0F, content->item_spacing());
        return clamp_value(candidate, 0.0F, max_value(0.0F, total_height - rect_height(layout.scroll_viewport)));
    };
    const auto rich_text_content_height = [&]() {
        if (interactive_controls_.rich_text_box == nullptr) {
            return 0.0F;
        }

        return max_value(
            rect_height(layout.rich_text_viewport) + 8.0F,
            measure_text_layout_height(
                dwrite_factory_,
                helper_text_format_ != nullptr ? helper_text_format_ : input_text_format_,
                utf8_to_wstring(interactive_controls_.rich_text_box->rich_text()),
                rect_width(layout.rich_text_viewport))
                + 8.0F);
    };
    const auto clamp_rich_text_offset = [&](float candidate) {
        const float max_offset = max_value(0.0F, rich_text_content_height() - rect_height(layout.rich_text_viewport));
        return clamp_value(candidate, 0.0F, max_offset);
    };
    const auto update_text_box_selection = [&](float x, float y, bool extend) {
        const std::wstring wide_text = utf8_to_wstring(interactive_controls_.text_box->text());
        const float clamped_x = clamp_value(x, layout.text_box_text.left, max_value(layout.text_box_text.left, layout.text_box_text.right - 1.0F));
        const float clamped_y = clamp_value(y, layout.text_box_text.top, max_value(layout.text_box_text.top, layout.text_box_text.bottom - 1.0F));
        const std::size_t position = text_box_hit_index(dwrite_factory_, input_text_format_, wide_text, layout.text_box_text, clamped_x, clamped_y);
        interactive_controls_.text_box->set_caret_position(position, extend);
        reset_caret_blink();
        window_host_->request_render();
    };
    const auto update_rich_text_selection = [&](float x, float y, bool extend) {
        if (interactive_controls_.rich_text_box == nullptr) {
            return;
        }
        const std::wstring wide_text = utf8_to_wstring(interactive_controls_.rich_text_box->rich_text());
        if (y < layout.rich_text_viewport.top) {
            interactive_controls_.rich_text_box->set_scroll_offset(clamp_rich_text_offset(interactive_controls_.rich_text_box->scroll_offset() - 24.0F));
        } else if (y > layout.rich_text_viewport.bottom) {
            interactive_controls_.rich_text_box->set_scroll_offset(clamp_rich_text_offset(interactive_controls_.rich_text_box->scroll_offset() + 24.0F));
        }

        const float clamped_x = clamp_value(x, layout.rich_text_viewport.left, max_value(layout.rich_text_viewport.left, layout.rich_text_viewport.right - 1.0F));
        const float clamped_y = clamp_value(y, layout.rich_text_viewport.top, max_value(layout.rich_text_viewport.top, layout.rich_text_viewport.bottom - 1.0F));
        const std::size_t position = text_box_hit_index(
            dwrite_factory_,
            helper_text_format_ != nullptr ? helper_text_format_ : input_text_format_,
            wide_text,
            layout.rich_text_viewport,
            clamped_x,
            clamped_y,
            interactive_controls_.rich_text_box->scroll_offset(),
            rich_text_content_height());
        interactive_controls_.rich_text_box->set_caret_position(position, extend);
        reset_caret_blink();
        window_host_->request_render();
    };
    const auto ensure_rich_text_caret_visible = [&]() {
        if (interactive_controls_.rich_text_box == nullptr) {
            return;
        }

        const std::wstring text = utf8_to_wstring(interactive_controls_.rich_text_box->rich_text());
        IDWriteTextLayout* text_layout = nullptr;
        const float content_height = rich_text_content_height();
        if (!create_text_layout(
                dwrite_factory_,
                helper_text_format_ != nullptr ? helper_text_format_ : input_text_format_,
                text,
                rect_width(layout.rich_text_viewport),
                content_height,
                &text_layout)) {
            return;
        }

        FLOAT caret_x = 0.0F;
        FLOAT caret_y = 0.0F;
        DWRITE_HIT_TEST_METRICS caret_metrics {};
        if (SUCCEEDED(text_layout->HitTestTextPosition(static_cast<UINT32>(interactive_controls_.rich_text_box->caret_position()), FALSE, &caret_x, &caret_y, &caret_metrics))) {
            float scroll_offset = interactive_controls_.rich_text_box->scroll_offset();
            if (caret_y < scroll_offset) {
                scroll_offset = caret_y;
            } else if (caret_y + max_value(18.0F, caret_metrics.height) > scroll_offset + rect_height(layout.rich_text_viewport)) {
                scroll_offset = caret_y + max_value(18.0F, caret_metrics.height) - rect_height(layout.rich_text_viewport);
            }
            interactive_controls_.rich_text_box->set_scroll_offset(clamp_rich_text_offset(scroll_offset));
        }
        safe_release(text_layout);
    };
    const auto move_rich_text_caret_vertically = [&](int direction, bool extend_selection) {
        if (interactive_controls_.rich_text_box == nullptr) {
            return false;
        }

        const std::wstring text = utf8_to_wstring(interactive_controls_.rich_text_box->rich_text());
        IDWriteTextLayout* text_layout = nullptr;
        const float content_height = rich_text_content_height();
        if (!create_text_layout(
                dwrite_factory_,
                helper_text_format_ != nullptr ? helper_text_format_ : input_text_format_,
                text,
                rect_width(layout.rich_text_viewport),
                content_height,
                &text_layout)) {
            return false;
        }

        FLOAT caret_x = 0.0F;
        FLOAT caret_y = 0.0F;
        DWRITE_HIT_TEST_METRICS caret_metrics {};
        if (FAILED(text_layout->HitTestTextPosition(static_cast<UINT32>(interactive_controls_.rich_text_box->caret_position()), FALSE, &caret_x, &caret_y, &caret_metrics))) {
            safe_release(text_layout);
            return false;
        }

        const float line_height = max_value(18.0F, caret_metrics.height);
        const float target_y = max_value(0.0F, min_value(content_height - 1.0F, caret_y + static_cast<float>(direction) * line_height));
        BOOL is_trailing_hit = FALSE;
        BOOL is_inside = FALSE;
        DWRITE_HIT_TEST_METRICS hit_metrics {};
        const HRESULT hit_hr = text_layout->HitTestPoint(caret_x + 1.0F, target_y + line_height * 0.5F, &is_trailing_hit, &is_inside, &hit_metrics);
        safe_release(text_layout);
        if (FAILED(hit_hr)) {
            return false;
        }

        interactive_controls_.rich_text_box->set_caret_position(
            static_cast<std::size_t>(hit_metrics.textPosition) + (is_trailing_hit != FALSE ? 1U : 0U),
            extend_selection);
        ensure_rich_text_caret_visible();
        return true;
    };
    const auto clamp_list_view_offset = [&]() {
        if (interactive_controls_.list_view == nullptr) {
            return 0.0F;
        }
        const float total_height = compute_items_total_height(interactive_controls_.list_view->items().size(), 28.0F, 0.0F);
        return max_value(0.0F, total_height - rect_height(layout.list_viewport));
    };
    const auto clamp_items_control_offset = [&]() {
        if (interactive_controls_.items_control == nullptr) {
            return 0.0F;
        }
        const float total_height = compute_items_total_height(interactive_controls_.items_control->items().size(), 28.0F, interactive_controls_.items_control->item_spacing());
        return max_value(0.0F, total_height - rect_height(layout.items_control_viewport));
    };
    const auto update_drag_scroll = [&](float y) {
        switch (drag_scroll_target_) {
        case DragScrollTarget::ScrollViewer: {
            if (!rect_has_area(layout.scroll_viewer_thumb)) {
                return;
            }
            const float max_offset = clamp_scroll_offset(100000.0F);
            const float travel = max_value(1.0F, rect_height(layout.scroll_viewport) - rect_height(layout.scroll_viewer_thumb));
            interactive_controls_.scroll_viewer->set_scroll_offset(
                interactive_controls_.scroll_viewer->scroll_offset().x,
                clamp_scroll_offset(drag_scroll_origin_offset_ + (y - drag_scroll_anchor_y_) * (max_offset / travel)));
            break;
        }
        case DragScrollTarget::ListView: {
            if (!rect_has_area(layout.list_view_thumb)) {
                return;
            }
            const float max_offset = clamp_list_view_offset();
            const float travel = max_value(1.0F, rect_height(layout.list_viewport) - rect_height(layout.list_view_thumb));
            interactive_controls_.list_view->set_scroll_offset(clamp_value(drag_scroll_origin_offset_ + (y - drag_scroll_anchor_y_) * (max_offset / travel), 0.0F, max_offset));
            break;
        }
        case DragScrollTarget::ItemsControl: {
            if (!rect_has_area(layout.items_control_thumb)) {
                return;
            }
            const float max_offset = clamp_items_control_offset();
            const float travel = max_value(1.0F, rect_height(layout.items_control_viewport) - rect_height(layout.items_control_thumb));
            interactive_controls_.items_control->set_scroll_offset(clamp_value(drag_scroll_origin_offset_ + (y - drag_scroll_anchor_y_) * (max_offset / travel), 0.0F, max_offset));
            break;
        }
        case DragScrollTarget::None:
            return;
        }
        window_host_->request_render();
    };

    switch (msg) {
    case WM_GETDLGCODE:
        result = DLGC_WANTARROWS | DLGC_WANTCHARS | DLGC_WANTTAB;
        return true;
    case WM_SETFOCUS:
        sync_focus_state();
        reset_caret_blink();
        result = 0;
        return true;
    case WM_KILLFOCUS:
        text_box_selecting_ = false;
        rich_text_box_selecting_ = false;
        slider_dragging_ = false;
        drag_scroll_target_ = DragScrollTarget::None;
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
        rich_text_box_hovered_ = hit.target == HitTarget::RichTextBox;
        check_box_hovered_ = hit.target == HitTarget::CheckBox;
        combo_box_hovered_ = hit.target == HitTarget::ComboBoxHeader || hit.target == HitTarget::ComboBoxItem;
        slider_hovered_ = hit.target == HitTarget::Slider;
        scroll_viewer_hovered_ = hit.target == HitTarget::ScrollViewer || hit.target == HitTarget::ScrollViewerThumb;
        list_view_hovered_ = hit.target == HitTarget::ListView || hit.target == HitTarget::ListViewThumb;
        items_control_hovered_ = hit.target == HitTarget::ItemsControl || hit.target == HitTarget::ItemsControlThumb;
        hovered_combo_index_ = hit.target == HitTarget::ComboBoxItem ? hit.combo_index : -1;
        hovered_item_index_ = hovered_combo_index_;
        if (text_box_selecting_) {
            update_text_box_selection(x, y, true);
        }
        if (rich_text_box_selecting_) {
            update_rich_text_selection(x, y, true);
        }
        if (slider_dragging_) {
            update_slider_from_x(x);
        }
        if (drag_scroll_target_ != DragScrollTarget::None) {
            update_drag_scroll(y);
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
        case HitTarget::RichTextBox:
            update_focus(kRichTextBoxFocusIndex);
            rich_text_box_selecting_ = true;
            update_rich_text_selection(x, y, false);
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
        case HitTarget::ScrollViewerThumb:
            drag_scroll_target_ = DragScrollTarget::ScrollViewer;
            drag_scroll_anchor_y_ = y;
            drag_scroll_origin_offset_ = interactive_controls_.scroll_viewer != nullptr ? interactive_controls_.scroll_viewer->scroll_offset().y : 0.0F;
            close_combo_box();
            break;
        case HitTarget::ListViewThumb:
            drag_scroll_target_ = DragScrollTarget::ListView;
            drag_scroll_anchor_y_ = y;
            drag_scroll_origin_offset_ = interactive_controls_.list_view != nullptr ? interactive_controls_.list_view->scroll_offset() : 0.0F;
            close_combo_box();
            break;
        case HitTarget::ItemsControlThumb:
            drag_scroll_target_ = DragScrollTarget::ItemsControl;
            drag_scroll_anchor_y_ = y;
            drag_scroll_origin_offset_ = interactive_controls_.items_control != nullptr ? interactive_controls_.items_control->scroll_offset() : 0.0F;
            close_combo_box();
            break;
        case HitTarget::ScrollViewer:
            close_combo_box();
            break;
        case HitTarget::ListView:
            if (interactive_controls_.list_view != nullptr) {
                const float viewport_top = layout.list_viewport.top;
                const float row_height = 28.0F;
                const std::size_t index = static_cast<std::size_t>(max_value(0.0F, y - viewport_top + interactive_controls_.list_view->scroll_offset()) / row_height);
                interactive_controls_.list_view->set_selected_index(index);
                window_host_->request_render();
            }
            close_combo_box();
            break;
        case HitTarget::ItemsControl:
            if (interactive_controls_.items_control != nullptr) {
                const float viewport_top = layout.items_control_viewport.top;
                const float row_stride = 28.0F + interactive_controls_.items_control->item_spacing();
                const std::size_t index = static_cast<std::size_t>(max_value(0.0F, y - viewport_top + interactive_controls_.items_control->scroll_offset()) / row_stride);
                interactive_controls_.items_control->set_selected_index(index);
                window_host_->request_render();
            }
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
        rich_text_box_selecting_ = false;
        slider_dragging_ = false;
        drag_scroll_target_ = DragScrollTarget::None;
        if (GetCapture() == window_host_->hwnd()) {
            ReleaseCapture();
        }
        result = 0;
        return true;
    }
    case WM_MOUSEWHEEL: {
        POINT screen_point {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        ScreenToClient(window_host_->hwnd(), &screen_point);
        const float x = static_cast<float>(screen_point.x);
        const float y = static_cast<float>(screen_point.y);
        if (interactive_controls_.rich_text_box != nullptr && point_in_rect(layout.rich_text_box, x, y)) {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / static_cast<float>(WHEEL_DELTA);
            interactive_controls_.rich_text_box->set_scroll_offset(clamp_rich_text_offset(interactive_controls_.rich_text_box->scroll_offset() - delta * 36.0F));
            window_host_->request_render();
            result = 0;
            return true;
        }
        if (interactive_controls_.scroll_viewer != nullptr && point_in_rect(layout.scroll_viewer, x, y)) {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / static_cast<float>(WHEEL_DELTA);
            interactive_controls_.scroll_viewer->set_scroll_offset(
                interactive_controls_.scroll_viewer->scroll_offset().x,
                clamp_scroll_offset(interactive_controls_.scroll_viewer->scroll_offset().y - delta * 48.0F));
            window_host_->request_render();
            result = 0;
            return true;
        }
        if (interactive_controls_.list_view != nullptr && point_in_rect(layout.list_view, x, y)) {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / static_cast<float>(WHEEL_DELTA);
            interactive_controls_.list_view->set_scroll_offset(clamp_value(interactive_controls_.list_view->scroll_offset() - delta * 32.0F, 0.0F, clamp_list_view_offset()));
            window_host_->request_render();
            result = 0;
            return true;
        }
        if (interactive_controls_.items_control != nullptr && point_in_rect(layout.items_control, x, y)) {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / static_cast<float>(WHEEL_DELTA);
            interactive_controls_.items_control->set_scroll_offset(clamp_value(interactive_controls_.items_control->scroll_offset() - delta * 32.0F, 0.0F, clamp_items_control_offset()));
            window_host_->request_render();
            result = 0;
            return true;
        }
        break;
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
        if (focused_control_index_ == kRichTextBoxFocusIndex && interactive_controls_.rich_text_box != nullptr) {
            const wchar_t wchar = static_cast<wchar_t>(wparam);
            if (wchar == L'\r') {
                interactive_controls_.rich_text_box->insert_text("\n");
                ensure_rich_text_caret_visible();
                reset_caret_blink();
                result = 0;
                return true;
            }
            if (wchar >= 32 && wchar != 127) {
                interactive_controls_.rich_text_box->insert_text(wstring_to_utf8(std::wstring(1, wchar)));
                ensure_rich_text_caret_visible();
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

        if (focused_control_index_ == kRichTextBoxFocusIndex && interactive_controls_.rich_text_box != nullptr) {
            if (ctrl && (wparam == 'A' || wparam == 'C' || wparam == 'V' || wparam == 'X')) {
                if (wparam == 'A') {
                    interactive_controls_.rich_text_box->select_all();
                } else if (wparam == 'C') {
                    const auto [start, end] = interactive_controls_.rich_text_box->selection();
                    const std::wstring text = utf8_to_wstring(interactive_controls_.rich_text_box->rich_text());
                    write_clipboard_text(window_host_->hwnd(), start < end && start < text.size() ? text.substr(start, min_value(end, text.size()) - start) : L"");
                } else if (wparam == 'V') {
                    const std::wstring pasted = read_clipboard_text(window_host_->hwnd());
                    if (!pasted.empty()) {
                        interactive_controls_.rich_text_box->insert_text(wstring_to_utf8(pasted));
                    }
                } else if (wparam == 'X') {
                    const auto [start, end] = interactive_controls_.rich_text_box->selection();
                    const std::wstring text = utf8_to_wstring(interactive_controls_.rich_text_box->rich_text());
                    write_clipboard_text(window_host_->hwnd(), start < end && start < text.size() ? text.substr(start, min_value(end, text.size()) - start) : L"");
                    interactive_controls_.rich_text_box->delete_forward();
                }
                reset_caret_blink();
                result = 0;
                return true;
            }

            switch (wparam) {
            case VK_LEFT:
                interactive_controls_.rich_text_box->move_caret_left(shift);
                break;
            case VK_RIGHT:
                interactive_controls_.rich_text_box->move_caret_right(shift);
                break;
            case VK_UP:
                move_rich_text_caret_vertically(-1, shift);
                break;
            case VK_DOWN:
                move_rich_text_caret_vertically(1, shift);
                break;
            case VK_HOME:
                interactive_controls_.rich_text_box->move_caret_home(shift);
                break;
            case VK_END:
                interactive_controls_.rich_text_box->move_caret_end(shift);
                break;
            case VK_BACK:
                interactive_controls_.rich_text_box->backspace();
                break;
            case VK_DELETE:
                interactive_controls_.rich_text_box->delete_forward();
                break;
            case VK_PRIOR:
                interactive_controls_.rich_text_box->set_scroll_offset(clamp_rich_text_offset(interactive_controls_.rich_text_box->scroll_offset() - rect_height(layout.rich_text_viewport) * 0.85F));
                break;
            case VK_NEXT:
                interactive_controls_.rich_text_box->set_scroll_offset(clamp_rich_text_offset(interactive_controls_.rich_text_box->scroll_offset() + rect_height(layout.rich_text_viewport) * 0.85F));
                break;
            default:
                break;
            }
            ensure_rich_text_caret_visible();
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
    if (interactive_controls_.rich_text_box) {
        interactive_controls_.rich_text_box->set_focused(focused_control_index_ == kRichTextBoxFocusIndex);
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
    if (interactive_controls_.scroll_viewer) {
        interactive_controls_.scroll_viewer->set_focused(false);
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
                L"Microsoft YaHei",
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
                L"Microsoft YaHei",
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
                item_text_format_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            }

            const HRESULT helper_format_hr = dwrite_factory_->CreateTextFormat(
                L"Microsoft YaHei",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                13.0F,
                L"en-us",
                &helper_text_format_);
            if (FAILED(helper_format_hr) || helper_text_format_ == nullptr) {
                helper_text_format_ = nullptr;
            } else {
                helper_text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                helper_text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
                helper_text_format_->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
            }

            const HRESULT input_format_hr = dwrite_factory_->CreateTextFormat(
                L"Microsoft YaHei",
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
                input_text_format_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
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
    safe_release(helper_text_format_);
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
