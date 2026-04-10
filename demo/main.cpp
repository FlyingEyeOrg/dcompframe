#include <fmt/format.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "dcompframe/application.h"
#include "dcompframe/binding/observable.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/layout/grid_panel.h"
#include "dcompframe/layout/stack_panel.h"

namespace {

struct DemoSectionMetrics {
    float top_showcase_height = 0.0F;
    float collections_height = 0.0F;
    float scroll_height = 0.0F;
    float footer_height = 0.0F;
    float rich_text_height = 0.0F;
    float text_block_height = 0.0F;
    float image_height = 0.0F;
};

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

DemoSectionMetrics compute_demo_section_metrics(const dcompframe::Size& size) {
    DemoSectionMetrics metrics;
    const float section_gap = 18.0F;
    const float available_height = max_value(0.0F, size.height - section_gap * 3.0F);
    metrics.top_showcase_height = clamp_value(available_height * 0.47F, 340.0F, 440.0F);
    metrics.collections_height = clamp_value(available_height * 0.18F, 132.0F, 176.0F);
    metrics.scroll_height = clamp_value(available_height * 0.18F, 126.0F, 164.0F);
    metrics.footer_height =
        available_height - metrics.top_showcase_height - metrics.collections_height - metrics.scroll_height;

    auto shrink_section = [](float& value, float minimum, float& deficit) {
        const float adjustable = max_value(0.0F, value - minimum);
        const float delta = min_value(adjustable, deficit);
        value -= delta;
        deficit -= delta;
    };

    if (metrics.footer_height < 112.0F) {
        float deficit = 112.0F - metrics.footer_height;
        shrink_section(metrics.top_showcase_height, 300.0F, deficit);
        shrink_section(metrics.scroll_height, 110.0F, deficit);
        shrink_section(metrics.collections_height, 118.0F, deficit);
        metrics.footer_height =
            available_height - metrics.top_showcase_height - metrics.collections_height - metrics.scroll_height;
    }

    if (metrics.top_showcase_height < 320.0F) {
        float deficit = 320.0F - metrics.top_showcase_height;
        shrink_section(metrics.footer_height, 96.0F, deficit);
        shrink_section(metrics.scroll_height, 110.0F, deficit);
        shrink_section(metrics.collections_height, 118.0F, deficit);
        metrics.top_showcase_height =
            available_height - metrics.footer_height - metrics.collections_height - metrics.scroll_height;
    }

    const float section_title_height = 18.0F;
    const float title_gap = 6.0F;
    const float stack_gap = 14.0F;
    const float editor_min_height = 136.0F;
    const float options_section_height = 82.0F;
    const float slider_section_height = section_title_height + title_gap + 40.0F;
    const float text_box_section_height = section_title_height + title_gap + 40.0F;
    const float rich_text_available = metrics.top_showcase_height - text_box_section_height - options_section_height
        - slider_section_height - stack_gap * 3.0F - section_title_height - title_gap;
    metrics.rich_text_height = max_value(editor_min_height, rich_text_available);

    const float right_fixed = (section_title_height + title_gap) * 2.0F + stack_gap * 2.0F;
    const float right_body_height = max_value(0.0F, metrics.top_showcase_height - right_fixed);
    metrics.text_block_height = clamp_value(right_body_height * 0.14F, 42.0F, 56.0F);
    metrics.image_height = clamp_value(right_body_height * 0.18F, 58.0F, 84.0F);
    return metrics;
}

class DemoWindow final : public dcompframe::Window {
public:
    DemoWindow(dcompframe::RenderManager* render_manager, dcompframe::Application* application, std::size_t window_id)
        : dcompframe::Window(render_manager, application, window_id) {}

protected:
    bool build(const dcompframe::AppConfig& config) override {
        (void)config;

        const dcompframe::Thickness section_title_margin {.left = 0.0F, .top = 24.0F, .right = 0.0F, .bottom = 0.0F};

        text_box_->set_placeholder("请输入窗口标题，支持复制粘贴、选区和键盘导航");
        text_box_->bind_text(binding_context_.title);
        binding_context_.title.set(fmt::format("Window {}", id()));
        text_box_->set_margin(section_title_margin);

        rich_text_box_->set_rich_text(
            "RichTextBox 示例: 这里现在支持直接编辑。\n"
            "你可以输入多行文本、回车换行，并观察光标、选区和自动换行表现。\n"
            "当前 Demo 会统一将非布局控件渲染为更接近 Element Plus 的浅色表单风格。\n"
            "窗口缩放时，表单区、列表区、卡片区和底部滚动区会一起自适应。");

        check_box_->set_checked((id() % 2U) == 1U);
        combo_box_->set_items({"Overview", "Diagnostics", "Editor", "Preview", "Settings", "Account", "Advanced Options"});
        combo_box_->set_selected_index(id() % combo_box_->items().size());
        combo_box_->set_margin(section_title_margin);
        slider_->set_range(0.0F, 100.0F);
        slider_->set_step(5.0F);
        slider_->set_value(25.0F + static_cast<float>((id() - 1U) % 4U) * 20.0F);
        slider_->set_margin(section_title_margin);

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
        items_control_->set_margin(section_title_margin);

        std::vector<std::string> scroll_items;
        scroll_items.reserve(48);
        for (int index = 1; index <= 48; ++index) {
            scroll_items.push_back(fmt::format("Scroll item {:02d} - 用于检查滚动条、裁剪区、文本对齐与 hover 呈现", index));
        }
        auto scroll_content = std::make_shared<dcompframe::ItemsControl>();
        scroll_content->set_items(std::move(scroll_items));
        scroll_viewer_->set_content(scroll_content);
        scroll_viewer_->set_margin(section_title_margin);

        list_view_->set_groups({
            dcompframe::ListGroup {.name = "Primary", .items = {"Layout", "Rendering", "Input", "Composition", "Typography", "Animation"}},
            dcompframe::ListGroup {.name = "Secondary", .items = {"Diagnostics", "Theme", "Assets", "Preview", "Scroll", "Tabs"}},
            dcompframe::ListGroup {.name = "Utility", .items = {"Card", "Popup", "Editor", "Tokens", "Status", "Expander"}},
        });
        list_view_->set_selected_index(2);
        list_view_->set_margin(section_title_margin);

        image_->set_source("demo://element-plus-placeholder");
        image_->set_margin(section_title_margin);
        progress_->set_range(0.0F, 100.0F);
        progress_->set_value(36.0F + static_cast<float>((id() % 4U) * 12U));
        progress_->set_indeterminate(false);
        loading_->set_active((id() % 2U) == 0U);
        loading_->set_overlay_mode(false);
        loading_->set_text("正在同步控件状态与合成帧...");
        log_box_->set_max_lines(120);
        log_box_->set_margin(section_title_margin);
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
        rich_text_box_->set_margin(section_title_margin);
        text_block_->set_margin(section_title_margin);

        root_stack_->add_child(top_showcase_grid_);
        root_stack_->add_child(collections_grid_);
        root_stack_->add_child(scroll_section_stack_);
        root_stack_->add_child(log_section_stack_);

        top_showcase_grid_->add_child(form_stack_);
        top_showcase_grid_->set_grid_position(form_stack_, {.row = 0, .col = 0});
        top_showcase_grid_->add_child(preview_stack_);
        top_showcase_grid_->set_grid_position(preview_stack_, {.row = 0, .col = 1});

        form_stack_->add_child(text_box_section_);
        form_stack_->add_child(editor_section_);
        form_stack_->add_child(option_grid_);
        form_stack_->add_child(slider_section_);
        text_box_section_->add_child(text_box_);
        editor_section_->add_child(rich_text_box_);
        option_grid_->add_child(check_box_section_);
        option_grid_->set_grid_position(check_box_section_, {.row = 0, .col = 0});
        option_grid_->add_child(combo_box_section_);
        option_grid_->set_grid_position(combo_box_section_, {.row = 0, .col = 1});
        check_box_section_->add_child(check_box_);
        combo_box_section_->add_child(combo_box_);
        slider_section_->add_child(slider_);

        preview_stack_->add_child(text_block_section_);
        preview_stack_->add_child(image_section_);
        preview_stack_->add_child(card_section_);
        text_block_section_->add_child(text_block_);
        image_section_->add_child(image_);
        card_section_->add_child(card_);

        collections_grid_->add_child(list_section_stack_);
        collections_grid_->set_grid_position(list_section_stack_, {.row = 0, .col = 0});
        collections_grid_->add_child(items_section_stack_);
        collections_grid_->set_grid_position(items_section_stack_, {.row = 0, .col = 1});
        list_section_stack_->add_child(list_view_);
        items_section_stack_->add_child(items_control_);

        scroll_section_stack_->add_child(scroll_viewer_);
        log_section_stack_->add_child(log_box_);

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

        set_root(root_stack_);
        set_arrange_handler([this](const dcompframe::Size& size) {
            arrange_demo_sections(size);
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
    void arrange_demo_sections(const dcompframe::Size& size) {
        const DemoSectionMetrics metrics = compute_demo_section_metrics(size);
        const float section_gap = 18.0F;
        const float container_gap = 14.0F;
        const float title_gap = 6.0F;
        const float section_title_height = 18.0F;

        root_stack_->set_spacing(section_gap);
        root_stack_->set_desired_size(size);
        top_showcase_grid_->set_desired_size({size.width, metrics.top_showcase_height});
        collections_grid_->set_desired_size({size.width, metrics.collections_height});
        scroll_section_stack_->set_desired_size({size.width, metrics.scroll_height});
        log_section_stack_->set_desired_size({size.width, metrics.footer_height});
        root_stack_->arrange(size);

        const auto top_bounds = top_showcase_grid_->bounds();
        const auto collections_bounds = collections_grid_->bounds();
        const auto scroll_bounds = scroll_section_stack_->bounds();
        const auto log_bounds = log_section_stack_->bounds();

        top_showcase_grid_->arrange({top_bounds.width, top_bounds.height});
        collections_grid_->arrange({collections_bounds.width, collections_bounds.height});

        const auto form_bounds = form_stack_->bounds();
        const auto preview_bounds = preview_stack_->bounds();
        form_stack_->set_spacing(container_gap);
        preview_stack_->set_spacing(container_gap);
        form_stack_->set_desired_size({form_bounds.width, form_bounds.height});
        preview_stack_->set_desired_size({preview_bounds.width, preview_bounds.height});

        text_box_section_->set_desired_size({form_bounds.width, section_title_height + title_gap + 40.0F});
        editor_section_->set_desired_size({form_bounds.width, section_title_height + title_gap + metrics.rich_text_height});
        option_grid_->set_desired_size({form_bounds.width, 82.0F});
        slider_section_->set_desired_size({form_bounds.width, section_title_height + title_gap + 40.0F});
        text_box_->set_desired_size({form_bounds.width, 40.0F});
        rich_text_box_->set_desired_size({form_bounds.width, metrics.rich_text_height});
        slider_->set_desired_size({form_bounds.width, 40.0F});
        form_stack_->arrange({form_bounds.width, form_bounds.height});

        const auto option_bounds = option_grid_->bounds();
        option_grid_->arrange({option_bounds.width, option_bounds.height});
        const auto check_bounds = check_box_section_->bounds();
        const auto combo_bounds = combo_box_section_->bounds();
        check_box_->set_desired_size({check_bounds.width, 40.0F});
        combo_box_->set_desired_size({combo_bounds.width, 40.0F});
        check_box_section_->set_desired_size({check_bounds.width, check_bounds.height});
        combo_box_section_->set_desired_size({combo_bounds.width, combo_bounds.height});
        check_box_section_->arrange({check_bounds.width, check_bounds.height});
        combo_box_section_->arrange({combo_bounds.width, combo_bounds.height});

        text_box_section_->arrange({text_box_section_->bounds().width, text_box_section_->bounds().height});
        editor_section_->arrange({editor_section_->bounds().width, editor_section_->bounds().height});
        slider_section_->arrange({slider_section_->bounds().width, slider_section_->bounds().height});

        text_block_section_->set_desired_size({preview_bounds.width, section_title_height + title_gap + metrics.text_block_height});
        image_section_->set_desired_size({preview_bounds.width, section_title_height + title_gap + metrics.image_height});
        const float preview_card_height = max_value(
            220.0F,
            preview_bounds.height - (section_title_height + title_gap + metrics.text_block_height)
                - (section_title_height + title_gap + metrics.image_height) - container_gap * 2.0F);
        card_section_->set_desired_size({preview_bounds.width, preview_card_height});
        text_block_->set_desired_size({preview_bounds.width, metrics.text_block_height});
        image_->set_desired_size({preview_bounds.width, metrics.image_height});
        card_->set_desired_size({preview_bounds.width, preview_card_height});
        preview_stack_->arrange({preview_bounds.width, preview_bounds.height});
        text_block_section_->arrange({text_block_section_->bounds().width, text_block_section_->bounds().height});
        image_section_->arrange({image_section_->bounds().width, image_section_->bounds().height});
        card_section_->arrange({card_section_->bounds().width, card_section_->bounds().height});

        list_section_stack_->set_desired_size({list_section_stack_->bounds().width, collections_bounds.height});
        items_section_stack_->set_desired_size({items_section_stack_->bounds().width, collections_bounds.height});
        list_view_->set_desired_size({list_section_stack_->bounds().width, collections_bounds.height - section_title_height - title_gap});
        items_control_->set_desired_size({items_section_stack_->bounds().width, collections_bounds.height - section_title_height - title_gap});
        list_section_stack_->arrange({list_section_stack_->bounds().width, list_section_stack_->bounds().height});
        items_section_stack_->arrange({items_section_stack_->bounds().width, items_section_stack_->bounds().height});
        scroll_viewer_->set_desired_size({scroll_bounds.width, metrics.scroll_height - section_title_height - title_gap});
        log_box_->set_desired_size({log_bounds.width, metrics.footer_height - section_title_height - title_gap});
        scroll_section_stack_->arrange({scroll_bounds.width, scroll_bounds.height});
        log_section_stack_->arrange({log_bounds.width, log_bounds.height});
    }

    dcompframe::BindingContext binding_context_ {};
    std::shared_ptr<dcompframe::StackPanel> root_stack_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::GridPanel> top_showcase_grid_ = std::make_shared<dcompframe::GridPanel>(1, 2);
    std::shared_ptr<dcompframe::StackPanel> form_stack_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::StackPanel> text_box_section_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::StackPanel> editor_section_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::GridPanel> option_grid_ = std::make_shared<dcompframe::GridPanel>(1, 2);
    std::shared_ptr<dcompframe::StackPanel> check_box_section_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::StackPanel> combo_box_section_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::StackPanel> slider_section_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::StackPanel> preview_stack_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::StackPanel> text_block_section_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::StackPanel> image_section_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::StackPanel> card_section_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::GridPanel> collections_grid_ = std::make_shared<dcompframe::GridPanel>(1, 2);
    std::shared_ptr<dcompframe::StackPanel> list_section_stack_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::StackPanel> items_section_stack_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::StackPanel> scroll_section_stack_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
    std::shared_ptr<dcompframe::StackPanel> log_section_stack_ =
        std::make_shared<dcompframe::StackPanel>(dcompframe::Orientation::Vertical);
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
