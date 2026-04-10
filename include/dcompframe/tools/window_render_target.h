#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d2d1_1.h>
#include <dcomp.h>
#include <dwrite.h>
#include <dxgi1_2.h>

#include "dcompframe/render_manager.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/window_host.h"

namespace dcompframe {

enum class DragScrollTarget {
    None,
    ScrollViewer,
    ListView,
    ItemsControl,
};

class WindowRenderTarget {
public:
    struct OverlayScene {
        std::wstring title;
        std::vector<std::wstring> items;
    };

    struct InteractiveControls {
        std::shared_ptr<Button> primary_button;
        std::shared_ptr<TextBox> text_box;
        std::shared_ptr<RichTextBox> rich_text_box;
        std::shared_ptr<CheckBox> check_box;
        std::shared_ptr<ComboBox> combo_box;
        std::shared_ptr<Slider> slider;
        std::shared_ptr<ScrollViewer> scroll_viewer;
        std::shared_ptr<ListView> list_view;
        std::shared_ptr<ItemsControl> items_control;
        std::shared_ptr<TextBlock> text_block;
        std::shared_ptr<Label> label;
        std::shared_ptr<Image> image;
        std::shared_ptr<Progress> progress;
        std::shared_ptr<Loading> loading;
        std::shared_ptr<TabControl> tab_control;
        std::shared_ptr<Popup> popup;
        std::shared_ptr<Expander> expander;
        std::shared_ptr<Card> card;
    };

    WindowRenderTarget(RenderManager* render_manager, WindowHost* window_host);
    ~WindowRenderTarget();

    void set_overlay_scene(OverlayScene scene);
    void set_interactive_controls(InteractiveControls controls);
    void set_primary_action_handler(std::function<void()> handler);
    bool initialize();
    bool render_frame(bool has_dirty_changes = true);
    bool handle_window_message(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& result);

    [[nodiscard]] bool is_ready() const;
    [[nodiscard]] int presented_frames() const;

private:
    bool initialize_dx11_dcomp_target();
    bool ensure_swap_chain_size();
    bool recreate_render_target_view();
    bool initialize_d2d_overlay();
    bool recreate_d2d_target();
    void cleanup_dx11_dcomp_target();
    void sync_focus_state();
    void reset_caret_blink();
    void close_combo_box();
    void perform_primary_action();

    RenderManager* render_manager_ = nullptr;
    WindowHost* window_host_ = nullptr;
    CompositionBridge bridge_ {nullptr};
    bool ready_ = false;
    int presented_frames_ = 0;
    bool using_dx11_dcomp_ = false;
    unsigned int swap_chain_width_ = 0;
    unsigned int swap_chain_height_ = 0;

    ID3D11Device* d3d_device_ = nullptr;
    ID3D11DeviceContext* d3d_context_ = nullptr;
    ID3D11DeviceContext1* d3d_context1_ = nullptr;
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
    IDWriteTextFormat* item_text_format_ = nullptr;
    IDWriteTextFormat* input_text_format_ = nullptr;
    IDWriteTextFormat* helper_text_format_ = nullptr;
    OverlayScene overlay_scene_ {};
    InteractiveControls interactive_controls_ {};
    std::function<void()> primary_action_handler_ {};
    bool interactive_mode_enabled_ = false;
    bool mouse_left_down_ = false;
    bool button_hovered_ = false;
    bool button_pressed_ = false;
    bool text_box_selecting_ = false;
    bool rich_text_box_selecting_ = false;
    bool slider_dragging_ = false;
    bool combo_box_hovered_ = false;
    bool combo_box_pressed_ = false;
    bool slider_hovered_ = false;
    bool check_box_hovered_ = false;
    bool text_box_hovered_ = false;
    bool rich_text_box_hovered_ = false;
    bool scroll_viewer_hovered_ = false;
    bool list_view_hovered_ = false;
    bool items_control_hovered_ = false;
    int hovered_item_index_ = -1;
    int hovered_combo_index_ = -1;
    int button_click_count_ = 0;
    float combo_box_scroll_offset_ = 0.0F;
    float page_scroll_offset_ = 0.0F;
    std::size_t focused_control_index_ = 0;
    std::optional<std::size_t> pressed_combo_index_ {};
    unsigned long long caret_blink_seed_ = 0;
    DragScrollTarget drag_scroll_target_ = DragScrollTarget::None;
    float drag_scroll_anchor_y_ = 0.0F;
    float drag_scroll_origin_offset_ = 0.0F;
};

}  // namespace dcompframe
