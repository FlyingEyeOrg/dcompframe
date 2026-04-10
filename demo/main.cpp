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
            .combo_box = combo_box_,
            .slider = slider_,
            .scroll_viewer = scroll_viewer_,
            .list_view = list_view_,
            .items_control = items_control_,
            .text_block = text_block_,
            .label = label_,
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
    void configure_panels() {
        page_->set_margin(dcompframe::Thickness {.left = 24.0F, .top = 122.0F, .right = 24.0F, .bottom = 24.0F});
        page_->set_row_gap(18.0F);

        hero_row_->set_column_gap(20.0F);
        hero_row_->set_align_items(dcompframe::FlexAlignItems::Stretch);
        hero_row_->set_flex_grow(3.0F);

        form_column_->set_row_gap(14.0F);
        form_column_->set_flex_grow(1.0F);
        form_column_->set_flex_basis(0.0F);

        options_row_->set_column_gap(14.0F);
        options_row_->set_wrap(dcompframe::FlexWrap::Wrap);

        preview_column_->set_row_gap(14.0F);
        preview_column_->set_flex_grow(1.0F);
        preview_column_->set_flex_basis(0.0F);

        collections_row_->set_column_gap(20.0F);
        collections_row_->set_flex_grow(1.0F);
    }

    void configure_controls() {
        text_box_->set_placeholder("Flexbox 布局: TextBox 由 flex-basis + flex-grow 管理主轴尺寸");
        text_box_->set_text(fmt::format("Flexbox Demo Window {}", id()));

        rich_text_box_->set_rich_text(
            "当前 demo 只使用 Web Flexbox 语义构建布局。\n"
            "1. 顶部区域使用 row + column 两层 flex 容器。\n"
            "2. 中部列表区使用 row 容器并给左右列相同 grow。\n"
            "3. 所有后续布局开发统一收敛到 flex-direction / wrap / gap / grow / shrink。\n"
            "4. 鼠标点击会走元素树 hit-test，再按隧道/目标/冒泡写入日志。");
        rich_text_box_->set_flex_grow(1.0F);

        check_box_->set_checked(true);
        check_box_->set_flex_grow(1.0F);
        check_box_->set_flex_basis(0.0F);

        combo_box_->set_items({"row", "column", "wrap", "grow", "shrink", "align"});
        combo_box_->set_selected_index(2);
        combo_box_->set_flex_grow(1.0F);
        combo_box_->set_flex_basis(0.0F);

        slider_->set_range(0.0F, 100.0F);
        slider_->set_step(5.0F);
        slider_->set_value(65.0F);

        text_block_->set_text(
            "右侧预览列使用 column flex 容器。卡片区域通过 flex-grow 吃掉剩余高度，模拟 Web 页面里常见的自适应侧栏。"
        );
        image_->set_source("demo://flexbox-preview");

        card_->set_title("Flex Layout Card");
        card_->set_body(
            "Card 自身不是容器算法的特例。它只是一个普通 flex item，通过 flex-grow 和 flex-basis 参与剩余空间分配。"
        );
        card_->set_icon("flex");
        card_->set_tags({"flex-direction", "gap", "bubble"});
        card_->set_primary_action(primary_button_);
        card_->set_flex_grow(1.0F);

        list_view_->set_groups({
            dcompframe::ListGroup {.name = "Flex Core", .items = {"direction", "wrap", "basis", "grow"}},
            dcompframe::ListGroup {.name = "Event Flow", .items = {"capture", "target", "bubble"}},
        });
        list_view_->set_selected_index(0);
        list_view_->set_flex_grow(1.0F);
        list_view_->set_flex_basis(0.0F);

        items_control_->set_items({
            "所有主布局容器统一为 FlexPanel",
            "旧 GridPanel 仅保留兼容测试，不再用于新界面",
            "hit-test 从元素树最深命中节点开始回溯",
            "InputManager 负责把窗口指针事件路由到树上",
            "文档与 README 统一约束后续只写 Flexbox",
        });
        items_control_->set_item_spacing(6.0F);
        items_control_->set_flex_grow(1.0F);
        items_control_->set_flex_basis(0.0F);

        auto scroll_content = std::make_shared<dcompframe::ItemsControl>();
        scroll_content->set_items({
            "justify-content: 主轴空白分配",
            "align-items: 交叉轴对齐",
            "align-self: 子项局部覆盖",
            "flex-basis: 初始主轴尺寸",
            "flex-grow / flex-shrink: 剩余空间解析",
            "wrap + row-gap/column-gap: 多行换行布局",
        });
        scroll_content->set_item_spacing(6.0F);
        scroll_viewer_->set_content(scroll_content);
        scroll_viewer_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 148.0F});

        log_box_->set_lines({
            "[flex] demo initialized",
            "[flex] click any visible element to inspect route phases",
            "[flex] top-level layout uses only FlexPanel containers",
        });
        log_box_->set_max_lines(200);
        log_box_->set_desired_size(dcompframe::Size {.width = 0.0F, .height = 180.0F});
        log_box_->set_flex_grow(1.0F);

        label_->set_text("规范: 后续新增布局只能使用 Flexbox");
        progress_->set_range(0.0F, 100.0F);
        progress_->set_value(65.0F);
        loading_->set_active(true);
        loading_->set_overlay_mode(false);
        loading_->set_text("Flexbox layout active");
        tab_control_->set_tabs({"Overview", "Layout", "Route", "Spec"});
        tab_control_->set_selected_index(1);
        popup_->set_title("Flexbox Notes");
        popup_->set_body("Popup 继续作为 overlay 演示对象，但页面主布局已经全部切到 FlexPanel。\n命中测试仍然基于元素树进行。"
        );
        popup_->set_modal(true);
        popup_->set_open((id() % 2U) == 1U);
        expander_->set_header("布局规范说明");
        expander_->set_content_text(
            "1. 容器统一使用 FlexPanel。\n"
            "2. 用嵌套 flex 表达二维布局。\n"
            "3. 命中测试先找最深命中元素，再执行 capture / target / bubble。"
        );
        expander_->set_expanded(true);

        combo_box_->set_on_selection_changed([this](std::optional<std::size_t> index, const std::string& value) {
            log_box_->append_line(
                fmt::format("[combo] index={} value={}", index ? static_cast<int>(*index) : -1, value));
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
        page_->add_child(collections_row_);
        page_->add_child(scroll_viewer_);
        page_->add_child(log_box_);

        hero_row_->add_child(form_column_);
        hero_row_->add_child(preview_column_);

        form_column_->add_child(text_box_);
        form_column_->add_child(rich_text_box_);
        form_column_->add_child(options_row_);
        form_column_->add_child(slider_);

        options_row_->add_child(check_box_);
        options_row_->add_child(combo_box_);

        preview_column_->add_child(text_block_);
        preview_column_->add_child(image_);
        preview_column_->add_child(card_);

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
        attach_trace(list_view_, "list-view");
        attach_trace(card_, "card");
    }

    std::shared_ptr<dcompframe::Panel> root_panel_ = std::make_shared<dcompframe::Panel>();
    std::shared_ptr<dcompframe::FlexPanel> page_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Column);
    std::shared_ptr<dcompframe::FlexPanel> hero_row_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Row);
    std::shared_ptr<dcompframe::FlexPanel> form_column_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Column);
    std::shared_ptr<dcompframe::FlexPanel> options_row_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Row);
    std::shared_ptr<dcompframe::FlexPanel> preview_column_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Column);
    std::shared_ptr<dcompframe::FlexPanel> collections_row_ = std::make_shared<dcompframe::FlexPanel>(dcompframe::FlexDirection::Row);
    std::shared_ptr<dcompframe::Button> primary_button_ = std::make_shared<dcompframe::Button>("新建窗口");
    std::shared_ptr<dcompframe::TextBox> text_box_ = std::make_shared<dcompframe::TextBox>();
    std::shared_ptr<dcompframe::RichTextBox> rich_text_box_ = std::make_shared<dcompframe::RichTextBox>();
    std::shared_ptr<dcompframe::CheckBox> check_box_ = std::make_shared<dcompframe::CheckBox>();
    std::shared_ptr<dcompframe::ComboBox> combo_box_ = std::make_shared<dcompframe::ComboBox>();
    std::shared_ptr<dcompframe::Slider> slider_ = std::make_shared<dcompframe::Slider>();
    std::shared_ptr<dcompframe::ScrollViewer> scroll_viewer_ = std::make_shared<dcompframe::ScrollViewer>();
    std::shared_ptr<dcompframe::ItemsControl> items_control_ = std::make_shared<dcompframe::ItemsControl>();
    std::shared_ptr<dcompframe::ListView> list_view_ = std::make_shared<dcompframe::ListView>();
    std::shared_ptr<dcompframe::TextBlock> text_block_ = std::make_shared<dcompframe::TextBlock>();
    std::shared_ptr<dcompframe::Label> label_ = std::make_shared<dcompframe::Label>();
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