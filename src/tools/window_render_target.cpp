#include "dcompframe/tools/window_render_target.h"

#include <d2d1helper.h>
#include <dwmapi.h>
#include <dxgi.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <iterator>
#include <string>
#include <vector>

#include <windowsx.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

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
constexpr std::size_t kToggleSwitchFocusIndex = 4;
constexpr std::size_t kComboBoxFocusIndex = 5;
constexpr std::size_t kRadioGroupFocusIndex = 6;
constexpr std::size_t kSliderFocusIndex = 7;
constexpr std::size_t kInteractiveFocusCount = 8;
constexpr UINT_PTR kCaretTimerId = 1;

constexpr float kCaptionHeight = 48.0F;
constexpr float kCaptionButtonWidth = 46.0F;

#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION 0x00AE
#endif

#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME 0x00AF
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

constexpr int kDwmWindowCornerPreferenceRound = 2;

struct ElementPalette {
    D2D1_COLOR_F window_background;
    D2D1_COLOR_F title_background;
    D2D1_COLOR_F title_separator;
    D2D1_COLOR_F surface;
    D2D1_COLOR_F surface_alt;
    D2D1_COLOR_F surface_hover;
    D2D1_COLOR_F surface_active;
    D2D1_COLOR_F border;
    D2D1_COLOR_F border_hover;
    D2D1_COLOR_F border_focus;
    D2D1_COLOR_F primary;
    D2D1_COLOR_F primary_hover;
    D2D1_COLOR_F primary_pressed;
    D2D1_COLOR_F primary_soft;
    D2D1_COLOR_F primary_soft_hover;
    D2D1_COLOR_F primary_soft_active;
    D2D1_COLOR_F text_primary;
    D2D1_COLOR_F text_secondary;
    D2D1_COLOR_F text_inverse;
    D2D1_COLOR_F success_background;
    D2D1_COLOR_F success_text;
    D2D1_COLOR_F warning_background;
    D2D1_COLOR_F warning_text;
    D2D1_COLOR_F danger_background;
    D2D1_COLOR_F danger_text;
    D2D1_COLOR_F scrollbar_track;
    D2D1_COLOR_F scrollbar_thumb;
    D2D1_COLOR_F scrollbar_thumb_hover;
};

const ElementPalette& element_palette() {
    static const ElementPalette palette {
        D2D1::ColorF(0.97F, 0.98F, 0.99F, 1.0F),
        D2D1::ColorF(0.98F, 0.99F, 1.0F, 0.98F),
        D2D1::ColorF(0.88F, 0.91F, 0.95F, 1.0F),
        D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F),
        D2D1::ColorF(0.98F, 0.99F, 1.0F, 1.0F),
        D2D1::ColorF(0.95F, 0.97F, 1.0F, 1.0F),
        D2D1::ColorF(0.93F, 0.96F, 1.0F, 1.0F),
        D2D1::ColorF(0.86F, 0.89F, 0.94F, 1.0F),
        D2D1::ColorF(0.76F, 0.81F, 0.89F, 1.0F),
        D2D1::ColorF(0.25F, 0.48F, 0.95F, 1.0F),
        D2D1::ColorF(0.25F, 0.48F, 0.95F, 1.0F),
        D2D1::ColorF(0.16F, 0.42F, 0.87F, 1.0F),
        D2D1::ColorF(0.13F, 0.35F, 0.74F, 1.0F),
        D2D1::ColorF(0.93F, 0.96F, 1.0F, 1.0F),
        D2D1::ColorF(0.90F, 0.94F, 1.0F, 1.0F),
        D2D1::ColorF(0.84F, 0.90F, 1.0F, 1.0F),
        D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F),
        D2D1::ColorF(0.38F, 0.40F, 0.43F, 1.0F),
        D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F),
        D2D1::ColorF(0.93F, 0.97F, 0.94F, 1.0F),
        D2D1::ColorF(0.22F, 0.56F, 0.28F, 1.0F),
        D2D1::ColorF(1.0F, 0.96F, 0.88F, 1.0F),
        D2D1::ColorF(0.77F, 0.48F, 0.06F, 1.0F),
        D2D1::ColorF(1.0F, 0.93F, 0.93F, 1.0F),
        D2D1::ColorF(0.82F, 0.17F, 0.17F, 1.0F),
        D2D1::ColorF(0.95F, 0.96F, 0.98F, 1.0F),
        D2D1::ColorF(0.72F, 0.77F, 0.86F, 1.0F),
        D2D1::ColorF(0.52F, 0.61F, 0.79F, 1.0F),
    };
    return palette;
}

unsigned int current_window_dpi(HWND hwnd) {
    return hwnd != nullptr ? GetDpiForWindow(hwnd) : GetDpiForSystem();
}

int resize_frame_thickness_x(unsigned int dpi) {
    return GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

int resize_frame_thickness_y(unsigned int dpi) {
    return GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

enum class HitTarget {
    None,
    CaptionMinimize,
    CaptionMaximize,
    CaptionClose,
    PrimaryButton,
    TextBox,
    RichTextBox,
    CheckBox,
    ToggleSwitch,
    ComboBoxHeader,
    ComboBoxItem,
    ComboBoxThumb,
    ComboBoxScrollBar,
    RadioGroupOption,
    Slider,
    ScrollViewerThumb,
    ScrollViewerScrollBar,
    ListViewThumb,
    ListViewScrollBar,
    ItemsControlThumb,
    ItemsControlScrollBar,
    LogBoxThumb,
    LogBoxScrollBar,
    TabControl,
    ExpanderHeader,
    ProgressDecrease,
    ProgressIncrease,
    ScrollViewer,
    ListView,
    ItemsControl,
    LogBox
};

struct HitResult {
    HitTarget target = HitTarget::None;
    int combo_index = -1;
    int radio_index = -1;
};

struct OverlayLayout {
    D2D1_RECT_F card {};
    D2D1_RECT_F title_band {};
    D2D1_RECT_F caption_icon {};
    D2D1_RECT_F caption_minimize {};
    D2D1_RECT_F caption_maximize {};
    D2D1_RECT_F caption_close {};
    D2D1_RECT_F title {};
    D2D1_RECT_F subtitle {};
    D2D1_RECT_F button {};
    D2D1_RECT_F content_clip {};
    D2D1_RECT_F left_column {};
    D2D1_RECT_F right_column {};
    D2D1_RECT_F text_box_label {};
    D2D1_RECT_F text_box {};
    D2D1_RECT_F text_box_text {};
    D2D1_RECT_F rich_text_label {};
    D2D1_RECT_F rich_text_box {};
    D2D1_RECT_F rich_text_viewport {};
    D2D1_RECT_F check_box {};
    D2D1_RECT_F toggle_label {};
    D2D1_RECT_F toggle_switch {};
    D2D1_RECT_F combo_label {};
    D2D1_RECT_F combo_box {};
    D2D1_RECT_F radio_label {};
    D2D1_RECT_F radio_group {};
    D2D1_RECT_F slider_label {};
    D2D1_RECT_F slider {};
    D2D1_RECT_F text_block_label {};
    D2D1_RECT_F text_block {};
    D2D1_RECT_F label_chip {};
    D2D1_RECT_F badge {};
    D2D1_RECT_F divider {};
    D2D1_RECT_F image_label {};
    D2D1_RECT_F image {};
    D2D1_RECT_F card_preview {};
    D2D1_RECT_F preview_header {};
    D2D1_RECT_F tab_control {};
    D2D1_RECT_F tab_body {};
    D2D1_RECT_F animation_demo {};
    D2D1_RECT_F expander_header {};
    D2D1_RECT_F expander_body {};
    D2D1_RECT_F progress_label {};
    D2D1_RECT_F progress_decrease {};
    D2D1_RECT_F progress_bar {};
    D2D1_RECT_F progress_increase {};
    D2D1_RECT_F loading_badge {};
    D2D1_RECT_F popup_preview {};
    D2D1_RECT_F list_view_label {};
    D2D1_RECT_F list_view {};
    D2D1_RECT_F list_viewport {};
    D2D1_RECT_F list_view_scrollbar {};
    D2D1_RECT_F list_view_thumb {};
    D2D1_RECT_F items_control_label {};
    D2D1_RECT_F items_control {};
    D2D1_RECT_F items_control_viewport {};
    D2D1_RECT_F items_control_scrollbar {};
    D2D1_RECT_F items_control_thumb {};
    D2D1_RECT_F scroll_viewer_label {};
    D2D1_RECT_F scroll_viewer {};
    D2D1_RECT_F scroll_viewport {};
    D2D1_RECT_F scroll_viewer_scrollbar {};
    D2D1_RECT_F scroll_viewer_thumb {};
    D2D1_RECT_F footer_label {};
    D2D1_RECT_F footer {};
    D2D1_RECT_F footer_viewport {};
    D2D1_RECT_F footer_scrollbar {};
    D2D1_RECT_F footer_thumb {};
    D2D1_RECT_F combo_dropdown {};
    D2D1_RECT_F combo_dropdown_viewport {};
    D2D1_RECT_F combo_dropdown_thumb {};
    D2D1_RECT_F combo_dropdown_scrollbar {};
    D2D1_RECT_F slider_track {};
    std::vector<D2D1_RECT_F> combo_items;
    std::vector<D2D1_RECT_F> radio_items;
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

float compute_combo_dropdown_total_height(std::size_t item_count, float item_height) {
    return static_cast<float>(item_count) * item_height;
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

D2D1_RECT_F compute_vertical_scrollbar_track(const D2D1_RECT_F& viewport, float width = 6.0F) {
    if (!rect_has_area(viewport)) {
        return D2D1::RectF(0.0F, 0.0F, 0.0F, 0.0F);
    }

    return D2D1::RectF(viewport.right - width, viewport.top, viewport.right, viewport.bottom);
}

float compute_track_scroll_offset(const D2D1_RECT_F& viewport, const D2D1_RECT_F& thumb, float click_y, float max_offset) {
    if (!rect_has_area(viewport) || max_offset <= 0.0F) {
        return 0.0F;
    }

    const float thumb_height = rect_has_area(thumb) ? rect_height(thumb) : 28.0F;
    const float track_travel = max_value(1.0F, rect_height(viewport) - thumb_height);
    const float thumb_top = clamp_value(click_y - thumb_height * 0.5F, viewport.top, viewport.bottom - thumb_height);
    const float ratio = (thumb_top - viewport.top) / track_travel;
    return clamp_value(ratio * max_offset, 0.0F, max_offset);
}

D2D1_RECT_F rect_from_bounds(const Rect& bounds) {
    return D2D1::RectF(bounds.x, bounds.y, bounds.x + bounds.width, bounds.y + bounds.height);
}

D2D1_RECT_F control_rect(const std::shared_ptr<UIElement>& element) {
    if (element == nullptr) {
        return D2D1::RectF(0.0F, 0.0F, 0.0F, 0.0F);
    }

    return rect_from_bounds(element->absolute_bounds());
}

D2D1_RECT_F label_rect_above(const D2D1_RECT_F& rect, float height = 18.0F, float gap = 6.0F) {
    if (!rect_has_area(rect)) {
        return D2D1::RectF(0.0F, 0.0F, 0.0F, 0.0F);
    }

    return D2D1::RectF(rect.left, rect.top - gap - height, rect.right, rect.top - gap);
}

OverlayLayout compute_layout(
    float width,
    float height,
    const WindowRenderTarget::InteractiveControls& controls,
    float combo_box_scroll_offset,
    float page_scroll_offset) {
    OverlayLayout layout;
    layout.card = D2D1::RectF(0.0F, 0.0F, width, height);
    layout.title_band = D2D1::RectF(0.0F, 0.0F, width, kCaptionHeight);
    layout.caption_icon = D2D1::RectF(14.0F, 10.0F, 42.0F, 38.0F);
    layout.caption_close = D2D1::RectF(width - kCaptionButtonWidth, 0.0F, width, kCaptionHeight);
    layout.caption_maximize = D2D1::RectF(layout.caption_close.left - kCaptionButtonWidth, 0.0F, layout.caption_close.left, kCaptionHeight);
    layout.caption_minimize = D2D1::RectF(layout.caption_maximize.left - kCaptionButtonWidth, 0.0F, layout.caption_maximize.left, kCaptionHeight);
    layout.title = D2D1::RectF(layout.caption_icon.right + 10.0F, 10.0F, layout.caption_minimize.left - 24.0F, 34.0F);
    layout.subtitle = D2D1::RectF(layout.caption_icon.right + 10.0F, 28.0F, layout.caption_minimize.left - 24.0F, 44.0F);
    layout.content_clip = D2D1::RectF(0.0F, kCaptionHeight, width, height);

    const float label_height = 18.0F;
    const auto scrolled_control_rect = [&](const std::shared_ptr<UIElement>& element) {
        D2D1_RECT_F rect = control_rect(element);
        rect.top -= page_scroll_offset;
        rect.bottom -= page_scroll_offset;
        return rect;
    };

    layout.button = scrolled_control_rect(controls.primary_button);
    layout.text_box = scrolled_control_rect(controls.text_box);
    layout.text_box_label = label_rect_above(layout.text_box, label_height);
    layout.text_box_text = inset_rect(layout.text_box, 14.0F, 8.0F);

    layout.rich_text_box = scrolled_control_rect(controls.rich_text_box);
    layout.rich_text_label = label_rect_above(layout.rich_text_box, label_height);
    layout.rich_text_viewport = inset_rect(layout.rich_text_box, 14.0F, 12.0F);

    layout.check_box = scrolled_control_rect(controls.check_box);

    layout.toggle_switch = scrolled_control_rect(controls.toggle_switch);
    layout.toggle_label = label_rect_above(layout.toggle_switch, label_height);

    layout.combo_box = scrolled_control_rect(controls.combo_box);
    layout.combo_label = label_rect_above(layout.combo_box, label_height);
    if (controls.combo_box && controls.combo_box->is_dropdown_open()) {
        const float item_height = 36.0F;
        const std::size_t visible_count = min_value<std::size_t>(controls.combo_box->items().size(), 8U);
        layout.combo_dropdown = D2D1::RectF(
            layout.combo_box.left,
            layout.combo_box.bottom + 4.0F,
            layout.combo_box.right,
            layout.combo_box.bottom + 12.0F + item_height * static_cast<float>(visible_count));
        layout.combo_dropdown_viewport = inset_rect(layout.combo_dropdown, 4.0F, 4.0F);
        const float total_height = compute_combo_dropdown_total_height(controls.combo_box->items().size(), item_height);
        layout.combo_dropdown_scrollbar = compute_vertical_scrollbar_track(layout.combo_dropdown_viewport);
        layout.combo_dropdown_thumb = compute_vertical_thumb(layout.combo_dropdown_viewport, total_height, combo_box_scroll_offset);
        layout.combo_items.reserve(controls.combo_box->items().size());
        for (std::size_t index = 0; index < controls.combo_box->items().size(); ++index) {
            const float item_top = layout.combo_dropdown_viewport.top + item_height * static_cast<float>(index) - combo_box_scroll_offset;
            layout.combo_items.push_back(D2D1::RectF(layout.combo_dropdown.left + 4.0F, item_top, layout.combo_dropdown.right - 4.0F, item_top + item_height));
        }
    }

    layout.radio_group = scrolled_control_rect(controls.radio_group);
    layout.radio_label = label_rect_above(layout.radio_group, label_height);
    if (controls.radio_group != nullptr) {
        const auto& items = controls.radio_group->items();
        layout.radio_items.reserve(items.size());
        const float segment_width = items.empty() ? rect_width(layout.radio_group) : rect_width(layout.radio_group) / static_cast<float>(items.size());
        for (std::size_t index = 0; index < items.size(); ++index) {
            layout.radio_items.push_back(D2D1::RectF(
                layout.radio_group.left + static_cast<float>(index) * segment_width,
                layout.radio_group.top,
                layout.radio_group.left + static_cast<float>(index + 1U) * segment_width,
                layout.radio_group.bottom));
        }
    }

    layout.slider = scrolled_control_rect(controls.slider);
    layout.slider_label = label_rect_above(layout.slider, label_height);
    layout.slider_track = D2D1::RectF(layout.slider.left + 16.0F, layout.slider.top + 18.0F, layout.slider.right - 76.0F, layout.slider.top + 24.0F);

    layout.text_block = scrolled_control_rect(controls.text_block);
    layout.text_block_label = label_rect_above(layout.text_block, label_height);
    layout.label_chip = scrolled_control_rect(controls.label);
    layout.badge = scrolled_control_rect(controls.badge);
    layout.divider = scrolled_control_rect(controls.divider);

    layout.image = scrolled_control_rect(controls.image);
    layout.image_label = label_rect_above(layout.image, label_height);

    layout.card_preview = scrolled_control_rect(controls.card);
    layout.preview_header = inset_rect(layout.card_preview, 12.0F, 12.0F);
    layout.tab_control = scrolled_control_rect(controls.tab_control);
    if (rect_has_area(layout.tab_control)) {
        layout.tab_body = D2D1::RectF(layout.tab_control.left + 12.0F, layout.tab_control.top + 44.0F, layout.tab_control.right - 12.0F, layout.tab_control.bottom - 12.0F);
        layout.animation_demo = D2D1::RectF(layout.tab_body.left, layout.tab_body.bottom - 28.0F, layout.tab_body.right, layout.tab_body.bottom);
    }
    const D2D1_RECT_F expander_rect = scrolled_control_rect(controls.expander);
    layout.expander_header = rect_has_area(expander_rect)
        ? D2D1::RectF(expander_rect.left, expander_rect.top, expander_rect.right, min_value(expander_rect.bottom, expander_rect.top + 34.0F))
        : D2D1::RectF(0.0F, 0.0F, 0.0F, 0.0F);
    layout.expander_body = rect_has_area(expander_rect)
        ? D2D1::RectF(expander_rect.left + 4.0F, layout.expander_header.bottom + 6.0F, expander_rect.right - 4.0F, expander_rect.bottom - 4.0F)
        : D2D1::RectF(0.0F, 0.0F, 0.0F, 0.0F);

    const D2D1_RECT_F progress_rect = scrolled_control_rect(controls.progress);
    if (rect_has_area(progress_rect)) {
        layout.progress_label = D2D1::RectF(progress_rect.left, progress_rect.top + 2.0F, progress_rect.left + 54.0F, progress_rect.bottom - 2.0F);
        layout.progress_decrease = D2D1::RectF(progress_rect.left + 62.0F, progress_rect.top + 6.0F, progress_rect.left + 88.0F, progress_rect.bottom - 6.0F);
        layout.progress_increase = D2D1::RectF(progress_rect.right - 26.0F, progress_rect.top + 6.0F, progress_rect.right, progress_rect.bottom - 6.0F);
        layout.progress_bar = D2D1::RectF(layout.progress_decrease.right + 8.0F, progress_rect.top + 12.0F, layout.progress_increase.left - 8.0F, progress_rect.bottom - 12.0F);
    }

    layout.loading_badge = scrolled_control_rect(controls.loading);
    layout.popup_preview = scrolled_control_rect(controls.popup);

    layout.list_view = scrolled_control_rect(controls.list_view);
    layout.list_view_label = label_rect_above(layout.list_view, label_height);
    layout.items_control = scrolled_control_rect(controls.items_control);
    layout.items_control_label = label_rect_above(layout.items_control, label_height);
    layout.scroll_viewer = scrolled_control_rect(controls.scroll_viewer);
    layout.scroll_viewer_label = label_rect_above(layout.scroll_viewer, label_height);
    layout.footer = scrolled_control_rect(controls.log_box);
    layout.footer_label = label_rect_above(layout.footer, label_height);

    layout.left_column = D2D1::RectF(
        min_value(layout.text_box.left, min_value(layout.check_box.left, layout.toggle_switch.left)),
        min_value(layout.text_box.top, layout.rich_text_box.top),
        max_value(layout.slider.right, max_value(layout.combo_box.right, layout.radio_group.right)),
        max_value(layout.slider.bottom, layout.rich_text_box.bottom));
    layout.right_column = D2D1::RectF(
        min_value(layout.text_block.left, layout.image.left),
        min_value(layout.text_block.top, layout.image.top),
        max_value(layout.popup_preview.right, max_value(layout.card_preview.right, layout.items_control.right)),
        max_value(layout.footer.bottom, max_value(layout.card_preview.bottom, layout.items_control.bottom)));

    layout.scroll_viewport = inset_rect(layout.scroll_viewer, 12.0F, 12.0F);
    layout.list_viewport = inset_rect(layout.list_view, 8.0F, 8.0F);
    layout.items_control_viewport = inset_rect(layout.items_control, 8.0F, 8.0F);
    layout.footer_viewport = inset_rect(layout.footer, 10.0F, 10.0F);

    if (controls.list_view != nullptr) {
        const float total_height = compute_items_total_height(controls.list_view->items().size(), 28.0F, 0.0F);
        layout.list_view_scrollbar = compute_vertical_scrollbar_track(layout.list_viewport);
        layout.list_view_thumb = compute_vertical_thumb(layout.list_viewport, total_height, controls.list_view->scroll_offset());
    }
    if (controls.items_control != nullptr) {
        const float total_height = compute_items_total_height(controls.items_control->items().size(), 28.0F, controls.items_control->item_spacing());
        layout.items_control_scrollbar = compute_vertical_scrollbar_track(layout.items_control_viewport);
        layout.items_control_thumb = compute_vertical_thumb(layout.items_control_viewport, total_height, controls.items_control->scroll_offset());
    }
    if (controls.scroll_viewer != nullptr && controls.scroll_viewer->content() != nullptr) {
        const auto content = controls.scroll_viewer->content();
        const float total_height = compute_items_total_height(content->items().size(), 28.0F, content->item_spacing());
        layout.scroll_viewer_scrollbar = compute_vertical_scrollbar_track(layout.scroll_viewport);
        layout.scroll_viewer_thumb = compute_vertical_thumb(layout.scroll_viewport, total_height, controls.scroll_viewer->scroll_offset().y);
    }
    if (controls.log_box != nullptr) {
        const float total_height = compute_items_total_height(controls.log_box->lines().size(), 20.0F, 4.0F);
        layout.footer_scrollbar = compute_vertical_scrollbar_track(layout.footer_viewport);
        layout.footer_thumb = compute_vertical_thumb(layout.footer_viewport, total_height, controls.log_box->scroll_offset());
    }

    return layout;
}

void draw_editor_caret(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    float x,
    float top,
    float bottom,
    float thickness = 1.5F) {
    if (context == nullptr || brush == nullptr || bottom <= top) {
        return;
    }

    brush->SetColor(D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F));
    context->DrawLine(D2D1::Point2F(x, top), D2D1::Point2F(x, bottom), brush, thickness);
}

HitResult hit_test(const OverlayLayout& layout, float x, float y) {
    if (point_in_rect(layout.caption_close, x, y)) {
        return {.target = HitTarget::CaptionClose};
    }
    if (point_in_rect(layout.caption_maximize, x, y)) {
        return {.target = HitTarget::CaptionMaximize};
    }
    if (point_in_rect(layout.caption_minimize, x, y)) {
        return {.target = HitTarget::CaptionMinimize};
    }
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
    if (point_in_rect(layout.toggle_switch, x, y)) {
        return {.target = HitTarget::ToggleSwitch};
    }
    if (point_in_rect(layout.combo_box, x, y)) {
        return {.target = HitTarget::ComboBoxHeader};
    }
    if (point_in_rect(layout.combo_dropdown_thumb, x, y)) {
        return {.target = HitTarget::ComboBoxThumb};
    }
    if (point_in_rect(layout.combo_dropdown_scrollbar, x, y)) {
        return {.target = HitTarget::ComboBoxScrollBar};
    }
    for (std::size_t index = 0; index < layout.combo_items.size(); ++index) {
        if (point_in_rect(layout.combo_items[index], x, y)) {
            return {.target = HitTarget::ComboBoxItem, .combo_index = static_cast<int>(index)};
        }
    }
    for (std::size_t index = 0; index < layout.radio_items.size(); ++index) {
        if (point_in_rect(layout.radio_items[index], x, y)) {
            return {.target = HitTarget::RadioGroupOption, .radio_index = static_cast<int>(index)};
        }
    }
    if (point_in_rect(layout.scroll_viewer_thumb, x, y)) {
        return {.target = HitTarget::ScrollViewerThumb};
    }
    if (point_in_rect(layout.scroll_viewer_scrollbar, x, y)) {
        return {.target = HitTarget::ScrollViewerScrollBar};
    }
    if (point_in_rect(layout.list_view_thumb, x, y)) {
        return {.target = HitTarget::ListViewThumb};
    }
    if (point_in_rect(layout.list_view_scrollbar, x, y)) {
        return {.target = HitTarget::ListViewScrollBar};
    }
    if (point_in_rect(layout.items_control_thumb, x, y)) {
        return {.target = HitTarget::ItemsControlThumb};
    }
    if (point_in_rect(layout.items_control_scrollbar, x, y)) {
        return {.target = HitTarget::ItemsControlScrollBar};
    }
    if (point_in_rect(layout.footer_thumb, x, y)) {
        return {.target = HitTarget::LogBoxThumb};
    }
    if (point_in_rect(layout.footer_scrollbar, x, y)) {
        return {.target = HitTarget::LogBoxScrollBar};
    }
    if (point_in_rect(layout.slider, x, y)) {
        return {.target = HitTarget::Slider};
    }
    if (point_in_rect(layout.tab_control, x, y)) {
        return {.target = HitTarget::TabControl};
    }
    if (point_in_rect(layout.expander_header, x, y)) {
        return {.target = HitTarget::ExpanderHeader};
    }
    if (point_in_rect(layout.progress_decrease, x, y)) {
        return {.target = HitTarget::ProgressDecrease};
    }
    if (point_in_rect(layout.progress_increase, x, y)) {
        return {.target = HitTarget::ProgressIncrease};
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
    if (point_in_rect(layout.footer, x, y)) {
        return {.target = HitTarget::LogBox};
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
    const auto& palette = element_palette();
    const float cx = rect.left + 28.0F; // Center of the checkbox
    const float cy = (rect.top + rect.bottom) / 2.0F;
    const D2D1_RECT_F box = D2D1::RectF(cx - 10.0F, cy - 10.0F, cx + 10.0F, cy + 10.0F);

    if (checked) {
        brush->SetColor(palette.primary);
        context->FillRoundedRectangle(D2D1::RoundedRect(box, 4.0F, 4.0F), brush);

        brush->SetColor(palette.text_inverse);
        context->DrawLine(D2D1::Point2F(box.left + 5.0F, box.top + 10.0F), D2D1::Point2F(box.left + 9.0F, box.bottom - 6.0F), brush, 2.0F);
        context->DrawLine(D2D1::Point2F(box.left + 9.0F, box.bottom - 6.0F), D2D1::Point2F(box.right - 4.0F, box.top + 5.0F), brush, 2.0F);
    } else {
        brush->SetColor(palette.surface);
        context->FillRoundedRectangle(D2D1::RoundedRect(box, 4.0F, 4.0F), brush);
        brush->SetColor(palette.border);
        context->DrawRoundedRectangle(D2D1::RoundedRect(box, 4.0F, 4.0F), brush, 1.0F);
    }
}

void draw_combobox_glyph(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect) {
    const auto& palette = element_palette();
    const float center_x = rect.right - 28.0F;
    const float center_y = (rect.top + rect.bottom) * 0.5F;
    brush->SetColor(palette.text_secondary);
    context->DrawLine(D2D1::Point2F(center_x - 6.0F, center_y - 3.0F), D2D1::Point2F(center_x, center_y + 3.0F), brush, 2.0F);
    context->DrawLine(D2D1::Point2F(center_x, center_y + 3.0F), D2D1::Point2F(center_x + 6.0F, center_y - 3.0F), brush, 2.0F);
}

void draw_slider_glyph(ID2D1DeviceContext* context, ID2D1SolidColorBrush* brush, const D2D1_RECT_F& rect, float value) {
    const auto& palette = element_palette();
    const float track_left = rect.left + 18.0F;
    const float track_right = rect.right - 18.0F;
    const float center_y = (rect.top + rect.bottom) * 0.5F;
    const float thumb_x = track_left + (track_right - track_left) * clamp_value(value, 0.0F, 1.0F);

    brush->SetColor(palette.surface_active);
    context->DrawLine(D2D1::Point2F(track_left, center_y), D2D1::Point2F(track_right, center_y), brush, 6.0F);
    brush->SetColor(palette.primary);
    context->DrawLine(D2D1::Point2F(track_left, center_y), D2D1::Point2F(thumb_x, center_y), brush, 6.0F);
    brush->SetColor(palette.text_inverse);
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

D2D1_COLOR_F badge_fill_color(BadgeTone tone) {
    const auto& palette = element_palette();
    switch (tone) {
    case BadgeTone::Primary:
        return palette.primary_soft;
    case BadgeTone::Success:
        return palette.success_background;
    case BadgeTone::Warning:
        return palette.warning_background;
    case BadgeTone::Danger:
        return palette.danger_background;
    case BadgeTone::Neutral:
    default:
        return palette.surface_active;
    }
}

D2D1_COLOR_F badge_text_color(BadgeTone tone) {
    const auto& palette = element_palette();
    switch (tone) {
    case BadgeTone::Primary:
        return palette.primary;
    case BadgeTone::Success:
        return palette.success_text;
    case BadgeTone::Warning:
        return palette.warning_text;
    case BadgeTone::Danger:
        return palette.danger_text;
    case BadgeTone::Neutral:
    default:
        return palette.text_secondary;
    }
}

void draw_caption_button(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteTextFormat* format,
    const D2D1_RECT_F& rect,
    const wchar_t* glyph,
    bool hovered,
    bool pressed,
    bool destructive = false) {
    if (!rect_has_area(rect) || context == nullptr || brush == nullptr || format == nullptr || glyph == nullptr) {
        return;
    }

    const auto& palette = element_palette();

    const D2D1_COLOR_F fill = destructive
        ? (pressed ? palette.danger_text : (hovered ? D2D1::ColorF(0.96F, 0.30F, 0.30F, 1.0F) : D2D1::ColorF(0.0F, 0.0F, 0.0F, 0.0F)))
        : (pressed ? palette.surface_active : (hovered ? palette.surface_hover : D2D1::ColorF(0.0F, 0.0F, 0.0F, 0.0F)));
    if (fill.a > 0.0F) {
        fill_rounded_rect(context, brush, inset_rect(rect, 4.0F, 6.0F), fill, 8.0F);
    }
    draw_text_line(
        context,
        brush,
        format,
        glyph,
        rect,
        destructive && hovered ? palette.text_inverse : palette.text_secondary,
        DWRITE_TEXT_ALIGNMENT_CENTER);
}

void draw_toggle_switch_control(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteTextFormat* format,
    const D2D1_RECT_F& rect,
    bool checked,
    bool hovered,
    bool focused) {
    if (!rect_has_area(rect) || context == nullptr || brush == nullptr) {
        return;
    }
    (void)format;

    const auto& palette = element_palette();

    const D2D1_RECT_F track = inset_rect(rect, 6.0F, 6.0F);
    fill_rounded_rect(
        context,
        brush,
        track,
        checked ? palette.primary : (hovered ? palette.primary_soft_hover : palette.surface_active),
        rect_height(track) * 0.5F);
    if (focused) {
        stroke_rounded_rect(context, brush, inset_rect(track, -2.0F, -2.0F), palette.border_focus, 2.0F, rect_height(track) * 0.5F);
    }

    const float thumb_radius = max_value(8.0F, rect_height(track) * 0.5F - 3.0F);
    const float thumb_center_x = checked ? track.right - thumb_radius - 4.0F : track.left + thumb_radius + 4.0F;
    const float thumb_center_y = (track.top + track.bottom) * 0.5F;
    brush->SetColor(palette.text_inverse);
    context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(thumb_center_x, thumb_center_y), thumb_radius, thumb_radius), brush);
}

void draw_radio_group_control(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteTextFormat* format,
    const RadioGroup& radio_group,
    const OverlayLayout& layout,
    int hovered_index,
    bool focused) {
    if (context == nullptr || brush == nullptr || format == nullptr || !rect_has_area(layout.radio_group)) {
        return;
    }

    const auto& palette = element_palette();

    fill_rounded_rect(context, brush, layout.radio_group, palette.surface_alt, 10.0F);
    stroke_rounded_rect(
        context,
        brush,
        layout.radio_group,
        focused ? palette.border_focus : palette.border,
        focused ? 2.0F : 1.0F,
        10.0F);

    const auto selected = radio_group.selected_index();
    const auto& items = radio_group.items();
    for (std::size_t index = 0; index < layout.radio_items.size() && index < items.size(); ++index) {
        const D2D1_RECT_F item_rect = inset_rect(layout.radio_items[index], 3.0F, 4.0F);
        const bool active = selected.has_value() && *selected == index;
        fill_rounded_rect(
            context,
            brush,
            item_rect,
            active ? palette.primary : (hovered_index == static_cast<int>(index) ? palette.primary_soft_hover : palette.surface),
            8.0F);
        draw_text_line(
            context,
            brush,
            format,
            utf8_to_wstring(items[index]),
            item_rect,
            active ? palette.text_inverse : palette.text_secondary,
            DWRITE_TEXT_ALIGNMENT_CENTER);
    }
}

void draw_badge_control(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteTextFormat* format,
    const Badge& badge,
    const D2D1_RECT_F& rect) {
    if (context == nullptr || brush == nullptr || format == nullptr || !rect_has_area(rect)) {
        return;
    }

    fill_rounded_rect(context, brush, rect, badge_fill_color(badge.tone()), rect_height(rect) * 0.5F);
    draw_text_line(context, brush, format, utf8_to_wstring(badge.text()), rect, badge_text_color(badge.tone()), DWRITE_TEXT_ALIGNMENT_CENTER);
}

void draw_divider_control(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    const Divider& divider,
    const D2D1_RECT_F& rect) {
    if (context == nullptr || brush == nullptr || !rect_has_area(rect)) {
        return;
    }

    const auto& palette = element_palette();

    brush->SetColor(palette.border);
    if (divider.orientation() == DividerOrientation::Vertical) {
        const float center_x = (rect.left + rect.right) * 0.5F;
        context->DrawLine(D2D1::Point2F(center_x, rect.top), D2D1::Point2F(center_x, rect.bottom), brush, 1.0F);
    } else {
        const float center_y = (rect.top + rect.bottom) * 0.5F;
        context->DrawLine(D2D1::Point2F(rect.left, center_y), D2D1::Point2F(rect.right, center_y), brush, 1.0F);
    }
}

void apply_dwm_frame(HWND hwnd) {
    if (hwnd == nullptr) {
        return;
    }

    const DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
    DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
    const BOOL dark_mode = FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark_mode, sizeof(dark_mode));
    const int corner_preference = kDwmWindowCornerPreferenceRound;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_preference, sizeof(corner_preference));
    const MARGINS margins {0, 0, 1, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

void draw_image_placeholder(
    ID2D1DeviceContext* context,
    ID2D1SolidColorBrush* brush,
    IDWriteTextFormat* format,
    const D2D1_RECT_F& rect) {
    const auto& palette = element_palette();
    fill_rounded_rect(context, brush, rect, palette.surface_alt, 10.0F);
    stroke_rounded_rect(context, brush, rect, palette.border, 1.0F, 10.0F);

    const D2D1_RECT_F image_box = D2D1::RectF(rect.left + 16.0F, rect.top + 14.0F, rect.left + 92.0F, rect.bottom - 14.0F);
    brush->SetColor(palette.primary_soft);
    context->FillRoundedRectangle(D2D1::RoundedRect(image_box, 6.0F, 6.0F), brush);
    brush->SetColor(palette.border_hover);
    context->DrawLine(D2D1::Point2F(image_box.left + 10.0F, image_box.bottom - 14.0F), D2D1::Point2F(image_box.left + 28.0F, image_box.top + 28.0F), brush, 2.0F);
    context->DrawLine(D2D1::Point2F(image_box.left + 28.0F, image_box.top + 28.0F), D2D1::Point2F(image_box.left + 44.0F, image_box.bottom - 24.0F), brush, 2.0F);
    context->DrawLine(D2D1::Point2F(image_box.left + 44.0F, image_box.bottom - 24.0F), D2D1::Point2F(image_box.right - 10.0F, image_box.top + 20.0F), brush, 2.0F);
    context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(image_box.right - 18.0F, image_box.top + 18.0F), 5.0F, 5.0F), brush);

    draw_text_line(context, brush, format, L"Image 控件示例", D2D1::RectF(image_box.right + 16.0F, rect.top + 16.0F, rect.right - 16.0F, rect.top + 38.0F), palette.text_primary);
    draw_text_line(context, brush, format, L"Element Plus 风格图片区块", D2D1::RectF(image_box.right + 16.0F, rect.top + 42.0F, rect.right - 16.0F, rect.top + 64.0F), palette.text_secondary);
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
    const auto& palette = element_palette();
    fill_rounded_rect(context, brush, rect, palette.surface, 8.0F);
    stroke_rounded_rect(
        context,
        brush,
        rect,
        focused ? palette.border_focus : (hovered ? palette.border_hover : palette.border),
        focused ? 2.0F : 1.0F,
        8.0F);

    const D2D1_RECT_F text_rect = inset_rect(rect, 14.0F, 8.0F);
    const std::wstring wide_text = utf8_to_wstring(text_box.text());
    const bool use_placeholder = wide_text.empty();
    const std::wstring display_text = use_placeholder ? utf8_to_wstring(text_box.placeholder()) : wide_text;

    IDWriteTextLayout* layout = nullptr;
    if (!create_text_layout(dwrite_factory, input_format, display_text, rect_width(text_rect), max_value(24.0F, rect_height(text_rect)), &layout)) {
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
                brush->SetColor(D2D1::ColorF(palette.primary.r, palette.primary.g, palette.primary.b, 0.22F));
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

    brush->SetColor(use_placeholder ? palette.text_secondary : palette.text_primary);
    context->DrawTextLayout(D2D1::Point2F(text_rect.left, text_rect.top), layout, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

    if (focused && caret_visible) {
        FLOAT caret_x = text_rect.left;
        FLOAT caret_y = text_rect.top;
        DWRITE_HIT_TEST_METRICS caret_metrics {};
        if (SUCCEEDED(layout->HitTestTextPosition(static_cast<UINT32>(text_box.caret_position()), FALSE, &caret_x, &caret_y, &caret_metrics))) {
            const float caret_top = clamp_value(text_rect.top + caret_y, text_rect.top + 2.0F, text_rect.bottom - 8.0F);
            const float caret_bottom = clamp_value(caret_top + max_value(14.0F, caret_metrics.height), caret_top + 6.0F, text_rect.bottom - 2.0F);
            draw_editor_caret(context, brush, text_rect.left + caret_x, caret_top, caret_bottom);
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
    const auto& palette = element_palette();
    fill_rounded_rect(context, brush, rect, palette.surface, 10.0F);
    stroke_rounded_rect(
        context,
        brush,
        rect,
        focused ? palette.border_focus : (hovered ? palette.border_hover : palette.border),
        focused ? 2.0F : 1.0F,
        10.0F);

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
                brush->SetColor(D2D1::ColorF(palette.primary.r, palette.primary.g, palette.primary.b, 0.18F));
                for (UINT32 index = 0; index < actual_count; ++index) {
                    const auto& metric = metrics[index];
                    context->FillRectangle(D2D1::RectF(metric.left, metric.top, metric.left + metric.width, metric.top + metric.height), brush);
                }
            }
        }
    }

    brush->SetColor(palette.text_primary);
    context->DrawTextLayout(D2D1::Point2F(text_rect.left, text_rect.top - rich_text_box.scroll_offset()), layout, brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

    if (focused && caret_visible) {
        FLOAT caret_x = text_rect.left;
        FLOAT caret_y = text_rect.top;
        DWRITE_HIT_TEST_METRICS caret_metrics {};
        if (SUCCEEDED(layout->HitTestTextPosition(static_cast<UINT32>(rich_text_box.caret_position()), FALSE, &caret_x, &caret_y, &caret_metrics))) {
            const float caret_top = text_rect.top - rich_text_box.scroll_offset() + caret_y;
            const float caret_bottom = caret_top + max_value(14.0F, caret_metrics.height);
            draw_editor_caret(context, brush, text_rect.left + caret_x, caret_top, caret_bottom);
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

void WindowRenderTarget::set_root_element(const std::shared_ptr<UIElement>& root_element) {
    root_element_ = root_element;
    input_manager_.set_focus_ring_root(root_element_);
}

void WindowRenderTarget::set_interactive_controls(InteractiveControls controls) {
    interactive_controls_ = std::move(controls);
    interactive_mode_enabled_ = interactive_controls_.primary_button != nullptr
        && interactive_controls_.text_box != nullptr
        && interactive_controls_.check_box != nullptr
        && interactive_controls_.combo_box != nullptr
        && interactive_controls_.slider != nullptr;
    focused_control_index_ = interactive_mode_enabled_ && interactive_controls_.text_box != nullptr ? kTextBoxFocusIndex : 0;
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
        apply_dwm_frame(window_host_->hwnd());
        window_host_->refresh_frame();
        return true;
    }

    ready_ = bridge_.bind_target_handle(window_host_->hwnd());
    if (ready_) {
        apply_dwm_frame(window_host_->hwnd());
        window_host_->refresh_frame();
    }
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
        const auto& palette = element_palette();

        const float clear_color[4] {palette.window_background.r, palette.window_background.g, palette.window_background.b, palette.window_background.a};
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
        const std::vector<std::wstring>& items = overlay_scene_.items;
        const OverlayLayout layout = compute_layout(width, height, interactive_controls_, combo_box_scroll_offset_, page_scroll_offset_);
        const HitResult hovered = pointer_inside ? hit_test(layout, pointer_x, pointer_y) : HitResult {};
        const bool caption_minimize_hovered = hovered.target == HitTarget::CaptionMinimize;
        const bool caption_maximize_hovered = hovered.target == HitTarget::CaptionMaximize;
        const bool caption_close_hovered = hovered.target == HitTarget::CaptionClose;
        button_hovered_ = hovered.target == HitTarget::PrimaryButton;
        text_box_hovered_ = hovered.target == HitTarget::TextBox;
        rich_text_box_hovered_ = hovered.target == HitTarget::RichTextBox;
        check_box_hovered_ = hovered.target == HitTarget::CheckBox;
        toggle_switch_hovered_ = hovered.target == HitTarget::ToggleSwitch;
        combo_box_hovered_ = hovered.target == HitTarget::ComboBoxHeader || hovered.target == HitTarget::ComboBoxItem
            || hovered.target == HitTarget::ComboBoxThumb || hovered.target == HitTarget::ComboBoxScrollBar;
        radio_group_hovered_ = hovered.target == HitTarget::RadioGroupOption;
        slider_hovered_ = hovered.target == HitTarget::Slider;
        scroll_viewer_hovered_ = hovered.target == HitTarget::ScrollViewer || hovered.target == HitTarget::ScrollViewerThumb || hovered.target == HitTarget::ScrollViewerScrollBar;
        list_view_hovered_ = hovered.target == HitTarget::ListView || hovered.target == HitTarget::ListViewThumb || hovered.target == HitTarget::ListViewScrollBar;
        items_control_hovered_ = hovered.target == HitTarget::ItemsControl || hovered.target == HitTarget::ItemsControlThumb || hovered.target == HitTarget::ItemsControlScrollBar;
        log_box_hovered_ = hovered.target == HitTarget::LogBox || hovered.target == HitTarget::LogBoxThumb || hovered.target == HitTarget::LogBoxScrollBar;
        hovered_combo_index_ = hovered.target == HitTarget::ComboBoxItem ? hovered.combo_index : -1;
        hovered_radio_index_ = hovered.target == HitTarget::RadioGroupOption ? hovered.radio_index : -1;
        hovered_item_index_ = hovered_combo_index_;

        if (d2d_context_ == nullptr || d2d_target_bitmap_ == nullptr || d2d_brush_ == nullptr) {
            render_manager_->diagnostics().log(LogLevel::Error, "D2D overlay context unavailable");
            return false;
        }

            d2d_context_->BeginDraw();
            d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());

            const D2D1_RECT_F card_rect = interactive_mode_enabled_
                ? layout.card
                : D2D1::RectF(width * 0.1F, height * 0.1F, width * 0.9F, height * 0.65F);
            if (interactive_mode_enabled_) {
                fill_rounded_rect(d2d_context_, d2d_brush_, card_rect, palette.window_background, 0.0F);
                fill_rounded_rect(d2d_context_, d2d_brush_, layout.title_band, palette.title_background, 0.0F);
                d2d_brush_->SetColor(palette.title_separator);
                d2d_context_->DrawLine(
                    D2D1::Point2F(layout.title_band.left, layout.title_band.bottom - 0.5F),
                    D2D1::Point2F(layout.title_band.right, layout.title_band.bottom - 0.5F),
                    d2d_brush_,
                    1.0F);

                fill_rounded_rect(d2d_context_, d2d_brush_, layout.caption_icon, palette.primary, 10.0F);
                draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"F", layout.caption_icon, palette.text_inverse, DWRITE_TEXT_ALIGNMENT_CENTER);
                draw_caption_button(d2d_context_, d2d_brush_, item_text_format_, layout.caption_minimize, L"-", caption_minimize_hovered, false);
                draw_caption_button(
                    d2d_context_,
                    d2d_brush_,
                    item_text_format_,
                    layout.caption_maximize,
                    window_host_->window_state() == WindowState::Maximized ? L"[]" : L"O",
                    caption_maximize_hovered,
                    false);
                draw_caption_button(d2d_context_, d2d_brush_, item_text_format_, layout.caption_close, L"X", caption_close_hovered, false, true);
            } else {
                d2d_brush_->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F));
                const D2D1_ROUNDED_RECT card = D2D1::RoundedRect(card_rect, 4.0F, 4.0F);
                d2d_context_->FillRoundedRectangle(card, d2d_brush_);
                d2d_brush_->SetColor(D2D1::ColorF(0.89F, 0.91F, 0.94F, 1.0F));
                d2d_context_->DrawRoundedRectangle(card, d2d_brush_, 1.0F);
            }

            d2d_brush_->SetColor(palette.text_primary);
            if (text_format_ != nullptr) {
                const std::wstring title = interactive_mode_enabled_ ? L"DCompFrame Interactive Demo" : overlay_scene_.title;
                const D2D1_RECT_F title_rect = interactive_mode_enabled_
                    ? layout.title
                    : D2D1::RectF(width * 0.13F, height * 0.14F, width * 0.85F, height * 0.22F);
                if (interactive_mode_enabled_) {
                    draw_text_line(d2d_context_, d2d_brush_, text_format_, title, title_rect, palette.text_primary);
                } else {
                    draw_centered_text(d2d_context_, d2d_brush_, text_format_, title, title_rect);
                }
            }
            if (interactive_mode_enabled_ && item_text_format_ != nullptr) {
                draw_text_line(
                    d2d_context_,
                    d2d_brush_,
                    item_text_format_,
                    L"Flex-only layout | custom caption | DWM-preserved window behavior",
                    layout.subtitle,
                        palette.text_secondary);
            }

            d2d_context_->PushAxisAlignedClip(layout.content_clip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            if (interactive_mode_enabled_ && interactive_controls_.text_box != nullptr && interactive_controls_.check_box != nullptr
                && interactive_controls_.combo_box != nullptr && interactive_controls_.slider != nullptr
                && interactive_controls_.primary_button != nullptr) {
                D2D1_COLOR_F button_color = palette.primary;
                if (button_pressed_) {
                    button_color = palette.primary_pressed;
                } else if (button_hovered_) {
                    button_color = palette.primary_hover;
                }
                fill_rounded_rect(d2d_context_, d2d_brush_, layout.button, button_color, 10.0F);
                if (focused_control_index_ == kPrimaryButtonFocusIndex) {
                    stroke_rounded_rect(d2d_context_, d2d_brush_, inset_rect(layout.button, -2.0F, -2.0F), palette.border_focus, 2.0F, 10.0F);
                }
                if (item_text_format_ != nullptr) {
                    d2d_brush_->SetColor(palette.text_inverse);
                    draw_centered_text(
                        d2d_context_,
                        d2d_brush_,
                        item_text_format_,
                        utf8_to_wstring(interactive_controls_.primary_button->text()),
                        layout.button);
                }

                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"TextBox", layout.text_box_label, palette.text_secondary);

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
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"RichTextBox", layout.rich_text_label, palette.text_secondary);
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

                fill_rounded_rect(d2d_context_, d2d_brush_, layout.check_box, palette.surface, 10.0F);
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.check_box, focused_control_index_ == kCheckBoxFocusIndex ? palette.border_focus : (check_box_hovered_ ? palette.border_hover : palette.border), focused_control_index_ == kCheckBoxFocusIndex ? 2.0F : 1.0F, 10.0F);
                draw_checkbox_glyph(d2d_context_, d2d_brush_, layout.check_box, interactive_controls_.check_box->checked());
                draw_text_line(
                    d2d_context_,
                    d2d_brush_,
                    item_text_format_,
                    interactive_controls_.check_box->checked() ? L"启用 Element Plus 表单增强行为" : L"关闭 Element Plus 表单增强行为",
                    D2D1::RectF(layout.check_box.left + 50.0F, layout.check_box.top, layout.check_box.right - 16.0F, layout.check_box.bottom),
                    palette.text_primary);

                if (interactive_controls_.toggle_switch != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"ToggleSwitch", layout.toggle_label, palette.text_secondary);
                    draw_toggle_switch_control(
                        d2d_context_,
                        d2d_brush_,
                        item_text_format_,
                        layout.toggle_switch,
                        interactive_controls_.toggle_switch->checked(),
                        toggle_switch_hovered_,
                        focused_control_index_ == kToggleSwitchFocusIndex);
                }

                draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"ComboBox", layout.combo_label, palette.text_secondary);
                fill_rounded_rect(d2d_context_, d2d_brush_, layout.combo_box, palette.surface, 8.0F);
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.combo_box, focused_control_index_ == kComboBoxFocusIndex ? palette.border_focus : (combo_box_hovered_ ? palette.border_hover : palette.border), focused_control_index_ == kComboBoxFocusIndex ? 2.0F : 1.0F, 8.0F);
                draw_combobox_glyph(d2d_context_, d2d_brush_, layout.combo_box);
                draw_text_line(
                    d2d_context_,
                    d2d_brush_,
                    item_text_format_,
                    utf8_to_wstring(interactive_controls_.combo_box->selected_text()),
                    D2D1::RectF(layout.combo_box.left + 14.0F, layout.combo_box.top, layout.combo_box.right - 42.0F, layout.combo_box.bottom),
                    palette.text_primary);

                if (interactive_controls_.radio_group != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"RadioGroup", layout.radio_label, palette.text_secondary);
                    draw_radio_group_control(
                        d2d_context_,
                        d2d_brush_,
                        item_text_format_,
                        *interactive_controls_.radio_group,
                        layout,
                        hovered_radio_index_,
                        focused_control_index_ == kRadioGroupFocusIndex);
                }

                draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"Slider", layout.slider_label, palette.text_secondary);
                fill_rounded_rect(d2d_context_, d2d_brush_, layout.slider, palette.surface_alt, 10.0F);
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.slider, focused_control_index_ == kSliderFocusIndex ? palette.border_focus : (slider_hovered_ ? palette.border_hover : palette.border), focused_control_index_ == kSliderFocusIndex ? 2.0F : 1.0F, 10.0F);
                draw_slider_glyph(d2d_context_, d2d_brush_, layout.slider_track, interactive_controls_.slider->normalized_value());
                fill_rounded_rect(d2d_context_, d2d_brush_, D2D1::RectF(layout.slider.right - 58.0F, layout.slider.top + 6.0F, layout.slider.right - 10.0F, layout.slider.bottom - 6.0F), palette.primary_soft, 12.0F);
                draw_text_line(
                    d2d_context_,
                    d2d_brush_,
                    item_text_format_,
                    utf8_to_wstring(std::to_string(static_cast<int>(interactive_controls_.slider->value())) + "%"),
                    D2D1::RectF(layout.slider.right - 58.0F, layout.slider.top + 4.0F, layout.slider.right - 10.0F, layout.slider.bottom - 4.0F),
                    palette.primary,
                    DWRITE_TEXT_ALIGNMENT_CENTER);

                if (interactive_controls_.text_block != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"TextBlock", layout.text_block_label, palette.text_secondary);
                    fill_rounded_rect(d2d_context_, d2d_brush_, layout.text_block, palette.surface_alt, 10.0F);
                    stroke_rounded_rect(d2d_context_, d2d_brush_, layout.text_block, palette.border, 1.0F, 10.0F);
                    draw_wrapped_text(
                        d2d_context_,
                        d2d_brush_,
                        dwrite_factory_,
                        helper_text_format_ != nullptr ? helper_text_format_ : item_text_format_,
                        utf8_to_wstring(interactive_controls_.text_block->text()),
                        inset_rect(layout.text_block, 12.0F, 10.0F),
                        palette.text_primary);
                    if (interactive_controls_.label != nullptr) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.label_chip, palette.primary_soft, 12.0F);
                        draw_text_line(
                            d2d_context_,
                            d2d_brush_,
                            item_text_format_,
                            utf8_to_wstring(interactive_controls_.label->text()),
                            layout.label_chip,
                            palette.primary,
                            DWRITE_TEXT_ALIGNMENT_CENTER);
                    }
                    if (interactive_controls_.badge != nullptr) {
                        draw_badge_control(d2d_context_, d2d_brush_, item_text_format_, *interactive_controls_.badge, layout.badge);
                    }
                }

                if (interactive_controls_.divider != nullptr) {
                    draw_divider_control(d2d_context_, d2d_brush_, *interactive_controls_.divider, layout.divider);
                }

                draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"Image", layout.image_label, palette.text_secondary);
                draw_image_placeholder(d2d_context_, d2d_brush_, item_text_format_, layout.image);

                if (interactive_controls_.card != nullptr) {
                    fill_rounded_rect(d2d_context_, d2d_brush_, layout.card_preview, palette.surface, 14.0F);
                    stroke_rounded_rect(d2d_context_, d2d_brush_, layout.card_preview, palette.border, 1.0F, 14.0F);
                    fill_rounded_rect(d2d_context_, d2d_brush_, layout.preview_header, palette.surface_alt, 12.0F);
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, utf8_to_wstring(interactive_controls_.card->title()), D2D1::RectF(layout.preview_header.left + 12.0F, layout.preview_header.top + 8.0F, layout.preview_header.right - 12.0F, layout.preview_header.top + 28.0F), palette.text_primary);
                    draw_wrapped_text(d2d_context_, d2d_brush_, dwrite_factory_, helper_text_format_ != nullptr ? helper_text_format_ : item_text_format_, utf8_to_wstring(interactive_controls_.card->body()), D2D1::RectF(layout.preview_header.left + 12.0F, layout.preview_header.top + 28.0F, layout.preview_header.right - 12.0F, layout.preview_header.bottom - 8.0F), palette.text_secondary);

                    if (interactive_controls_.tab_control != nullptr && !interactive_controls_.tab_control->tabs().empty()) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.tab_control, palette.surface_alt, 12.0F);
                        const float tab_width = rect_width(layout.tab_control) / static_cast<float>(interactive_controls_.tab_control->tabs().size());
                        for (std::size_t tab_index = 0; tab_index < interactive_controls_.tab_control->tabs().size(); ++tab_index) {
                            const D2D1_RECT_F tab_rect = D2D1::RectF(
                                layout.tab_control.left + tab_width * static_cast<float>(tab_index),
                                layout.tab_control.top,
                                layout.tab_control.left + tab_width * static_cast<float>(tab_index + 1U),
                                layout.tab_control.bottom);
                            const bool selected = interactive_controls_.tab_control->selected_index().has_value()
                                && *interactive_controls_.tab_control->selected_index() == tab_index;
                            fill_rounded_rect(
                                d2d_context_,
                                d2d_brush_,
                                inset_rect(tab_rect, 2.0F, 2.0F),
                                selected ? palette.primary : (hovered.target == HitTarget::TabControl ? palette.primary_soft_hover : palette.surface),
                                10.0F);
                            draw_text_line(
                                d2d_context_,
                                d2d_brush_,
                                item_text_format_,
                                utf8_to_wstring(interactive_controls_.tab_control->tabs()[tab_index]),
                                tab_rect,
                                selected ? palette.text_inverse : palette.text_secondary,
                                DWRITE_TEXT_ALIGNMENT_CENTER);
                        }

                        const std::string selected_tab = interactive_controls_.tab_control->selected_tab();
                        draw_wrapped_text(
                            d2d_context_,
                            d2d_brush_,
                            dwrite_factory_,
                            helper_text_format_ != nullptr ? helper_text_format_ : item_text_format_,
                            utf8_to_wstring(selected_tab.empty() ? "TabControl 示例" : std::string("当前页签: ") + selected_tab + "，用于演示切换后的正文与动画联动。"),
                            layout.tab_body,
                            palette.text_secondary);

                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.animation_demo, palette.primary_soft, 8.0F);
                        const bool animation_active = (interactive_controls_.loading != nullptr && interactive_controls_.loading->active())
                            || (interactive_controls_.progress != nullptr && interactive_controls_.progress->is_indeterminate());
                        const float animation_phase = animation_active ? static_cast<float>((GetTickCount64() % 1800ULL)) / 1800.0F : 0.0F;
                        const float pulse_width = max_value(28.0F, rect_width(layout.animation_demo) * 0.18F);
                        const float pulse_left = layout.animation_demo.left + (rect_width(layout.animation_demo) - pulse_width) * animation_phase;
                        fill_rounded_rect(
                            d2d_context_,
                            d2d_brush_,
                            D2D1::RectF(pulse_left, layout.animation_demo.top + 3.0F, pulse_left + pulse_width, layout.animation_demo.bottom - 3.0F),
                            D2D1::ColorF(palette.primary.r, palette.primary.g, palette.primary.b, 0.82F),
                            5.0F);
                        draw_text_line(
                            d2d_context_,
                            d2d_brush_,
                            item_text_format_,
                            L"Animation Demo",
                            D2D1::RectF(layout.animation_demo.left + 10.0F, layout.animation_demo.top, layout.animation_demo.right - 10.0F, layout.animation_demo.bottom),
                            palette.text_primary);
                    }

                    if (interactive_controls_.expander != nullptr) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.expander_header, palette.surface_alt, 10.0F);
                        draw_text_line(
                            d2d_context_,
                            d2d_brush_,
                            item_text_format_,
                            utf8_to_wstring(std::string(interactive_controls_.expander->expanded() ? "▼ " : "▶ ") + interactive_controls_.expander->header()),
                            D2D1::RectF(layout.expander_header.left + 8.0F, layout.expander_header.top, layout.expander_header.right - 8.0F, layout.expander_header.bottom),
                            palette.text_secondary);
                        if (interactive_controls_.expander->expanded()) {
                            fill_rounded_rect(d2d_context_, d2d_brush_, layout.expander_body, palette.surface, 10.0F);
                            draw_wrapped_text(
                                d2d_context_,
                                d2d_brush_,
                                dwrite_factory_,
                                helper_text_format_ != nullptr ? helper_text_format_ : item_text_format_,
                                utf8_to_wstring(interactive_controls_.expander->content_text()),
                                inset_rect(layout.expander_body, 8.0F, 8.0F),
                                palette.text_secondary);
                        }
                    }

                    if (interactive_controls_.progress != nullptr) {
                        draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"Progress", layout.progress_label, D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F));
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.progress_decrease, D2D1::ColorF(0.95F, 0.96F, 0.98F, 1.0F), 10.0F);
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.progress_increase, D2D1::ColorF(0.95F, 0.96F, 0.98F, 1.0F), 10.0F);
                        draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"-", layout.progress_decrease, D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F), DWRITE_TEXT_ALIGNMENT_CENTER);
                        draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"+", layout.progress_increase, D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F), DWRITE_TEXT_ALIGNMENT_CENTER);
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.progress_bar, D2D1::ColorF(0.94F, 0.95F, 0.97F, 1.0F), 4.0F);
                        const float ratio = interactive_controls_.progress->is_indeterminate()
                            ? static_cast<float>((GetTickCount64() / 160ULL) % 10ULL) / 10.0F
                            : interactive_controls_.progress->normalized_value();
                        const float fill_right = layout.progress_bar.left + rect_width(layout.progress_bar) * clamp_value(ratio, 0.0F, 1.0F);
                        fill_rounded_rect(
                            d2d_context_,
                            d2d_brush_,
                            D2D1::RectF(layout.progress_bar.left, layout.progress_bar.top, fill_right, layout.progress_bar.bottom),
                            D2D1::ColorF(0.25F, 0.62F, 1.0F, 1.0F),
                            4.0F);
                    }

                    if (interactive_controls_.loading != nullptr) {
                        fill_rounded_rect(
                            d2d_context_,
                            d2d_brush_,
                            layout.loading_badge,
                            interactive_controls_.loading->active() ? D2D1::ColorF(0.90F, 0.96F, 0.90F, 1.0F) : D2D1::ColorF(0.95F, 0.96F, 0.98F, 1.0F),
                            6.0F);
                        draw_text_line(
                            d2d_context_,
                            d2d_brush_,
                            item_text_format_,
                            utf8_to_wstring(interactive_controls_.loading->active() ? std::string("Loading: ") + interactive_controls_.loading->text() : std::string("Loading: Idle")),
                            D2D1::RectF(layout.loading_badge.left + 6.0F, layout.loading_badge.top, layout.loading_badge.right - 6.0F, layout.loading_badge.bottom),
                            D2D1::ColorF(0.38F, 0.40F, 0.43F, 1.0F));
                    }

                    if (interactive_controls_.popup != nullptr) {
                        stroke_rounded_rect(d2d_context_, d2d_brush_, layout.popup_preview, D2D1::ColorF(0.84F, 0.88F, 0.95F, 1.0F), 1.0F, 5.0F);
                        if (interactive_controls_.popup->is_open()) {
                            fill_rounded_rect(d2d_context_, d2d_brush_, layout.popup_preview, D2D1::ColorF(1.0F, 1.0F, 1.0F, 0.98F), 5.0F);
                            draw_text_line(
                                d2d_context_,
                                d2d_brush_,
                                item_text_format_,
                                utf8_to_wstring(interactive_controls_.popup->title()),
                                D2D1::RectF(layout.popup_preview.left + 6.0F, layout.popup_preview.top + 2.0F, layout.popup_preview.right - 6.0F, layout.popup_preview.bottom - 2.0F),
                                D2D1::ColorF(0.19F, 0.20F, 0.22F, 1.0F));
                        } else {
                            draw_text_line(
                                d2d_context_,
                                d2d_brush_,
                                item_text_format_,
                                L"Popup(Closed)",
                                layout.popup_preview,
                                D2D1::ColorF(0.56F, 0.58F, 0.62F, 1.0F),
                                DWRITE_TEXT_ALIGNMENT_CENTER);
                        }
                    }
                }

                if (interactive_controls_.list_view != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"ListView", layout.list_view_label, palette.text_secondary);
                    if (rect_has_area(layout.list_view_scrollbar)) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.list_view_scrollbar, palette.scrollbar_track, 4.0F);
                    }
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
                            fill_rounded_rect(d2d_context_, d2d_brush_, item_rect, palette.primary_soft, 8.0F);
                        }
                        draw_text_line(d2d_context_, d2d_brush_, item_text_format_, utf8_to_wstring(interactive_controls_.list_view->items()[index]), D2D1::RectF(item_rect.left + 8.0F, item_rect.top, item_rect.right - 8.0F, item_rect.bottom), selected ? palette.primary : palette.text_primary);
                    }
                    d2d_context_->PopAxisAlignedClip();
                    if (rect_has_area(layout.list_view_thumb)) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.list_view_thumb, focused_scroll_target_ == DragScrollTarget::ListView ? palette.primary : (list_view_hovered_ ? palette.scrollbar_thumb_hover : palette.scrollbar_thumb), 4.0F);
                    }
                }

                if (interactive_controls_.items_control != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"ItemsControl", layout.items_control_label, palette.text_secondary);
                    if (rect_has_area(layout.items_control_scrollbar)) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.items_control_scrollbar, palette.scrollbar_track, 4.0F);
                    }
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
                        fill_rounded_rect(d2d_context_, d2d_brush_, tag_rect, selected ? palette.primary : palette.surface_active, 10.0F);
                        draw_text_line(d2d_context_, d2d_brush_, item_text_format_, utf8_to_wstring(item), D2D1::RectF(tag_rect.left + 10.0F, tag_rect.top, tag_rect.right - 10.0F, tag_rect.bottom), selected ? palette.text_inverse : palette.text_secondary);
                    }
                    d2d_context_->PopAxisAlignedClip();
                    if (rect_has_area(layout.items_control_thumb)) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.items_control_thumb, focused_scroll_target_ == DragScrollTarget::ItemsControl ? palette.primary : (items_control_hovered_ ? palette.scrollbar_thumb_hover : palette.scrollbar_thumb), 4.0F);
                    }
                }

                if (interactive_controls_.scroll_viewer != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"ScrollViewer", layout.scroll_viewer_label, palette.text_secondary);
                    if (rect_has_area(layout.scroll_viewer_scrollbar)) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.scroll_viewer_scrollbar, palette.scrollbar_track, 4.0F);
                    }

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
                            fill_rounded_rect(d2d_context_, d2d_brush_, item_rect, selected ? palette.primary_soft : palette.surface, 8.0F);
                            stroke_rounded_rect(d2d_context_, d2d_brush_, item_rect, palette.border, 1.0F, 8.0F);
                            draw_text_line(d2d_context_, d2d_brush_, item_text_format_, utf8_to_wstring(content->items()[index]), D2D1::RectF(item_rect.left + 10.0F, item_rect.top, item_rect.right - 10.0F, item_rect.bottom), palette.text_primary);
                        }
                    }
                    d2d_context_->PopAxisAlignedClip();
                    if (rect_has_area(layout.scroll_viewer_thumb)) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.scroll_viewer_thumb, focused_scroll_target_ == DragScrollTarget::ScrollViewer ? palette.primary : (scroll_viewer_hovered_ ? palette.scrollbar_thumb_hover : palette.scrollbar_thumb), 4.0F);
                    }
                }

                if (interactive_controls_.log_box != nullptr) {
                    draw_text_line(d2d_context_, d2d_brush_, item_text_format_, L"LogBox", layout.footer_label, palette.text_secondary);
                    if (rect_has_area(layout.footer_scrollbar)) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.footer_scrollbar, palette.scrollbar_track, 4.0F);
                    }
                    const float line_height = 20.0F;
                    const float line_stride = 24.0F;
                    const float scroll_offset = interactive_controls_.log_box->scroll_offset();
                    const std::size_t begin = static_cast<std::size_t>(max_value(0.0F, scroll_offset) / line_stride);
                    const std::size_t visible_count = static_cast<std::size_t>(std::ceil(rect_height(layout.footer_viewport) / line_stride)) + 1U;
                    const std::size_t end = min_value(interactive_controls_.log_box->lines().size(), begin + visible_count);
                    const float content_right = rect_has_area(layout.footer_thumb) ? layout.footer_thumb.left - 8.0F : layout.footer_viewport.right;
                    d2d_context_->PushAxisAlignedClip(layout.footer_viewport, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                    for (std::size_t index = begin; index < end; ++index) {
                        const float line_top = layout.footer_viewport.top + static_cast<float>(index) * line_stride - scroll_offset;
                        const D2D1_RECT_F line_rect = D2D1::RectF(layout.footer_viewport.left, line_top, content_right, line_top + line_height);
                        draw_text_line(d2d_context_, d2d_brush_, helper_text_format_ != nullptr ? helper_text_format_ : item_text_format_, utf8_to_wstring(interactive_controls_.log_box->lines()[index]), line_rect, palette.text_secondary);
                    }
                    d2d_context_->PopAxisAlignedClip();
                    if (rect_has_area(layout.footer_thumb)) {
                        fill_rounded_rect(d2d_context_, d2d_brush_, layout.footer_thumb, focused_scroll_target_ == DragScrollTarget::LogBox ? palette.primary : (log_box_hovered_ ? palette.scrollbar_thumb_hover : palette.scrollbar_thumb), 4.0F);
                    }
                }
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
                fill_rounded_rect(d2d_context_, d2d_brush_, layout.combo_dropdown, palette.surface, 12.0F);
                stroke_rounded_rect(d2d_context_, d2d_brush_, layout.combo_dropdown, palette.border_hover, 1.0F, 12.0F);
                if (rect_has_area(layout.combo_dropdown_scrollbar)) {
                    fill_rounded_rect(
                        d2d_context_,
                        d2d_brush_,
                        layout.combo_dropdown_scrollbar,
                        palette.scrollbar_track,
                        4.0F);
                }
                d2d_context_->PushAxisAlignedClip(layout.combo_dropdown_viewport, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                for (std::size_t index = 0; index < layout.combo_items.size(); ++index) {
                    const float center_y = (layout.combo_items[index].top + layout.combo_items[index].bottom) * 0.5F;
                    if (center_y < layout.combo_dropdown_viewport.top || center_y > layout.combo_dropdown_viewport.bottom) {
                        continue;
                    }
                    const bool item_hovered = hovered_combo_index_ == static_cast<int>(index);
                    const bool selected = interactive_controls_.combo_box->selected_index() && *interactive_controls_.combo_box->selected_index() == index;
                    const D2D1_RECT_F item_rect = inset_rect(layout.combo_items[index], 4.0F, 3.0F);
                    fill_rounded_rect(
                        d2d_context_,
                        d2d_brush_,
                        item_rect,
                        selected ? palette.primary_soft : (item_hovered ? palette.primary_soft_hover : palette.surface),
                        8.0F);
                    draw_text_line(
                        d2d_context_,
                        d2d_brush_,
                        item_text_format_,
                        utf8_to_wstring(interactive_controls_.combo_box->items()[index]),
                        D2D1::RectF(item_rect.left + 10.0F, item_rect.top, item_rect.right - 28.0F, item_rect.bottom),
                        selected ? palette.primary : palette.text_primary);
                }
                d2d_context_->PopAxisAlignedClip();
                if (rect_has_area(layout.combo_dropdown_thumb)) {
                    fill_rounded_rect(
                        d2d_context_,
                        d2d_brush_,
                        layout.combo_dropdown_thumb,
                        focused_scroll_target_ == DragScrollTarget::ComboBox ? palette.primary : palette.scrollbar_thumb,
                        4.0F);
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
    const OverlayLayout layout = compute_layout(size.width > 0.0F ? size.width : 1280.0F, size.height > 0.0F ? size.height : 720.0F, interactive_controls_, combo_box_scroll_offset_, page_scroll_offset_);
    const auto is_focus_available = [&](std::size_t index) {
        switch (index) {
        case kPrimaryButtonFocusIndex:
            return interactive_controls_.primary_button != nullptr;
        case kTextBoxFocusIndex:
            return interactive_controls_.text_box != nullptr;
        case kRichTextBoxFocusIndex:
            return interactive_controls_.rich_text_box != nullptr;
        case kCheckBoxFocusIndex:
            return interactive_controls_.check_box != nullptr;
        case kToggleSwitchFocusIndex:
            return interactive_controls_.toggle_switch != nullptr;
        case kComboBoxFocusIndex:
            return interactive_controls_.combo_box != nullptr;
        case kRadioGroupFocusIndex:
            return interactive_controls_.radio_group != nullptr;
        case kSliderFocusIndex:
            return interactive_controls_.slider != nullptr;
        default:
            return false;
        }
    };
    const auto update_focus = [&](std::size_t index) {
        if (!is_focus_available(index)) {
            return;
        }
        focused_control_index_ = min_value(index, kInteractiveFocusCount - 1U);
        sync_focus_state();
        reset_caret_blink();
        window_host_->request_render();
    };
    const auto focus_next = [&](bool reverse) {
        const std::size_t count = kInteractiveFocusCount;
        for (std::size_t step = 0; step < count; ++step) {
            const std::size_t candidate = reverse
                ? (focused_control_index_ + count - 1U - step) % count
                : (focused_control_index_ + 1U + step) % count;
            if (is_focus_available(candidate)) {
                focused_control_index_ = candidate;
                break;
            }
        }
        sync_focus_state();
        reset_caret_blink();
        window_host_->request_render();
    };
    const auto update_slider_from_x = [&](float x) {
        if (interactive_controls_.slider == nullptr || !rect_has_area(layout.slider_track)) {
            return;
        }
        const float track_width = max_value(1.0F, rect_width(layout.slider_track));
        const float ratio = clamp_value((x - layout.slider_track.left) / track_width, 0.0F, 1.0F);
        interactive_controls_.slider->set_value_from_ratio(ratio);
        window_host_->request_render();
    };
    const auto clamp_combo_box_offset = [&](float candidate) {
        if (interactive_controls_.combo_box == nullptr) {
            return 0.0F;
        }

        const float total_height = compute_combo_dropdown_total_height(interactive_controls_.combo_box->items().size(), 36.0F);
        return clamp_value(candidate, 0.0F, max_value(0.0F, total_height - rect_height(layout.combo_dropdown_viewport)));
    };
    const auto clamp_page_offset = [&](float candidate) {
        const float viewport_height = max_value(1.0F, rect_height(layout.content_clip));
        const auto control_bottom = [](const std::shared_ptr<UIElement>& element) {
            if (element == nullptr) {
                return 0.0F;
            }
            const Rect bounds = element->absolute_bounds();
            return bounds.y + bounds.height;
        };
        const float content_bottom = max_value(
            control_bottom(interactive_controls_.log_box),
            max_value(
                control_bottom(interactive_controls_.scroll_viewer),
                max_value(control_bottom(interactive_controls_.items_control), control_bottom(interactive_controls_.popup))));
        const float virtual_height = max_value(viewport_height, content_bottom - layout.content_clip.top + 24.0F);
        return clamp_value(candidate, 0.0F, max_value(0.0F, virtual_height - viewport_height));
    };
    const auto ensure_combo_selection_visible = [&]() {
        if (interactive_controls_.combo_box == nullptr || !interactive_controls_.combo_box->selected_index().has_value()) {
            return;
        }

        const float item_top = static_cast<float>(*interactive_controls_.combo_box->selected_index()) * 36.0F;
        const float item_bottom = item_top + 36.0F;
        if (item_top < combo_box_scroll_offset_) {
            combo_box_scroll_offset_ = clamp_combo_box_offset(item_top);
        } else if (item_bottom > combo_box_scroll_offset_ + rect_height(layout.combo_dropdown_viewport)) {
            combo_box_scroll_offset_ = clamp_combo_box_offset(item_bottom - rect_height(layout.combo_dropdown_viewport));
        }
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
    const auto clamp_log_box_offset = [&](float candidate) {
        if (interactive_controls_.log_box == nullptr) {
            return 0.0F;
        }
        const float total_height = compute_items_total_height(interactive_controls_.log_box->lines().size(), 20.0F, 4.0F);
        return clamp_value(candidate, 0.0F, max_value(0.0F, total_height - rect_height(layout.footer_viewport)));
    };
    const auto update_drag_scroll = [&](float y) {
        switch (drag_scroll_target_) {
        case DragScrollTarget::ComboBox: {
            if (!rect_has_area(layout.combo_dropdown_thumb)) {
                return;
            }
            if (interactive_controls_.combo_box == nullptr) {
                return;
            }
            const float total_height = compute_combo_dropdown_total_height(interactive_controls_.combo_box->items().size(), 36.0F);
            const float max_offset = max_value(0.0F, total_height - rect_height(layout.combo_dropdown_viewport));
            const float travel = max_value(1.0F, rect_height(layout.combo_dropdown_viewport) - rect_height(layout.combo_dropdown_thumb));
            combo_box_scroll_offset_ = clamp_combo_box_offset(drag_scroll_origin_offset_ + (y - drag_scroll_anchor_y_) * (max_offset / travel));
            break;
        }
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
        case DragScrollTarget::LogBox: {
            if (!rect_has_area(layout.footer_thumb)) {
                return;
            }
            const float max_offset = clamp_log_box_offset(100000.0F);
            const float travel = max_value(1.0F, rect_height(layout.footer_viewport) - rect_height(layout.footer_thumb));
            interactive_controls_.log_box->set_scroll_offset(clamp_value(drag_scroll_origin_offset_ + (y - drag_scroll_anchor_y_) * (max_offset / travel), 0.0F, max_offset));
            break;
        }
        case DragScrollTarget::None:
            return;
        }
        window_host_->request_render();
    };
    const auto begin_track_drag = [&](DragScrollTarget target, float y, float new_offset) {
        drag_scroll_target_ = target;
        focused_scroll_target_ = target;
        drag_scroll_anchor_y_ = y;
        drag_scroll_origin_offset_ = new_offset;
        window_host_->request_render();
    };

    switch (msg) {
    case WM_DWMCOMPOSITIONCHANGED:
        apply_dwm_frame(window_host_->hwnd());
        window_host_->refresh_frame();
        return false;
    case WM_ACTIVATE:
        apply_dwm_frame(window_host_->hwnd());
        window_host_->request_render();
        return false;
    case WM_NCACTIVATE:
        apply_dwm_frame(window_host_->hwnd());
        window_host_->request_render();
        result = TRUE;
        return true;
    case WM_NCUAHDRAWCAPTION:
    case WM_NCUAHDRAWFRAME:
        result = 0;
        return true;
    case WM_NCMOUSEMOVE:
    case WM_NCMOUSELEAVE: {
        LRESULT dwm_result = 0;
        DwmDefWindowProc(window_host_->hwnd(), msg, wparam, lparam, &dwm_result);
        window_host_->request_render();
        return false;
    }
    case WM_NCCALCSIZE:
        if (wparam == TRUE) {
            result = 0;
            return true;
        }
        return false;
    case WM_NCHITTEST: {
        LRESULT dwm_result = 0;
        if (DwmDefWindowProc(window_host_->hwnd(), msg, wparam, lparam, &dwm_result)) {
            if (dwm_result != HTMINBUTTON && dwm_result != HTMAXBUTTON && dwm_result != HTCLOSE) {
                result = dwm_result;
                return true;
            }
        }

        POINT screen_point {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        POINT client_point = screen_point;
        ScreenToClient(window_host_->hwnd(), &client_point);
        const float x = static_cast<float>(client_point.x);
        const float y = static_cast<float>(client_point.y);

        if (point_in_rect(layout.caption_close, x, y)) {
            result = HTCLOSE;
            return true;
        }
        if (point_in_rect(layout.caption_maximize, x, y)) {
            result = HTMAXBUTTON;
            return true;
        }
        if (point_in_rect(layout.caption_minimize, x, y)) {
            result = HTMINBUTTON;
            return true;
        }
        if (point_in_rect(layout.caption_icon, x, y)) {
            result = HTSYSMENU;
            return true;
        }

        if (window_host_->window_state() != WindowState::Maximized && window_host_->window_state() != WindowState::Fullscreen) {
            const unsigned int dpi = current_window_dpi(window_host_->hwnd());
            const float resize_border_x = static_cast<float>(resize_frame_thickness_x(dpi));
            const float resize_border_y = static_cast<float>(resize_frame_thickness_y(dpi));
            const bool on_left = x >= 0.0F && x < resize_border_x;
            const bool on_right = x <= size.width && x > size.width - resize_border_x;
            const bool on_top = y >= 0.0F && y < resize_border_y;
            const bool on_bottom = y <= size.height && y > size.height - resize_border_y;

            if (on_top && on_left) {
                result = HTTOPLEFT;
                return true;
            }
            if (on_top && on_right) {
                result = HTTOPRIGHT;
                return true;
            }
            if (on_bottom && on_left) {
                result = HTBOTTOMLEFT;
                return true;
            }
            if (on_bottom && on_right) {
                result = HTBOTTOMRIGHT;
                return true;
            }
            if (on_left) {
                result = HTLEFT;
                return true;
            }
            if (on_right) {
                result = HTRIGHT;
                return true;
            }
            if (on_top) {
                result = HTTOP;
                return true;
            }
            if (on_bottom) {
                result = HTBOTTOM;
                return true;
            }
        }

        if (y >= layout.title_band.top && y <= layout.title_band.bottom && x >= layout.title.left && x <= layout.caption_minimize.left - 8.0F) {
            result = HTCAPTION;
            return true;
        }

        result = HTCLIENT;
        return true;
    }
    case WM_GETDLGCODE:
        result = DLGC_WANTARROWS | DLGC_WANTCHARS | DLGC_WANTTAB;
        return true;
    case WM_SETFOCUS:
        sync_focus_state();
        reset_caret_blink();
        result = 0;
        return true;
    case WM_KILLFOCUS:
    case WM_CAPTURECHANGED:
    case WM_CANCELMODE:
        if (drag_scroll_target_ != DragScrollTarget::None || focused_scroll_target_ != DragScrollTarget::None) {
            focused_scroll_target_ = DragScrollTarget::None;
            window_host_->request_render();
        }
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
        toggle_switch_hovered_ = hit.target == HitTarget::ToggleSwitch;
        combo_box_hovered_ = hit.target == HitTarget::ComboBoxHeader || hit.target == HitTarget::ComboBoxItem
            || hit.target == HitTarget::ComboBoxThumb || hit.target == HitTarget::ComboBoxScrollBar;
        radio_group_hovered_ = hit.target == HitTarget::RadioGroupOption;
        slider_hovered_ = hit.target == HitTarget::Slider;
        scroll_viewer_hovered_ = hit.target == HitTarget::ScrollViewer || hit.target == HitTarget::ScrollViewerThumb || hit.target == HitTarget::ScrollViewerScrollBar;
        list_view_hovered_ = hit.target == HitTarget::ListView || hit.target == HitTarget::ListViewThumb || hit.target == HitTarget::ListViewScrollBar;
        items_control_hovered_ = hit.target == HitTarget::ItemsControl || hit.target == HitTarget::ItemsControlThumb || hit.target == HitTarget::ItemsControlScrollBar;
        log_box_hovered_ = hit.target == HitTarget::LogBox || hit.target == HitTarget::LogBoxThumb || hit.target == HitTarget::LogBoxScrollBar;
        hovered_combo_index_ = hit.target == HitTarget::ComboBoxItem ? hit.combo_index : -1;
        hovered_radio_index_ = hit.target == HitTarget::RadioGroupOption ? hit.radio_index : -1;
        hovered_item_index_ = hovered_combo_index_;
        if (slider_dragging_ && (wparam & MK_LBUTTON) == 0) {
            slider_dragging_ = false;
        }
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
        if (root_element_ != nullptr) {
            input_manager_.route_pointer_move(root_element_, Point {.x = x, .y = y});
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
        const bool handled_by_tree = root_element_ != nullptr
            && input_manager_.route_pointer_down(root_element_, Point {.x = x, .y = y});
        if (handled_by_tree) {
            window_host_->request_render();
            result = 0;
            return true;
        }
        switch (hit.target) {
        case HitTarget::CaptionMinimize:
            if (GetCapture() == window_host_->hwnd()) {
                ReleaseCapture();
            }
            window_host_->set_window_state(WindowState::Minimized);
            result = 0;
            return true;
        case HitTarget::CaptionMaximize:
            if (GetCapture() == window_host_->hwnd()) {
                ReleaseCapture();
            }
            window_host_->set_window_state(
                window_host_->window_state() == WindowState::Maximized ? WindowState::Normal : WindowState::Maximized);
            result = 0;
            return true;
        case HitTarget::CaptionClose:
            if (GetCapture() == window_host_->hwnd()) {
                ReleaseCapture();
            }
            PostMessageW(window_host_->hwnd(), WM_CLOSE, 0, 0);
            result = 0;
            return true;
        case HitTarget::PrimaryButton:
            focused_scroll_target_ = DragScrollTarget::None;
            update_focus(kPrimaryButtonFocusIndex);
            button_pressed_ = true;
            close_combo_box();
            break;
        case HitTarget::TextBox:
            focused_scroll_target_ = DragScrollTarget::None;
            update_focus(kTextBoxFocusIndex);
            text_box_selecting_ = true;
            update_text_box_selection(x, y, false);
            close_combo_box();
            break;
        case HitTarget::RichTextBox:
            focused_scroll_target_ = DragScrollTarget::None;
            update_focus(kRichTextBoxFocusIndex);
            rich_text_box_selecting_ = true;
            update_rich_text_selection(x, y, false);
            close_combo_box();
            break;
        case HitTarget::CheckBox:
            focused_scroll_target_ = DragScrollTarget::None;
            update_focus(kCheckBoxFocusIndex);
            interactive_controls_.check_box->toggle();
            close_combo_box();
            break;
        case HitTarget::ToggleSwitch:
            focused_scroll_target_ = DragScrollTarget::None;
            update_focus(kToggleSwitchFocusIndex);
            if (interactive_controls_.toggle_switch != nullptr) {
                interactive_controls_.toggle_switch->toggle();
            }
            close_combo_box();
            break;
        case HitTarget::ComboBoxHeader:
            focused_scroll_target_ = DragScrollTarget::None;
            update_focus(kComboBoxFocusIndex);
            interactive_controls_.combo_box->toggle_dropdown();
            if (interactive_controls_.combo_box->is_dropdown_open()) {
                combo_box_scroll_offset_ = 0.0F;
            }
            combo_box_pressed_ = true;
            break;
        case HitTarget::ComboBoxItem:
            focused_scroll_target_ = DragScrollTarget::None;
            update_focus(kComboBoxFocusIndex);
            interactive_controls_.combo_box->set_selected_index(static_cast<std::size_t>(hit.combo_index));
            close_combo_box();
            break;
        case HitTarget::RadioGroupOption:
            focused_scroll_target_ = DragScrollTarget::None;
            update_focus(kRadioGroupFocusIndex);
            if (interactive_controls_.radio_group != nullptr && hit.radio_index >= 0) {
                interactive_controls_.radio_group->set_selected_index(static_cast<std::size_t>(hit.radio_index));
            }
            close_combo_box();
            break;
        case HitTarget::ComboBoxThumb:
            focused_scroll_target_ = DragScrollTarget::ComboBox;
            drag_scroll_target_ = DragScrollTarget::ComboBox;
            drag_scroll_anchor_y_ = y;
            drag_scroll_origin_offset_ = combo_box_scroll_offset_;
            break;
        case HitTarget::ComboBoxScrollBar:
            if (interactive_controls_.combo_box != nullptr) {
                const float total_height = compute_combo_dropdown_total_height(interactive_controls_.combo_box->items().size(), 36.0F);
                const float max_offset = max_value(0.0F, total_height - rect_height(layout.combo_dropdown_viewport));
                const float new_offset = compute_track_scroll_offset(layout.combo_dropdown_viewport, layout.combo_dropdown_thumb, y, max_offset);
                combo_box_scroll_offset_ = clamp_combo_box_offset(new_offset);
                begin_track_drag(DragScrollTarget::ComboBox, y, combo_box_scroll_offset_);
            }
            break;
        case HitTarget::Slider:
            focused_scroll_target_ = DragScrollTarget::None;
            update_focus(kSliderFocusIndex);
            slider_dragging_ = true;
            update_slider_from_x(x);
            close_combo_box();
            break;
        case HitTarget::ScrollViewerThumb:
            focused_scroll_target_ = DragScrollTarget::ScrollViewer;
            drag_scroll_target_ = DragScrollTarget::ScrollViewer;
            drag_scroll_anchor_y_ = y;
            drag_scroll_origin_offset_ = interactive_controls_.scroll_viewer != nullptr ? interactive_controls_.scroll_viewer->scroll_offset().y : 0.0F;
            close_combo_box();
            break;
        case HitTarget::ScrollViewerScrollBar:
            if (interactive_controls_.scroll_viewer != nullptr) {
                const float max_offset = clamp_scroll_offset(100000.0F);
                const float new_offset = compute_track_scroll_offset(layout.scroll_viewport, layout.scroll_viewer_thumb, y, max_offset);
                interactive_controls_.scroll_viewer->set_scroll_offset(interactive_controls_.scroll_viewer->scroll_offset().x, new_offset);
                begin_track_drag(DragScrollTarget::ScrollViewer, y, new_offset);
            }
            close_combo_box();
            break;
        case HitTarget::ListViewThumb:
            focused_scroll_target_ = DragScrollTarget::ListView;
            drag_scroll_target_ = DragScrollTarget::ListView;
            drag_scroll_anchor_y_ = y;
            drag_scroll_origin_offset_ = interactive_controls_.list_view != nullptr ? interactive_controls_.list_view->scroll_offset() : 0.0F;
            close_combo_box();
            break;
        case HitTarget::ListViewScrollBar:
            if (interactive_controls_.list_view != nullptr) {
                const float max_offset = clamp_list_view_offset();
                const float new_offset = compute_track_scroll_offset(layout.list_viewport, layout.list_view_thumb, y, max_offset);
                interactive_controls_.list_view->set_scroll_offset(new_offset);
                begin_track_drag(DragScrollTarget::ListView, y, new_offset);
            }
            close_combo_box();
            break;
        case HitTarget::ItemsControlThumb:
            focused_scroll_target_ = DragScrollTarget::ItemsControl;
            drag_scroll_target_ = DragScrollTarget::ItemsControl;
            drag_scroll_anchor_y_ = y;
            drag_scroll_origin_offset_ = interactive_controls_.items_control != nullptr ? interactive_controls_.items_control->scroll_offset() : 0.0F;
            close_combo_box();
            break;
        case HitTarget::ItemsControlScrollBar:
            if (interactive_controls_.items_control != nullptr) {
                const float max_offset = clamp_items_control_offset();
                const float new_offset = compute_track_scroll_offset(layout.items_control_viewport, layout.items_control_thumb, y, max_offset);
                interactive_controls_.items_control->set_scroll_offset(new_offset);
                begin_track_drag(DragScrollTarget::ItemsControl, y, new_offset);
            }
            close_combo_box();
            break;
        case HitTarget::TabControl:
            focused_scroll_target_ = DragScrollTarget::None;
            if (interactive_controls_.tab_control != nullptr && !interactive_controls_.tab_control->tabs().empty()) {
                const float tab_width = rect_width(layout.tab_control) / static_cast<float>(interactive_controls_.tab_control->tabs().size());
                const std::size_t tab_index = min_value(
                    static_cast<std::size_t>(max_value(0.0F, x - layout.tab_control.left) / max_value(1.0F, tab_width)),
                    interactive_controls_.tab_control->tabs().size() - 1U);
                interactive_controls_.tab_control->set_selected_index(tab_index);
                window_host_->request_render();
            }
            close_combo_box();
            break;
        case HitTarget::ExpanderHeader:
            focused_scroll_target_ = DragScrollTarget::None;
            if (interactive_controls_.expander != nullptr) {
                interactive_controls_.expander->toggle();
                window_host_->request_render();
            }
            close_combo_box();
            break;
        case HitTarget::ProgressDecrease:
            focused_scroll_target_ = DragScrollTarget::None;
            if (interactive_controls_.progress != nullptr) {
                interactive_controls_.progress->set_value(interactive_controls_.progress->value() - 5.0F);
                if (interactive_controls_.loading != nullptr) {
                    interactive_controls_.loading->set_active(interactive_controls_.progress->value() < interactive_controls_.progress->max_value());
                }
                if (interactive_controls_.log_box != nullptr) {
                    interactive_controls_.log_box->append_line("[progress] value decreased");
                }
                window_host_->request_render();
            }
            close_combo_box();
            break;
        case HitTarget::ProgressIncrease:
            focused_scroll_target_ = DragScrollTarget::None;
            if (interactive_controls_.progress != nullptr) {
                interactive_controls_.progress->set_value(interactive_controls_.progress->value() + 5.0F);
                if (interactive_controls_.loading != nullptr) {
                    interactive_controls_.loading->set_active(interactive_controls_.progress->value() < interactive_controls_.progress->max_value());
                }
                if (interactive_controls_.log_box != nullptr) {
                    interactive_controls_.log_box->append_line("[progress] value increased");
                }
                window_host_->request_render();
            }
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
        case HitTarget::LogBoxThumb:
            focused_scroll_target_ = DragScrollTarget::LogBox;
            drag_scroll_target_ = DragScrollTarget::LogBox;
            drag_scroll_anchor_y_ = y;
            drag_scroll_origin_offset_ = interactive_controls_.log_box != nullptr ? interactive_controls_.log_box->scroll_offset() : 0.0F;
            close_combo_box();
            break;
        case HitTarget::LogBoxScrollBar:
            if (interactive_controls_.log_box != nullptr) {
                const float max_offset = clamp_log_box_offset(100000.0F);
                const float new_offset = compute_track_scroll_offset(layout.footer_viewport, layout.footer_thumb, y, max_offset);
                interactive_controls_.log_box->set_scroll_offset(new_offset);
                begin_track_drag(DragScrollTarget::LogBox, y, new_offset);
            }
            close_combo_box();
            break;
        case HitTarget::LogBox:
            focused_scroll_target_ = DragScrollTarget::LogBox;
            close_combo_box();
            break;
        case HitTarget::None:
            focused_scroll_target_ = DragScrollTarget::None;
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
        const bool handled_by_tree = root_element_ != nullptr
            && input_manager_.route_pointer_up(root_element_, Point {.x = x, .y = y});
        if (!handled_by_tree && button_pressed_ && hit.target == HitTarget::PrimaryButton) {
            perform_primary_action();
        }
        button_pressed_ = false;
        combo_box_pressed_ = false;
        text_box_selecting_ = false;
        rich_text_box_selecting_ = false;
        slider_dragging_ = false;
        focused_scroll_target_ = DragScrollTarget::None;
        drag_scroll_target_ = DragScrollTarget::None;
        if (GetCapture() == window_host_->hwnd()) {
            ReleaseCapture();
        }
        window_host_->request_render();
        result = 0;
        return true;
    }
    case WM_MOUSEWHEEL: {
        POINT screen_point {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        ScreenToClient(window_host_->hwnd(), &screen_point);
        const float x = static_cast<float>(screen_point.x);
        const float y = static_cast<float>(screen_point.y);
        if (interactive_controls_.combo_box != nullptr && interactive_controls_.combo_box->is_dropdown_open()
            && (point_in_rect(layout.combo_dropdown, x, y) || point_in_rect(layout.combo_box, x, y))) {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / static_cast<float>(WHEEL_DELTA);
            combo_box_scroll_offset_ = clamp_combo_box_offset(combo_box_scroll_offset_ - delta * 36.0F);
            window_host_->request_render();
            result = 0;
            return true;
        }
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
        if (interactive_controls_.log_box != nullptr && point_in_rect(layout.footer, x, y)) {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / static_cast<float>(WHEEL_DELTA);
            interactive_controls_.log_box->set_scroll_offset(clamp_log_box_offset(interactive_controls_.log_box->scroll_offset() - delta * 28.0F));
            focused_scroll_target_ = DragScrollTarget::LogBox;
            window_host_->request_render();
            result = 0;
            return true;
        }
        if (point_in_rect(layout.card, x, y)) {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / static_cast<float>(WHEEL_DELTA);
            page_scroll_offset_ = clamp_page_offset(page_scroll_offset_ - delta * 36.0F);
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
                window_host_->request_render();
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
                window_host_->request_render();
                result = 0;
                return true;
            }
            if (wchar >= 32 && wchar != 127) {
                interactive_controls_.rich_text_box->insert_text(wstring_to_utf8(std::wstring(1, wchar)));
                ensure_rich_text_caret_visible();
                reset_caret_blink();
                window_host_->request_render();
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
            window_host_->request_render();
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
            window_host_->request_render();
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

        if (focused_control_index_ == kToggleSwitchFocusIndex && interactive_controls_.toggle_switch != nullptr) {
            if (wparam == VK_SPACE || wparam == VK_RETURN) {
                interactive_controls_.toggle_switch->toggle();
                window_host_->request_render();
                result = 0;
                return true;
            }
        }

        if (focused_control_index_ == kComboBoxFocusIndex) {
            if (wparam == VK_SPACE || wparam == VK_RETURN) {
                interactive_controls_.combo_box->toggle_dropdown();
                if (interactive_controls_.combo_box->is_dropdown_open()) {
                    combo_box_scroll_offset_ = 0.0F;
                }
                window_host_->request_render();
                result = 0;
                return true;
            }
            if (wparam == VK_DOWN) {
                interactive_controls_.combo_box->open_dropdown();
                interactive_controls_.combo_box->select_next();
                ensure_combo_selection_visible();
                window_host_->request_render();
                result = 0;
                return true;
            }
            if (wparam == VK_UP) {
                interactive_controls_.combo_box->open_dropdown();
                interactive_controls_.combo_box->select_previous();
                ensure_combo_selection_visible();
                window_host_->request_render();
                result = 0;
                return true;
            }
        }

        if (focused_control_index_ == kRadioGroupFocusIndex && interactive_controls_.radio_group != nullptr) {
            if (wparam == VK_LEFT || wparam == VK_UP) {
                interactive_controls_.radio_group->select_previous();
                window_host_->request_render();
                result = 0;
                return true;
            }
            if (wparam == VK_RIGHT || wparam == VK_DOWN || wparam == VK_SPACE || wparam == VK_RETURN) {
                interactive_controls_.radio_group->select_next();
                window_host_->request_render();
                result = 0;
                return true;
            }
        }

        if (focused_control_index_ == kSliderFocusIndex) {
            if (wparam == VK_LEFT) {
                interactive_controls_.slider->step_by(-1.0F);
                window_host_->request_render();
                result = 0;
                return true;
            }
            if (wparam == VK_RIGHT) {
                interactive_controls_.slider->step_by(1.0F);
                window_host_->request_render();
                result = 0;
                return true;
            }
            if (wparam == VK_HOME) {
                interactive_controls_.slider->set_value(interactive_controls_.slider->min_value());
                window_host_->request_render();
                result = 0;
                return true;
            }
            if (wparam == VK_END) {
                interactive_controls_.slider->set_value(interactive_controls_.slider->max_value());
                window_host_->request_render();
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
    if (interactive_controls_.toggle_switch) {
        interactive_controls_.toggle_switch->set_focused(focused_control_index_ == kToggleSwitchFocusIndex);
    }
    if (interactive_controls_.combo_box) {
        interactive_controls_.combo_box->set_focused(focused_control_index_ == kComboBoxFocusIndex);
    }
    if (interactive_controls_.radio_group) {
        interactive_controls_.radio_group->set_focused(focused_control_index_ == kRadioGroupFocusIndex);
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
        combo_box_scroll_offset_ = 0.0F;
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

bool WindowRenderTarget::needs_continuous_rendering() const {
    if (!interactive_mode_enabled_) {
        return false;
    }

    if (interactive_controls_.loading != nullptr && interactive_controls_.loading->active()) {
        return true;
    }
    if (interactive_controls_.progress != nullptr && interactive_controls_.progress->is_indeterminate()) {
        return true;
    }
    return false;
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
                L"Microsoft YaHei UI",
                nullptr,
                DWRITE_FONT_WEIGHT_SEMI_BOLD,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                18.0F,
                L"en-us",
                &text_format_);
            if (FAILED(format_hr) || text_format_ == nullptr) {
                text_format_ = nullptr;
            } else {
                text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }

            const HRESULT item_format_hr = dwrite_factory_->CreateTextFormat(
                L"Microsoft YaHei UI",
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
                L"Microsoft YaHei UI",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                14.0F,
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
                L"Microsoft YaHei UI",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                14.0F,
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
