#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>

namespace dcompframe {

enum class RenderBackend {
    Simulated,
    DirectX
};

enum class ResourceType {
    Texture,
    Buffer,
    Unknown
};

struct RenderCapabilities {
    bool d3d11_ready = false;
    bool dcomp_ready = false;
    bool d2d_ready = false;
    bool dwrite_ready = false;
};

struct GpuResourceInfo {
    std::string name;
    ResourceType type = ResourceType::Unknown;
    std::size_t byte_size = 0;
};

class ResourceManager {
public:
    bool register_resource(std::string name, ResourceType type, std::size_t byte_size);
    bool release_resource(const std::string& name);
    void clear();

    [[nodiscard]] std::size_t resource_count() const;
    [[nodiscard]] std::size_t total_bytes() const;
    [[nodiscard]] bool contains(const std::string& name) const;

private:
    std::unordered_map<std::string, GpuResourceInfo> resources_;
};

class DeviceRecovery {
public:
    void notify_device_lost();
    bool try_recover();

    [[nodiscard]] bool is_device_lost() const;
    [[nodiscard]] int recover_count() const;

private:
    bool device_lost_ = false;
    int recover_count_ = 0;
};

enum class LogLevel {
    Info,
    Warning,
    Error
};

struct LogEntry {
    LogLevel level = LogLevel::Info;
    std::string message;
};

class DiagnosticsCenter {
public:
    void log(LogLevel level, std::string message);
    void record_frame(std::chrono::milliseconds frame_time);
    void record_commit();
    void update_resource_peak(std::size_t total_bytes);
    bool export_report(const std::filesystem::path& output) const;

    [[nodiscard]] std::size_t log_count() const;
    [[nodiscard]] std::size_t warning_count() const;
    [[nodiscard]] std::size_t error_count() const;
    [[nodiscard]] double average_frame_ms() const;
    [[nodiscard]] double frame_p95_ms() const;
    [[nodiscard]] double commits_per_second() const;
    [[nodiscard]] std::size_t peak_resource_bytes() const;

private:
    std::vector<LogEntry> logs_;
    std::vector<std::chrono::milliseconds> frame_times_;
    std::size_t commit_count_ = 0;
    std::chrono::steady_clock::time_point first_commit_time_ {};
    std::chrono::steady_clock::time_point last_commit_time_ {};
    std::size_t peak_resource_bytes_ = 0;
};

class RenderManager;

class CompositionBridge {
public:
    explicit CompositionBridge(RenderManager* owner);

    bool bind_target_handle(HWND hwnd);
    bool commit_changes(bool has_dirty_changes = true);

    [[nodiscard]] int commit_count() const;
    [[nodiscard]] HWND bound_hwnd() const;

private:
    RenderManager* owner_ = nullptr;
    HWND bound_hwnd_ = nullptr;
    int commit_count_ = 0;
};

class RenderManager {
public:
    bool initialize(bool simulate_device_available = true);
    bool initialize_with_backend(RenderBackend backend);
    void shutdown();

    [[nodiscard]] bool is_initialized() const;
    [[nodiscard]] RenderCapabilities capabilities() const;
    [[nodiscard]] RenderBackend backend() const;
    [[nodiscard]] CompositionBridge create_composition_bridge();
    [[nodiscard]] int total_commit_count() const;
    [[nodiscard]] ResourceManager& resource_manager();
    [[nodiscard]] const ResourceManager& resource_manager() const;
    [[nodiscard]] DeviceRecovery& device_recovery();
    [[nodiscard]] const DeviceRecovery& device_recovery() const;
    [[nodiscard]] DiagnosticsCenter& diagnostics();
    [[nodiscard]] const DiagnosticsCenter& diagnostics() const;

    void notify_commit();

private:
    bool initialized_ = false;
    RenderBackend backend_ = RenderBackend::Simulated;
    RenderCapabilities capabilities_ {};
    int total_commit_count_ = 0;
    ResourceManager resource_manager_ {};
    DeviceRecovery device_recovery_ {};
    DiagnosticsCenter diagnostics_ {};
};

}  // namespace dcompframe
