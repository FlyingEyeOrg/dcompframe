#include <fmt/format.h>

#include <memory>
#include <string>

#include "dcompframe/application.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/layout/flex_panel.h"

namespace {

std::string phase_name(dcompframe::EventPhase phase) {
    switch (phase) {
    case dcompframe::EventPhase::Capture:
        return "capture";
    case dcompframe::EventPhase::Target:
        return "target";
    case dcompframe::EventPhase::Bubble:
        return "bubble";
    }

    return "unknown";
}

bool is_pointer_event(dcompframe::EventType type) {
    return type == dcompframe::EventType::MouseDown || type == dcompframe::EventType::MouseMove
        || type == dcompframe::EventType::MouseUp;
}

class FlexLayoutDemoWindow final : public dcompframe::Window {
public:
    FlexLayoutDemoWindow(
        dcompframe::RenderManager* render_manager,
        dcompframe::Application* application,
        std::size_t window_id)
        : dcompframe::Window(render_manager, application, window_id) {}

protected:
    bool build(const dcompframe::AppConfig& config) override {
        (void)config;

        configure_panels();
        configure_controls();
        build_layout_tree();
        wire_hit_test_trace();

        set_root(root_panel_);
        set_interactive_controls(dcompframe::WindowRenderTarget::InteractiveControls {
            .primary_button = primary_button_,
            .text_box = text_box_,
            .rich_text_box = rich_text_box_,
            .check_box = check_box_,
            .toggle_switch = toggle_switch_,
            .combo_box = combo_box_,
            .radio_group = radio_group_,
            .slider = slider_,
            .scroll_viewer = scroll_viewer_,
            .list_view = list_view_,
            .items_control = items_control_,
            .text_block = text_block_,
            .label = label_,
            .badge = badge_,
            .divider = divider_,
            .image = image_,
            .progress = progress_,
            .loading = loading_,
            .log_box = log_box_,
            .tab_control = tab_control_,
            .popup = popup_,
            .expander = expander_,
            .card = card_,
        });
        set_primary_action_handler([this] {
            if (application() != nullptr) {
                application()->create_window();
            }
        });
        return true;
    }

    [[nodiscard]] std::wstring title() const override {
        return std::wstring(L"DCompFrame Flexbox Demo - Window ") + std::to_wstring(id());
    }

private:
    static dcompframe::Thickness make_insets(float horizontal, float vertical) {
        return dcompframe::Thickness {.left = horizontal, .top = vertical, .right = horizontal, .bottom = vertical};
    }

    void reserve_label_slot(const std::shared_ptr<dcompframe::UIElement>& element, float top = 24.0F, float bottom = 4.0F) {
        element->set_margin(dcompframe::Thickness {.left = 0.0F, .top = top, .right = 0.0F, .bottom = bottom});
    }

    void configure_panels() {
        page_->set_margin(dcompframe::Thickness {.left = 24.0F, .top = 72.0F, .right = 24.0F, .bottom = 24.0F});
        page_->set_padding(make_insets(18.0F, 16.0F));
        page_->set_row_gap(22.0F);

        hero_row_->set_padding(make_insets(14.0F, 14.0F));
        hero_row_->set_column_gap(20.0F);
        hero_row_->set_row_gap(24.0F);
        hero_row_->set_wrap(dcompframe::FlexWrap::Wrap);
        hero_row_->set_align_items(dcompframe::FlexAlignItems::Stretch);

        form_column_->set_padding(make_insets(16.0F, 14.0F));
        form_column_->set_row_gap(18.0F);
        form_column_->set_flex_grow(1.0F);
        form_column_->set_flex_basis(420.0F);
        form_column_->set_min_size(dcompframe::Size {.width = 420.0F, .height = 0.0F});

        options_row_->set_column_gap(14.0F);
        options_row_->set_row_gap(14.0F);
        options_row_->set_wrap(dcompframe::FlexWrap::Wrap);

        switches_row_->set_column_gap(14.0F);
        switches_row_->set_row_gap(12.0F);
        switches_row_->set_wrap(dcompframe::FlexWrap::Wrap);
        switches_row_->set_align_items(dcompframe::FlexAlignItems::Center);

        action_row_->set_column_gap(12.0F);
        action_row_->set_row_gap(12.0F);
        action_row_->set_wrap(dcompframe::FlexWrap::Wrap);
        action_row_->set_align_items(dcompframe::FlexAlignItems::Center);

        preview_column_->set_padding(make_insets(16.0F, 14.0F));
        preview_column_->set_row_gap(18.0F);
        preview_column_->set_flex_grow(1.0F);
        preview_column_->set_flex_basis(420.0F);
        preview_column_->set_min_size(dcompframe::Size {.width = 420.0F, .height = 0.0F});

        status_row_->set_column_gap(14.0F);
        status_row_->set_row_gap(12.0F);
        status_row_->set_wrap(dcompframe::FlexWrap::Wrap);
        status_row_->set_align_items(dcompframe::FlexAlignItems::Center);

        collections_row_->set_padding(make_insets(14.0F, 14.0F));
        collections_row_->set_column_gap(20.0F);
        collections_row_->set_row_gap(24.0F);
        collections_row_->set_wrap(dcompframe::FlexWrap::Wrap);

        list_view_->set_flex_grow(1.0F);
        list_view_->set_flex_basis(340.0F);
        items_control_->set_flex_grow(1.0F);
        items_control_->set_flex_basis(340.0F);

        scroll_viewer_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 184.0F});
        log_box_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 212.0F});
    }

    void configure_controls() {
        text_box_->set_placeholder("输入标题，观察 Flex 布局与事件日志联动");
        text_box_->set_text(fmt::format("Flexbox Demo Window {}", id()));
        text_box_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 46.0F});
        reserve_label_slot(text_box_);

        rich_text_box_->set_rich_text(
            "此 Demo 现在只保留 Flexbox 主线。\n"
            "- 旧 Grid/Stack 已从框架主路径移除。\n"
            "- 每个控件都拥有独立的 Flex 边界。\n"
            "- 标题栏走自定义非客户区 + DWM 扩展。\n"
            "- 鼠标事件统一经过 hit-test 与 capture/target/bubble。"
        );
        rich_text_box_->set_flex_grow(1.0F);
        rich_text_box_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 208.0F});
        rich_text_box_->set_min_size(dcompframe::Size {.width = 0.0F, .height = 188.0F});
        reserve_label_slot(rich_text_box_);

        check_box_->set_checked(true);
        check_box_->set_flex_grow(1.0F);
        check_box_->set_flex_basis(176.0F);
        check_box_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 46.0F});

        toggle_switch_->set_checked(true);
        toggle_switch_->set_desired_size(dcompframe::Size {.width = 92.0F, .height = 40.0F});
        reserve_label_slot(toggle_switch_);

        combo_box_->set_items({"row", "column", "wrap", "grow", "space-evenly", "align-content"});
        combo_box_->set_selected_index(4);
        combo_box_->set_flex_grow(1.0F);
        combo_box_->set_flex_basis(176.0F);
        combo_box_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 46.0F});
        reserve_label_slot(combo_box_);

        radio_group_->set_items({"Design", "Runtime", "Routing"});
        radio_group_->set_selected_index(0);
        radio_group_->set_desired_size(dcompframe::Size {.width = 320.0F, .height = 44.0F});
        reserve_label_slot(radio_group_);

        slider_->set_range(0.0F, 100.0F);
        slider_->set_step(5.0F);
        slider_->set_value(65.0F);
        slider_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 48.0F});
        reserve_label_slot(slider_);

        primary_button_->set_text("新建窗口");
        primary_button_->set_desired_size(dcompframe::Size {.width = 148.0F, .height = 40.0F});

        badge_->set_text("Element Plus");
        badge_->set_tone(dcompframe::BadgeTone::Primary);
        badge_->set_desired_size(dcompframe::Size {.width = 128.0F, .height = 32.0F});

        divider_->set_orientation(dcompframe::DividerOrientation::Horizontal);
        divider_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 18.0F});

        text_block_->set_text(
            "右侧列不再把 Tab/Progress/Popup/Expander 塞进一张卡片里，而是让它们作为独立控件并列在 Flex 树中。"
        );
        text_block_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 88.0F});
        reserve_label_slot(text_block_);

        label_->set_text("规范: 新增布局与示例只允许使用 Flexbox");
        label_->set_desired_size(dcompframe::Size {.width = 260.0F, .height = 32.0F});

        image_->set_source("demo://flexbox-preview");
        image_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 176.0F});
        reserve_label_slot(image_);

        card_->set_title("Independent Card");
        card_->set_body(
            "Card 只展示卡片本身，不再承载其他控件的内部派生布局，因此窗口缩放时不会再把 Tab/Progress/Popup 挤到一起。"
        );
        card_->set_icon("flex");
        card_->set_tags({"flex-only", "stable-bounds", "element-plus"});
        card_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 156.0F});

        tab_control_->set_tabs({"Overview", "Layout", "Route", "API"});
        tab_control_->set_selected_index(1);
        tab_control_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 156.0F});

        expander_->set_header("布局与架构约束");
        expander_->set_content_text(
            "1. 删除旧布局入口，只保留 Flex 主线。\n"
            "2. 布局引擎补足 padding / min / max / space-evenly。\n"
            "3. 标题栏进入自定义非客户区路径。"
        );
        expander_->set_expanded(true);
        expander_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 132.0F});

        progress_->set_range(0.0F, 100.0F);
        progress_->set_value(65.0F);
        progress_->set_flex_grow(1.0F);
        progress_->set_flex_basis(260.0F);
        progress_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 46.0F});

        loading_->set_active(true);
        loading_->set_overlay_mode(false);
        loading_->set_text("Flexbox layout active");
        loading_->set_desired_size(dcompframe::Size {.width = 220.0F, .height = 42.0F});

        popup_->set_title("Popup Notes");
        popup_->set_body(
            "Popup 保留为覆盖式控件，但在 Demo 中它有自己的独立边界和独立绘制入口。"
        );
        popup_->set_modal(true);
        popup_->set_open(true);
        popup_->set_desired_size(dcompframe::Size {.width = 220.0F, .height = 64.0F});

        list_view_->set_groups({
            dcompframe::ListGroup {.name = "Flex Core", .items = {"direction", "wrap", "basis", "grow", "padding"}},
            dcompframe::ListGroup {.name = "Window", .items = {"title bar", "DWM", "hit test", "DPI"}},
        });
        list_view_->set_selected_index(0);
        list_view_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 220.0F});
        reserve_label_slot(list_view_);

        items_control_->set_items({
            "ToggleSwitch / RadioGroup / Badge / Divider 已补入项目",
            "自定义标题栏保留 DWM resize / snap / shadow 行为",
            "旧布局代码已不再进入主库编译",
            "Flex 布局支持 padding 与 min/max 约束",
            "Demo 中每个控件都有独立边界并可单独调试",
        });
        items_control_->set_item_spacing(6.0F);
        items_control_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 220.0F});
        reserve_label_slot(items_control_);

        auto scroll_content = std::make_shared<dcompframe::ItemsControl>();
        scroll_content->set_items({
            "justify-content: start / end / center / space-between / space-around / space-evenly",
            "align-content: start / end / center / stretch / space-between / space-around / space-evenly",
            "item constraints: flex-basis + grow/shrink + min/max",
            "container insets: padding 与 gap 分工明确",
            "命中测试优先最深可见子节点",
            "窗口消息与控件事件统一路由",
        });
        scroll_content->set_item_spacing(6.0F);
        scroll_viewer_->set_content(scroll_content);
        reserve_label_slot(scroll_viewer_);

        log_box_->set_lines({
            "[flex] demo initialized",
            "[frame] custom title bar + DWM path ready",
            "[route] click any visible control to inspect phases",
        });
        log_box_->set_max_lines(200);
        log_box_->set_flex_grow(1.0F);
        reserve_label_slot(log_box_);

        combo_box_->set_on_selection_changed([this](std::optional<std::size_t> index, const std::string& value) {
            log_box_->append_line(fmt::format("[combo] index={} value={}", index ? static_cast<int>(*index) : -1, value));
            host().request_render();
        });
        toggle_switch_->set_on_checked_changed([this](bool checked) {
            badge_->set_text(checked ? "Element Plus" : "Compact");
            badge_->set_tone(checked ? dcompframe::BadgeTone::Primary : dcompframe::BadgeTone::Warning);
            log_box_->append_line(fmt::format("[switch] checked={}", checked));
            host().request_render();
        });
        radio_group_->set_on_selection_changed([this](std::optional<std::size_t> index, const std::string& value) {
            log_box_->append_line(fmt::format("[radio] index={} value={}", index ? static_cast<int>(*index) : -1, value));
            host().request_render();
        });
        slider_->set_on_value_changed([this](float value) {
            progress_->set_value(value);
            loading_->set_active(value < 100.0F);
            log_box_->append_line(fmt::format("[slider] value={:.0f}", value));
            host().request_render();
        });
        text_box_->set_on_text_changed([this](const std::string& value) {
            log_box_->append_line(fmt::format("[textbox] {}", value));
            host().request_render();
        });
    }

    void build_layout_tree() {
        root_panel_->add_child(page_);

        page_->add_child(hero_row_);
        page_->add_child(divider_);
        page_->add_child(collections_row_);
        page_->add_child(scroll_viewer_);
        page_->add_child(log_box_);

        hero_row_->add_child(form_column_);
        hero_row_->add_child(preview_column_);

        form_column_->add_child(text_box_);
        form_column_->add_child(rich_text_box_);
        form_column_->add_child(options_row_);
        form_column_->add_child(switches_row_);
        form_column_->add_child(slider_);
        form_column_->add_child(action_row_);

        options_row_->add_child(check_box_);
        options_row_->add_child(combo_box_);

        switches_row_->add_child(toggle_switch_);
        switches_row_->add_child(radio_group_);

        action_row_->add_child(primary_button_);
        action_row_->add_child(badge_);

        preview_column_->add_child(text_block_);
        preview_column_->add_child(label_);
        preview_column_->add_child(image_);
        preview_column_->add_child(card_);
        preview_column_->add_child(tab_control_);
        preview_column_->add_child(expander_);
        preview_column_->add_child(status_row_);

        status_row_->add_child(progress_);
        status_row_->add_child(loading_);
        status_row_->add_child(popup_);

        collections_row_->add_child(list_view_);
        collections_row_->add_child(items_control_);
    }

    void wire_hit_test_trace() {
        const auto attach_trace = [this](const std::shared_ptr<dcompframe::UIElement>& element, const std::string& label) {
            element->set_event_handler([this, label](dcompframe::InputEvent& event, dcompframe::EventPhase phase) {
                if (!is_pointer_event(event.type) || event.type != dcompframe::EventType::MouseDown) {
                    return;
                }

                const std::string target_name = event.target != nullptr ? event.target->name() : "none";
                log_box_->append_line(fmt::format(
                    "[route] {} -> {} at ({:.0f}, {:.0f}) target={}",
                    phase_name(phase),
                    label,
                    event.position.x,
                    event.position.y,
                    target_name));
                host().request_render();
            });
        };

        attach_trace(root_panel_, "root");
        attach_trace(page_, "page");
        attach_trace(hero_row_, "hero-row");
        attach_trace(form_column_, "form-column");
        attach_trace(preview_column_, "preview-column");
        attach_trace(collections_row_, "collections-row");
        attach_trace(text_box_, "text-box");
        attach_trace(combo_box_, "combo-box");
        attach_trace(toggle_switch_, "toggle-switch");
        attach_trace(radio_group_, "radio-group");
        attach_trace(list_view_, "list-view");
        attach_trace(card_, "card");
    }

    std::shared_ptr<dcompframe::Panel> root_panel_ = std::make_shared<dcompframe::Panel>();
    std::shared_ptr<dcompframe::FlexPanel> page_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Column);
    std::shared_ptr<dcompframe::FlexPanel> hero_row_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Row);
    std::shared_ptr<dcompframe::FlexPanel> form_column_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Column);
    std::shared_ptr<dcompframe::FlexPanel> options_row_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Row);
    std::shared_ptr<dcompframe::FlexPanel> switches_row_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Row);
    std::shared_ptr<dcompframe::FlexPanel> action_row_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Row);
    std::shared_ptr<dcompframe::FlexPanel> preview_column_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Column);
    std::shared_ptr<dcompframe::FlexPanel> status_row_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Row);
    std::shared_ptr<dcompframe::FlexPanel> collections_row_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Row);
    std::shared_ptr<dcompframe::Button> primary_button_ = std::make_shared<dcompframe::Button>("新建窗口");
    std::shared_ptr<dcompframe::TextBox> text_box_ = std::make_shared<dcompframe::TextBox>();
    std::shared_ptr<dcompframe::RichTextBox> rich_text_box_ = std::make_shared<dcompframe::RichTextBox>();
    std::shared_ptr<dcompframe::CheckBox> check_box_ = std::make_shared<dcompframe::CheckBox>();
    std::shared_ptr<dcompframe::ToggleSwitch> toggle_switch_ = std::make_shared<dcompframe::ToggleSwitch>();
    std::shared_ptr<dcompframe::ComboBox> combo_box_ = std::make_shared<dcompframe::ComboBox>();
    std::shared_ptr<dcompframe::RadioGroup> radio_group_ = std::make_shared<dcompframe::RadioGroup>();
    std::shared_ptr<dcompframe::Slider> slider_ = std::make_shared<dcompframe::Slider>();
    std::shared_ptr<dcompframe::ScrollViewer> scroll_viewer_ = std::make_shared<dcompframe::ScrollViewer>();
    std::shared_ptr<dcompframe::ItemsControl> items_control_ = std::make_shared<dcompframe::ItemsControl>();
    std::shared_ptr<dcompframe::ListView> list_view_ = std::make_shared<dcompframe::ListView>();
    std::shared_ptr<dcompframe::TextBlock> text_block_ = std::make_shared<dcompframe::TextBlock>();
    std::shared_ptr<dcompframe::Label> label_ = std::make_shared<dcompframe::Label>();
    std::shared_ptr<dcompframe::Badge> badge_ = std::make_shared<dcompframe::Badge>();
    std::shared_ptr<dcompframe::Divider> divider_ = std::make_shared<dcompframe::Divider>();
    std::shared_ptr<dcompframe::Image> image_ = std::make_shared<dcompframe::Image>();
    std::shared_ptr<dcompframe::Progress> progress_ = std::make_shared<dcompframe::Progress>();
    std::shared_ptr<dcompframe::Loading> loading_ = std::make_shared<dcompframe::Loading>();
    std::shared_ptr<dcompframe::LogBox> log_box_ = std::make_shared<dcompframe::LogBox>();
    std::shared_ptr<dcompframe::TabControl> tab_control_ = std::make_shared<dcompframe::TabControl>();
    std::shared_ptr<dcompframe::Popup> popup_ = std::make_shared<dcompframe::Popup>();
    std::shared_ptr<dcompframe::Expander> expander_ = std::make_shared<dcompframe::Expander>();
    std::shared_ptr<dcompframe::Card> card_ = std::make_shared<dcompframe::Card>();
};

}  // namespace

int main() {
    dcompframe::Application app;
    app.set_window_factory([](
                                 dcompframe::RenderManager* render_manager,
                                 dcompframe::Application* application,
                                 std::size_t window_id) {
        return std::make_unique<FlexLayoutDemoWindow>(render_manager, application, window_id);
    });
    if (!app.initialize("demo/demo-config.json")) {
        return 1;
    }

    app.render_manager().resource_manager().register_resource(
        "main-card-texture",
        dcompframe::ResourceType::Texture,
        2048 * 2048 * 4);
    app.render_manager().resource_manager().register_resource(
        "ui-vertex-buffer",
        dcompframe::ResourceType::Buffer,
        1024 * 64);
    if (!app.create_window()) {
        return 1;
    }

    return app.run();
}