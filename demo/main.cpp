#include <fmt/format.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "dcompframe/application.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/layout/grid_panel.h"
#include "dcompframe/layout/stack_panel.h"

namespace {

class LayoutValidationWindow final : public dcompframe::Window {
public:
    LayoutValidationWindow(
        dcompframe::RenderManager* render_manager,
        dcompframe::Application* application,
        std::size_t window_id)
        : dcompframe::Window(render_manager, application, window_id) {}

protected:
    bool build(const dcompframe::AppConfig& config) override {
        (void)config;

        configure_controls();
        build_layout_tree();

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
        return std::wstring(L"DCompFrame Layout Validation - Window ") + std::to_wstring(id());
    }

private:
    void configure_controls() {
        content_stack_->set_margin(
            dcompframe::Thickness {.left = 24.0F, .top = 122.0F, .right = 24.0F, .bottom = 24.0F});
        content_stack_->set_spacing(16.0F);

        top_grid_->set_flex_grow(5.0F);
        collections_grid_->set_flex_grow(2.0F);
        footer_grid_->set_flex_grow(2.0F);

        form_stack_->set_spacing(12.0F);
        preview_stack_->set_spacing(12.0F);

        text_box_->set_placeholder("布局验证页: TextBox 使用元素自身尺寸，父容器负责最终分配");
        text_box_->set_text(fmt::format("Layout Validation Window {}", id()));

        rich_text_box_->set_rich_text(
            "这不是旧 demo。\n"
            "当前窗口只用于验证三类容器: Panel / StackPanel / GridPanel。\n"
            "布局由 measure + arrange 驱动，容器根据子元素尺寸计算自身需要的空间。\n"
            "窗口拉伸时，富文本区、卡片区、列表区和底部区域会按 flex grow 分配剩余空间。");
        rich_text_box_->set_flex_grow(1.0F);

        check_box_->set_checked(true);
        combo_box_->set_items({"Start", "Center", "End", "Stretch", "Grow", "Shrink"});
        combo_box_->set_selected_index(4);

        slider_->set_range(0.0F, 100.0F);
        slider_->set_step(5.0F);
        slider_->set_value(60.0F);

        text_block_->set_text(
            "右侧预览区不再使用手工 section 比例公式，而是直接由 Grid + StackPanel 的测量结果驱动。");
        image_->set_source("demo://layout-validation-preview");
        card_->set_title("Flex Layout Card");
        card_->set_body("Card 区域通过 flex grow 吃掉右列剩余空间，用来验证容器在主轴上分配空白区域。");
        card_->set_icon("grid");
        card_->set_tags({"measure", "arrange", "flex"});
        card_->set_primary_action(primary_button_);
        card_->set_flex_grow(1.0F);

        list_view_->set_groups({
            dcompframe::ListGroup {.name = "Layout", .items = {"measure pass", "arrange pass", "flex grow", "cross axis"}},
            dcompframe::ListGroup {.name = "Grid", .items = {"column width", "row height", "span sizing"}},
        });
        list_view_->set_selected_index(1);

        items_control_->set_items({
            "Panel overlays child area",
            "StackPanel computes main axis sum",
            "GridPanel computes cell tracks from content",
            "Leaf controls expose intrinsic sizes",
            "Window directly drives root layout",
            "Old demo metrics removed",
        });
        items_control_->set_item_spacing(6.0F);

        auto scroll_content = std::make_shared<dcompframe::ItemsControl>();
        scroll_content->set_items({
            "Measure child with available cross size",
            "Distribute remaining main-axis space by flex-grow",
            "Shrink proportionally when host is too small",
            "Grid uses max cell contribution to compute tracks",
            "Panel stretches child to arranged slot",
            "Interactive overlay consumes actual control bounds",
        });
        scroll_content->set_item_spacing(6.0F);
        scroll_viewer_->set_content(scroll_content);

        log_box_->set_lines({
            "[layout] root panel owns the viewport slot",
            "[layout] content stack uses top margin to stay below title band",
            "[layout] vertical flex drives section growth",
            "[layout] grid measures columns and rows from child content",
            "[layout] no custom demo section metrics remain",
        });
        log_box_->set_max_lines(100);

        progress_->set_range(0.0F, 100.0F);
        progress_->set_value(60.0F);
        loading_->set_active(true);
        loading_->set_overlay_mode(false);
        loading_->set_text("布局引擎验证中...");
        tab_control_->set_tabs({"Layout", "Grid", "Stack", "Panel"});
        tab_control_->set_selected_index(0);
        popup_->set_title("Layout Notes");
        popup_->set_body("Popup 继续用于验证 overlay 交互，不再参与主布局计算。\n布局验证只看容器树。");
        popup_->set_modal(true);
        popup_->set_open((id() % 2U) == 1U);
        expander_->set_header("布局系统说明");
        expander_->set_content_text(
            "1. Window 先 measure 再 arrange 根元素。\n"
            "2. StackPanel 负责主轴空间分配与交叉轴对齐。\n"
            "3. GridPanel 根据子元素测量结果计算轨道尺寸。");
        expander_->set_expanded(true);
        label_->set_text("状态: 布局验证窗口");

        combo_box_->set_on_selection_changed(
            [this](std::optional<std::size_t> index, const std::string& value) {
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
        primary_button_->set_on_click([this] {
            if (application() != nullptr) {
                application()->render_manager().diagnostics().log(
                    dcompframe::LogLevel::Info,
                    fmt::format("Layout validation requested create_window from {}", id()));
            }
        });
    }

    void build_layout_tree() {
        root_panel_->add_child(content_stack_);

        content_stack_->add_child(top_grid_);
        content_stack_->add_child(collections_grid_);
        content_stack_->add_child(footer_grid_);

        top_grid_->add_child(form_stack_);
        top_grid_->set_grid_position(form_stack_, {.row = 0, .col = 0});
        top_grid_->add_child(preview_stack_);
        top_grid_->set_grid_position(preview_stack_, {.row = 0, .col = 1});

        form_stack_->add_child(text_box_);
        form_stack_->add_child(rich_text_box_);
        form_stack_->add_child(option_grid_);
        form_stack_->add_child(slider_);

        option_grid_->add_child(check_box_);
        option_grid_->set_grid_position(check_box_, {.row = 0, .col = 0});
        option_grid_->add_child(combo_box_);
        option_grid_->set_grid_position(combo_box_, {.row = 0, .col = 1});

        preview_stack_->add_child(text_block_);
        preview_stack_->add_child(image_);
        preview_stack_->add_child(card_);

        collections_grid_->add_child(list_view_);
        collections_grid_->set_grid_position(list_view_, {.row = 0, .col = 0});
        collections_grid_->add_child(items_control_);
        collections_grid_->set_grid_position(items_control_, {.row = 0, .col = 1});

        footer_grid_->add_child(scroll_viewer_);
        footer_grid_->set_grid_position(scroll_viewer_, {.row = 0, .col = 0});
        footer_grid_->add_child(log_box_);
        footer_grid_->set_grid_position(log_box_, {.row = 0, .col = 1});
    }

    std::shared_ptr<dcompframe::Panel> root_panel_ = std::make_shared<dcompframe::Panel>();
    std::shared_ptr<dcompframe::StackPanel> content_stack_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::GridPanel> top_grid_ = std::make_shared<dcompframe::GridPanel>(1, 2);
    std::shared_ptr<dcompframe::StackPanel> form_stack_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::GridPanel> option_grid_ = std::make_shared<dcompframe::GridPanel>(1, 2);
    std::shared_ptr<dcompframe::StackPanel> preview_stack_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::GridPanel> collections_grid_ = std::make_shared<dcompframe::GridPanel>(1, 2);
    std::shared_ptr<dcompframe::GridPanel> footer_grid_ = std::make_shared<dcompframe::GridPanel>(1, 2);
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
        return std::make_unique<LayoutValidationWindow>(render_manager, application, window_id);
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
