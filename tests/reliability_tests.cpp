#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "dcompframe/config/app_config.h"
#include "dcompframe/render_manager.h"

namespace dcompframe::tests {

TEST(ReliabilityTests, SoakBaselineLoopMaintainsConsistency) {
    RenderManager manager;
    ASSERT_TRUE(manager.initialize(true));

    auto bridge = manager.create_composition_bridge();
    ASSERT_TRUE(bridge.bind_target_handle(reinterpret_cast<HWND>(1)));

    constexpr int kIterations = 10000;
    for (int i = 0; i < kIterations; ++i) {
        ASSERT_TRUE(bridge.commit_changes(true));
    }

    EXPECT_EQ(manager.total_commit_count(), kIterations);
    EXPECT_GT(manager.diagnostics().average_frame_ms(), 0.0);
}

TEST(ReliabilityTests, ResourcePeakPatrolStaysBoundedAfterReleaseCycles) {
    RenderManager manager;
    ASSERT_TRUE(manager.initialize(true));

    auto bridge = manager.create_composition_bridge();
    ASSERT_TRUE(bridge.bind_target_handle(reinterpret_cast<HWND>(1)));

    for (int i = 0; i < 100; ++i) {
        const std::string name = "res-" + std::to_string(i);
        ASSERT_TRUE(manager.resource_manager().register_resource(name, ResourceType::Buffer, 4096));
        ASSERT_TRUE(bridge.commit_changes(true));
        ASSERT_TRUE(manager.resource_manager().release_resource(name));
        ASSERT_TRUE(bridge.commit_changes(true));
    }

    EXPECT_EQ(manager.resource_manager().resource_count(), 0U);
    EXPECT_GE(manager.diagnostics().peak_resource_bytes(), 4096U);
    EXPECT_LT(manager.diagnostics().peak_resource_bytes(), 4096U * 8U);
}

TEST(ReliabilityTests, FaultInjectionCoversDeviceLostConfigMissingAndCorruptJson) {
    RenderManager manager;
    ASSERT_TRUE(manager.initialize(true));
    auto bridge = manager.create_composition_bridge();
    ASSERT_TRUE(bridge.bind_target_handle(reinterpret_cast<HWND>(1)));

    manager.device_recovery().notify_device_lost();
    EXPECT_TRUE(manager.device_recovery().is_device_lost());
    EXPECT_TRUE(manager.device_recovery().try_recover());

    AppConfig config;
    const Status missing_status = AppConfigLoader::load_from_file("not-exists-config.json", config);
    EXPECT_FALSE(missing_status.ok());
    EXPECT_EQ(missing_status.code, ErrorCode::IOError);

    const auto broken_path = std::filesystem::temp_directory_path() / "dcompframe-broken-config.json";
    {
        std::ofstream file(broken_path);
        file << "{\"width\": 1024, \"height\":";
    }

    const Status broken_status = AppConfigLoader::load_from_file(broken_path.string(), config);
    EXPECT_FALSE(broken_status.ok());
    EXPECT_EQ(broken_status.code, ErrorCode::ParseError);

    std::error_code ec;
    std::filesystem::remove(broken_path, ec);
}

}  // namespace dcompframe::tests
