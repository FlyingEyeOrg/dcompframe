#include <fmt/format.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "dcompframe/application.h"
#include "dcompframe/binding/observable.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/layout/grid_panel.h"

namespace {

class DemoWindow final : public dcompframe::Window {
public:
    DemoWindow(dcompframe::RenderManager* render_manager, dcompframe::Application* application, std::size_t window_id)
        : dcompframe::Window(render_manager, application, window_id) {}

protected:
    bool build(const dcompframe::AppConfig& config) override {
        (void)config;

        text_box_->set_placeholder("请输入窗口标题，支持复制粘贴、选区和键盘导航");
        text_box_->bind_text(binding_context_.title);
        binding_context_.title.set(fmt::format("Window {}", id()));

        rich_text_box_->set_rich_text(
            "RichTextBox 示例: 这里现在支持直接编辑。\n"
            "你可以输入多行文本、回车换行，并观察光标、选区和自动换行表现。\n"
            "当前 Demo 会统一将非布局控件渲染为更接近 Element Plus 的浅色表单风格。\n"
            "窗口缩放时，表单区、列表区、卡片区和底部滚动区会一起自适应。");

        check_box_->set_checked((id() % 2U) == 1U);
        combo_box_->set_items({"Overview", "Diagnostics", "Editor", "Preview", "Settings", "Account", "Advanced Options"});
        combo_box_->set_selected_index(id() % combo_box_->items().size());
        slider_->set_range(0.0F, 100.0F);
        slider_->set_step(5.0F);
        slider_->set_value(25.0F + static_cast<float>((id() - 1U) % 4U) * 20.0F);

        items_control_->set_items({
            "Alpha capability",
            "Beta diagnostics",
            "Gamma animation",
            "Delta layout token",
            "Epsilon state sync",
            "Zeta typography",
            "Eta popup layering",
            "Theta responsive panel",
            "Iota virtual viewport",
            "Kappa action footer",
            "Lambda tab switching",
            "Mu expander details",
            "Nu scrollbar track click",
            "Xi stacked sections",
        });
        items_control_->set_selected_index((id() - 1U) % items_control_->items().size());
        items_control_->set_item_spacing(6.0F);

        std::vector<std::string> scroll_items;
        scroll_items.reserve(48);
        for (int index = 1; index <= 48; ++index) {
            scroll_items.push_back(fmt::format("Scroll item {:02d} - 用于检查滚动条、裁剪区、文本对齐与 hover 呈现", index));
        }
        auto scroll_content = std::make_shared<dcompframe::ItemsControl>();
        scroll_content->set_items(std::move(scroll_items));
        scroll_viewer_->set_content(scroll_content);

        list_view_->set_groups({
            dcompframe::ListGroup {.name = "Primary", .items = {"Layout", "Rendering", "Input", "Composition", "Typography", "Animation"}},
            dcompframe::ListGroup {.name = "Secondary", .items = {"Diagnostics", "Theme", "Assets", "Preview", "Scroll", "Tabs"}},
            dcompframe::ListGroup {.name = "Utility", .items = {"Card", "Popup", "Editor", "Tokens", "Status", "Expander"}},
        });
        list_view_->set_selected_index(2);

        image_->set_source("demo://element-plus-placeholder");
        progress_->set_range(0.0F, 100.0F);
        progress_->set_value(36.0F + static_cast<float>((id() % 4U) * 12U));
        progress_->set_indeterminate(false);
        loading_->set_active((id() % 2U) == 0U);
        loading_->set_overlay_mode(false);
        loading_->set_text("正在同步控件状态与合成帧...");
        log_box_->set_max_lines(120);
        log_box_->set_lines({
            fmt::format("[window:{}] Demo 初始化完成", id()),
            "[layout] demo 内容由示例层创建",
            "[tabs] 页签区域启用独立正文区",
            "[scroll] 滚动条支持轨道点击与聚焦态",
            "[perf] 动画改为节流驱动，避免消息阻塞卡顿",
        });
        tab_control_->set_tabs({"概览", "动画", "数据", "诊断"});
        tab_control_->set_selected_index((id() - 1U) % 4U);
        popup_->set_title("状态弹层");
        popup_->set_body("用于演示 Popup/Modal 语义与层级展示。\nEsc 可关闭临时 UI。\n当前为示例展示模式。");
        popup_->set_modal(true);
        popup_->set_open((id() % 2U) == 1U);
        expander_->set_header("更多控件说明");
        expander_->set_content_text("点击标题可展开或收起。\n收起时不再占用正文布局空间。\n滚动条轨道支持直接点击跳转，并显示聚焦态。\n右侧预览区包含 TabControl、Loading、Progress 与日志框示例。");
        expander_->set_expanded(true);
        card_->set_title("Element Plus Card");
        card_->set_body("用于展示卡片、页签、动画、进度与展开区的组合布局。当前改为更清晰的纵向分区，便于在滚动时保持标题稳定。");
        card_->set_icon("picture");
        card_->set_tags({"Preview", "Tabs", "Animation"});
        card_->set_primary_action(primary_button_);

        root_grid_->set_rows_cols(12, 2);
        root_grid_->add_child(text_box_);
        root_grid_->set_grid_position(text_box_, {.row = 0, .col = 0});
        root_grid_->add_child(rich_text_box_);
        root_grid_->set_grid_position(rich_text_box_, {.row = 1, .col = 0, .row_span = 3});
        root_grid_->add_child(check_box_);
        root_grid_->set_grid_position(check_box_, {.row = 4, .col = 0});
        root_grid_->add_child(combo_box_);
        root_grid_->set_grid_position(combo_box_, {.row = 5, .col = 0});
        root_grid_->add_child(slider_);
        root_grid_->set_grid_position(slider_, {.row = 6, .col = 0});
        root_grid_->add_child(text_block_);
        root_grid_->set_grid_position(text_block_, {.row = 0, .col = 1});
        root_grid_->add_child(image_);
        root_grid_->set_grid_position(image_, {.row = 1, .col = 1, .row_span = 2});
        root_grid_->add_child(card_);
        root_grid_->set_grid_position(card_, {.row = 3, .col = 1, .row_span = 2});
        root_grid_->add_child(list_view_);
        root_grid_->set_grid_position(list_view_, {.row = 5, .col = 1, .row_span = 2});
        root_grid_->add_child(items_control_);
        root_grid_->set_grid_position(items_control_, {.row = 7, .col = 1, .row_span = 2});
        root_grid_->add_child(scroll_viewer_);
        root_grid_->set_grid_position(scroll_viewer_, {.row = 9, .col = 0, .row_span = 3, .col_span = 2});

        primary_button_->set_on_click([this] {
            if (application() != nullptr) {
                application()->render_manager().diagnostics().log(dcompframe::LogLevel::Info, fmt::format("Create window requested from window {}", id()));
            }
        });
        primary_button_->bind_enabled(binding_context_.enabled);
        binding_context_.enabled.set(true);

        rich_text_box_->set_rich_text(rich_text_box_->rich_text() + fmt::format("\n\nWindow {} 已准备就绪。", id()));

        text_box_->set_on_text_changed([this](const std::string& value) {
            if (application() != nullptr) {
                application()->render_manager().diagnostics().log(dcompframe::LogLevel::Info, fmt::format("Window {} title edited: {}", id(), value));
            }
            log_box_->append_line(fmt::format("[input] 标题更新为: {}", value));
            host().request_render();
        });
        check_box_->set_on_checked_changed([this](bool checked) {
            if (application() != nullptr) {
                application()->render_manager().diagnostics().log(dcompframe::LogLevel::Info, fmt::format("Window {} checkbox={}", id(), checked));
            }
            log_box_->append_line(fmt::format("[toggle] CheckBox = {}", checked ? "true" : "false"));
            host().request_render();
        });
        combo_box_->set_on_selection_changed([this](std::optional<std::size_t> index, const std::string& value) {
            if (application() != nullptr) {
                application()->render_manager().diagnostics().log(
                    dcompframe::LogLevel::Info,
                    fmt::format("Window {} combo index={} value={}", id(), index ? static_cast<int>(*index) : -1, value));
            }
            log_box_->append_line(fmt::format("[combo] index={} value={}", index ? static_cast<int>(*index) : -1, value));
            host().request_render();
        });
        slider_->set_on_value_changed([this](float value) {
            if (application() != nullptr) {
                application()->render_manager().diagnostics().log(dcompframe::LogLevel::Info, fmt::format("Window {} slider={:.2f}", id(), value));
            }
            progress_->set_value(value);
            loading_->set_active(value < 100.0F);
            log_box_->append_line(fmt::format("[slider] value={:.0f}", value));
            host().request_render();
        });

        set_root(root_grid_);
        set_arrange_handler([this](const dcompframe::Size& size) {
            root_grid_->arrange(size);
        });
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
        return std::wstring(L"DCompFrame Demo - Window ") + std::to_wstring(id());
    }

private:
    dcompframe::BindingContext binding_context_ {};
    std::shared_ptr<dcompframe::GridPanel> root_grid_ = std::make_shared<dcompframe::GridPanel>(12, 2);
    std::shared_ptr<dcompframe::Button> primary_button_ = std::make_shared<dcompframe::Button>("新建窗口");
    std::shared_ptr<dcompframe::TextBox> text_box_ = std::make_shared<dcompframe::TextBox>();
    std::shared_ptr<dcompframe::RichTextBox> rich_text_box_ = std::make_shared<dcompframe::RichTextBox>();
    std::shared_ptr<dcompframe::CheckBox> check_box_ = std::make_shared<dcompframe::CheckBox>();
    std::shared_ptr<dcompframe::ComboBox> combo_box_ = std::make_shared<dcompframe::ComboBox>();
    std::shared_ptr<dcompframe::Slider> slider_ = std::make_shared<dcompframe::Slider>();
    std::shared_ptr<dcompframe::ScrollViewer> scroll_viewer_ = std::make_shared<dcompframe::ScrollViewer>();
    std::shared_ptr<dcompframe::ItemsControl> items_control_ = std::make_shared<dcompframe::ItemsControl>();
    std::shared_ptr<dcompframe::ListView> list_view_ = std::make_shared<dcompframe::ListView>();
    std::shared_ptr<dcompframe::TextBlock> text_block_ = std::make_shared<dcompframe::TextBlock>("StackPanel 式纵向示例布局，标题固定，内容区独立滚动。");
    std::shared_ptr<dcompframe::Label> label_ = std::make_shared<dcompframe::Label>("状态：动画预览中");
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
    app.set_window_factory([](dcompframe::RenderManager* render_manager, dcompframe::Application* application, std::size_t window_id) {
        return std::make_unique<DemoWindow>(render_manager, application, window_id);
    });
    if (!app.initialize("demo/demo-config.json")) {
        return 1;
    }

    app.render_manager().resource_manager().register_resource("main-card-texture", dcompframe::ResourceType::Texture, 2048 * 2048 * 4);
    app.render_manager().resource_manager().register_resource("ui-vertex-buffer", dcompframe::ResourceType::Buffer, 1024 * 64);
    if (!app.create_window()) {
        return 1;
    }

    return app.run();
}
