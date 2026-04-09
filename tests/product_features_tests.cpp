#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>

#include "dcompframe/binding/observable.h"
#include "dcompframe/config/app_config.h"
#include "dcompframe/controls/controls.h"
#include "dcompframe/controls/style.h"
#include "dcompframe/input/input_manager.h"
#include "dcompframe/render_manager.h"

namespace dcompframe::tests {

TEST(ThemeTests, BuiltinPalettesWork) {
    const auto dark = Theme::make_dark();
    const auto light = Theme::make_light();
    const auto brand = Theme::make_brand();

    EXPECT_EQ(dark.active_palette(), "dark");
    EXPECT_EQ(light.active_palette(), "light");
    EXPECT_EQ(brand.active_palette(), "brand");

    EXPECT_NE(dark.resolve("card").background, light.resolve("card").background);
}

TEST(BindingTests, CardAndTextBoxBindingsUpdateState) {
    BindingContext context;
    Card card;
    TextBox text_box;

    card.bind(context);
    text_box.bind_text(context.title);

    context.title.set("Title A");
    context.body.set("Body A");

    EXPECT_EQ(card.title(), "Title A");
    EXPECT_EQ(card.body(), "Body A");
    EXPECT_EQ(text_box.text(), "Title A");
}

TEST(ControlExtensionTests, AdditionalControlsStoreAndExposeState) {
    ListView list_view;
    list_view.set_items({"A", "B", "C"});
    list_view.set_selected_index(1);
    ASSERT_TRUE(list_view.selected_index().has_value());
    EXPECT_EQ(*list_view.selected_index(), 1U);
    list_view.set_groups({
        ListGroup {.name = "G1", .items = {"A", "B"}},
        ListGroup {.name = "G2", .items = {"C", "D"}},
    });
    const auto visible = list_view.visible_range(8.0F, 24.0F, 8.0F);
    EXPECT_EQ(visible.first, 1U);
    EXPECT_GE(visible.second, visible.first);

    ScrollViewer scroll;
    scroll.set_scroll_offset(10.0F, 20.0F);
    EXPECT_FLOAT_EQ(scroll.scroll_offset().x, 10.0F);
    EXPECT_FLOAT_EQ(scroll.scroll_offset().y, 20.0F);
    scroll.set_inertia_velocity(0.2F, 0.1F);
    scroll.tick_inertia(std::chrono::milliseconds {10});
    EXPECT_GT(scroll.scroll_offset().x, 10.0F);
    EXPECT_GT(scroll.scroll_offset().y, 20.0F);

    CheckBox check;
    check.set_checked(true);
    EXPECT_TRUE(check.checked());
    EXPECT_EQ(check.state(), ControlState::Selected);

    Slider slider;
    slider.set_range(0.0F, 1.0F);
    slider.set_value(3.5F);
    EXPECT_FLOAT_EQ(slider.value(), 1.0F);
}

TEST(InputManagerTests, FocusDoubleClickAndDragAreHandled) {
    auto root = std::make_shared<UIElement>("root");
    auto a = std::make_shared<Button>("a");
    auto b = std::make_shared<Button>("b");
    ASSERT_TRUE(root->add_child(a));
    ASSERT_TRUE(root->add_child(b));

    InputManager input;
    input.set_focus_ring_root(root);

    int click_count = 0;
    int double_click_count = 0;
    int shortcut_count = 0;
    Point drag_delta {};

    input.set_click_handler([&](UIElement&) { ++click_count; });
    input.set_double_click_handler([&](UIElement&) { ++double_click_count; });
    input.set_drag_handler([&](UIElement&, Point delta) { drag_delta = delta; });
    input.register_shortcut('S', true, false, false, [&shortcut_count] { ++shortcut_count; });

    input.focus_next();
    EXPECT_EQ(input.focused_element(), a);
    input.focus_next();
    EXPECT_EQ(input.focused_element(), b);

    input.on_mouse_down(a, Point {.x = 1.0F, .y = 2.0F});
    input.on_mouse_down(a, Point {.x = 1.0F, .y = 2.0F});
    EXPECT_GE(click_count + double_click_count, 1);
    EXPECT_GE(double_click_count, 1);

    input.on_mouse_down(b, Point {.x = 2.0F, .y = 2.0F});
    input.on_mouse_move(Point {.x = 9.0F, .y = 11.0F});
    input.on_mouse_up(Point {.x = 9.0F, .y = 11.0F});
    EXPECT_FLOAT_EQ(drag_delta.x, 7.0F);
    EXPECT_FLOAT_EQ(drag_delta.y, 9.0F);
    EXPECT_TRUE(input.on_key_down('S', true, false, false));
    EXPECT_EQ(shortcut_count, 1);
}

TEST(TextBoxTests, CompositionAndSelectionWorkflow) {
    TextBox text_box;
    text_box.set_text("abcd");
    text_box.set_selection(1, 3);
    text_box.set_composition_text("XYZ");
    text_box.commit_composition();

    EXPECT_EQ(text_box.text(), "aXYZd");
    EXPECT_TRUE(text_box.composition_text().empty());
    const auto [start, end] = text_box.selection();
    EXPECT_EQ(start, end);
}

TEST(RenderManagerTests, BackendRegistryAndCommandBatchingWork) {
    RenderManager manager;
    const auto backends = manager.supported_backends();
    EXPECT_GE(backends.size(), 4U);

    manager.enqueue_command(RenderCommand {.type = RenderCommandType::Clear, .payload = "c0"});
    manager.enqueue_command(RenderCommand {.type = RenderCommandType::Commit, .payload = "c1"});
    auto drained = manager.drain_commands();
    ASSERT_EQ(drained.size(), 2U);
    EXPECT_EQ(drained[0].payload, "c0");
    EXPECT_EQ(drained[1].payload, "c1");
}

TEST(ConfigTests, JsonConfigCanBeLoaded) {
    const std::filesystem::path config_path = std::filesystem::temp_directory_path() / "dcompframe-config-test.json";

    {
        std::ofstream file(config_path);
        file << R"({
            "width": 1440,
            "height": 900,
            "use_directx_backend": true,
            "theme_name": "brand",
            "card_style": {
                "corner_radius": 22.0,
                "border_thickness": 2.0
            }
        })";
    }

    AppConfig config;
    const Status status = AppConfigLoader::load_from_file(config_path.string(), config);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.width, 1440);
    EXPECT_EQ(config.height, 900);
    EXPECT_TRUE(config.use_directx_backend);
    EXPECT_EQ(config.theme_name, "brand");
    EXPECT_FLOAT_EQ(config.card_style.corner_radius, 22.0F);

    std::error_code ec;
    std::filesystem::remove(config_path, ec);
}

TEST(DiagnosticsTests, ExportReportAndMetricsWork) {
    RenderManager manager;
    ASSERT_TRUE(manager.initialize(true));

    auto bridge = manager.create_composition_bridge();
    ASSERT_TRUE(bridge.bind_target_handle(reinterpret_cast<HWND>(1)));

    manager.resource_manager().register_resource("rt", ResourceType::Texture, 8192);
    ASSERT_TRUE(bridge.commit_changes(true));

    const auto& diagnostics = manager.diagnostics();
    EXPECT_GT(diagnostics.average_frame_ms(), 0.0);
    EXPECT_GE(diagnostics.frame_p95_ms(), diagnostics.average_frame_ms());
    EXPECT_GE(diagnostics.commits_per_second(), 0.0);
    EXPECT_EQ(diagnostics.peak_resource_bytes(), 8192U);

    const std::filesystem::path report_path = std::filesystem::temp_directory_path() / "dcompframe-report.json";
    EXPECT_TRUE(diagnostics.export_report(report_path));
    EXPECT_TRUE(std::filesystem::exists(report_path));

    std::error_code ec;
    std::filesystem::remove(report_path, ec);
}

}  // namespace dcompframe::tests
