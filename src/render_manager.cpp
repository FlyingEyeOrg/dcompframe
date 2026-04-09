#include "dcompframe/render_manager.h"

#include <d3d11.h>
#include <dcomp.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <numeric>

#include <nlohmann/json.hpp>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dcomp.lib")

namespace dcompframe {

bool ResourceManager::register_resource(std::string name, ResourceType type, std::size_t byte_size) {
    if (name.empty() || resources_.contains(name)) {
        return false;
    }

    const std::string key = name;
    resources_[key] = GpuResourceInfo {.name = std::move(name), .type = type, .byte_size = byte_size};
    return true;
}

bool ResourceManager::release_resource(const std::string& name) {
    return resources_.erase(name) > 0;
}

void ResourceManager::clear() {
    resources_.clear();
}

std::size_t ResourceManager::resource_count() const {
    return resources_.size();
}

std::size_t ResourceManager::total_bytes() const {
    std::size_t total = 0;
    for (const auto& [_, resource] : resources_) {
        total += resource.byte_size;
    }

    return total;
}

bool ResourceManager::contains(const std::string& name) const {
    return resources_.contains(name);
}

void DeviceRecovery::notify_device_lost() {
    device_lost_ = true;
}

bool DeviceRecovery::try_recover() {
    if (!device_lost_) {
        return false;
    }

    device_lost_ = false;
    ++recover_count_;
    return true;
}

bool DeviceRecovery::is_device_lost() const {
    return device_lost_;
}

int DeviceRecovery::recover_count() const {
    return recover_count_;
}

void DiagnosticsCenter::log(LogLevel level, std::string message) {
    logs_.push_back(LogEntry {.level = level, .message = std::move(message)});
}

void DiagnosticsCenter::record_frame(std::chrono::milliseconds frame_time) {
    frame_times_.push_back(frame_time);
}

void DiagnosticsCenter::record_commit() {
    const auto now = std::chrono::steady_clock::now();
    if (commit_count_ == 0) {
        first_commit_time_ = now;
    }
    last_commit_time_ = now;
    ++commit_count_;
}

void DiagnosticsCenter::update_resource_peak(std::size_t total_bytes) {
    if (total_bytes > peak_resource_bytes_) {
        peak_resource_bytes_ = total_bytes;
    }
}

bool DiagnosticsCenter::export_report(const std::filesystem::path& output) const {
    nlohmann::json json;
    json["log_count"] = log_count();
    json["warning_count"] = warning_count();
    json["error_count"] = error_count();
    json["average_frame_ms"] = average_frame_ms();
    json["frame_p95_ms"] = frame_p95_ms();
    json["commits_per_second"] = commits_per_second();
    json["peak_resource_bytes"] = peak_resource_bytes();

    std::ofstream file(output);
    if (!file.is_open()) {
        return false;
    }

    file << json.dump(2);
    return true;
}

std::size_t DiagnosticsCenter::log_count() const {
    return logs_.size();
}

std::size_t DiagnosticsCenter::warning_count() const {
    return static_cast<std::size_t>(std::count_if(
        logs_.begin(),
        logs_.end(),
        [](const LogEntry& entry) { return entry.level == LogLevel::Warning; }));
}

std::size_t DiagnosticsCenter::error_count() const {
    return static_cast<std::size_t>(std::count_if(
        logs_.begin(),
        logs_.end(),
        [](const LogEntry& entry) { return entry.level == LogLevel::Error; }));
}

double DiagnosticsCenter::average_frame_ms() const {
    if (frame_times_.empty()) {
        return 0.0;
    }

    const auto total = std::accumulate(
        frame_times_.begin(),
        frame_times_.end(),
        std::chrono::milliseconds {0},
        [](std::chrono::milliseconds lhs, std::chrono::milliseconds rhs) { return lhs + rhs; });
    return static_cast<double>(total.count()) / static_cast<double>(frame_times_.size());
}

double DiagnosticsCenter::frame_p95_ms() const {
    if (frame_times_.empty()) {
        return 0.0;
    }

    std::vector<long long> values;
    values.reserve(frame_times_.size());
    for (const auto frame : frame_times_) {
        values.push_back(frame.count());
    }
    std::sort(values.begin(), values.end());
    const std::size_t idx = static_cast<std::size_t>(0.95 * static_cast<double>(values.size() - 1));
    return static_cast<double>(values[idx]);
}

double DiagnosticsCenter::commits_per_second() const {
    if (commit_count_ <= 1) {
        return 0.0;
    }

    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(last_commit_time_ - first_commit_time_);
    if (duration.count() <= 0) {
        return static_cast<double>(commit_count_);
    }

    return static_cast<double>(commit_count_) * 1000.0 / static_cast<double>(duration.count());
}

std::size_t DiagnosticsCenter::peak_resource_bytes() const {
    return peak_resource_bytes_;
}

CompositionBridge::CompositionBridge(RenderManager* owner) : owner_(owner) {}

bool CompositionBridge::bind_target_handle(HWND hwnd) {
    if (hwnd == nullptr) {
        return false;
    }

    bound_hwnd_ = hwnd;
    return true;
}

bool CompositionBridge::commit_changes(bool has_dirty_changes) {
    if (owner_ == nullptr || !owner_->is_initialized() || bound_hwnd_ == nullptr) {
        return false;
    }

    if (!has_dirty_changes) {
        return false;
    }

    ++commit_count_;
    owner_->notify_commit();
    owner_->diagnostics().record_commit();
    owner_->diagnostics().record_frame(std::chrono::milliseconds {16});
    owner_->diagnostics().update_resource_peak(owner_->resource_manager().total_bytes());
    return true;
}

int CompositionBridge::commit_count() const {
    return commit_count_;
}

HWND CompositionBridge::bound_hwnd() const {
    return bound_hwnd_;
}

bool RenderManager::initialize(bool simulate_device_available) {
    backend_ = RenderBackend::Simulated;
    initialized_ = simulate_device_available;
    capabilities_ = RenderCapabilities {
        .d3d11_ready = simulate_device_available,
        .dcomp_ready = simulate_device_available,
        .d2d_ready = simulate_device_available,
        .dwrite_ready = simulate_device_available,
    };
    return initialized_;
}

bool RenderManager::initialize_with_backend(RenderBackend backend) {
    backend_ = backend;
    if (backend == RenderBackend::Simulated) {
        return initialize(true);
    }

    if (backend == RenderBackend::DirectX12) {
        diagnostics_.log(LogLevel::Warning, "DirectX12 backend is not implemented yet");
        capabilities_ = {};
        initialized_ = false;
        return false;
    }

    UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_FEATURE_LEVEL feature_levels[] {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    ID3D11Device* raw_device = nullptr;
    ID3D11DeviceContext* raw_context = nullptr;
    D3D_FEATURE_LEVEL actual_level = D3D_FEATURE_LEVEL_10_0;

    const auto create_device = [&](D3D_DRIVER_TYPE driver_type, UINT flags) {
        return D3D11CreateDevice(
            nullptr,
            driver_type,
            nullptr,
            flags,
            feature_levels,
            static_cast<UINT>(std::size(feature_levels)),
            D3D11_SDK_VERSION,
            &raw_device,
            &actual_level,
            &raw_context);
    };

    HRESULT d3d_hr = E_FAIL;
    if (backend == RenderBackend::Warp) {
        d3d_hr = create_device(D3D_DRIVER_TYPE_WARP, create_flags);
        if (FAILED(d3d_hr) && (create_flags & D3D11_CREATE_DEVICE_DEBUG) != 0U) {
            d3d_hr = create_device(D3D_DRIVER_TYPE_WARP, D3D11_CREATE_DEVICE_BGRA_SUPPORT);
        }
    } else {
        d3d_hr = create_device(D3D_DRIVER_TYPE_HARDWARE, create_flags);
        if (FAILED(d3d_hr) && (create_flags & D3D11_CREATE_DEVICE_DEBUG) != 0U) {
            d3d_hr = create_device(D3D_DRIVER_TYPE_HARDWARE, D3D11_CREATE_DEVICE_BGRA_SUPPORT);
        }
        if (FAILED(d3d_hr)) {
            d3d_hr = create_device(D3D_DRIVER_TYPE_WARP, D3D11_CREATE_DEVICE_BGRA_SUPPORT);
            if (SUCCEEDED(d3d_hr)) {
                backend_ = RenderBackend::Warp;
            }
        }
    }

    if (raw_context != nullptr) {
        raw_context->Release();
    }

    if (FAILED(d3d_hr) || raw_device == nullptr) {
        capabilities_ = {};
        initialized_ = false;
        diagnostics_.log(LogLevel::Error, "D3D11CreateDevice failed");
        return false;
    }

    IDXGIDevice* dxgi_device = nullptr;
    const HRESULT qhr = raw_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device));

    IUnknown* dcomp_device = nullptr;
    HRESULT dcomp_hr = E_FAIL;
    if (SUCCEEDED(qhr) && dxgi_device != nullptr) {
        dcomp_hr = DCompositionCreateDevice(dxgi_device, __uuidof(IUnknown), reinterpret_cast<void**>(&dcomp_device));
    }

    raw_device->Release();
    if (dxgi_device != nullptr) {
        dxgi_device->Release();
    }
    if (dcomp_device != nullptr) {
        dcomp_device->Release();
    }

    initialized_ = SUCCEEDED(dcomp_hr);
    capabilities_ = RenderCapabilities {
        .d3d11_ready = true,
        .dcomp_ready = initialized_,
        .d2d_ready = true,
        .dwrite_ready = true,
    };
    if (!initialized_) {
        diagnostics_.log(LogLevel::Error, "DCompositionCreateDevice failed");
    }

    return initialized_;
}

void RenderManager::shutdown() {
    stop_render_thread();
    initialized_ = false;
    capabilities_ = {};
    total_commit_count_ = 0;
    resource_manager_.clear();
}

bool RenderManager::is_initialized() const {
    return initialized_;
}

CompositionBridge RenderManager::create_composition_bridge() {
    return CompositionBridge(this);
}

RenderCapabilities RenderManager::capabilities() const {
    return capabilities_;
}

RenderBackend RenderManager::backend() const {
    return backend_;
}

std::vector<RenderBackend> RenderManager::supported_backends() const {
    return {
        RenderBackend::Simulated,
        RenderBackend::DirectX,
        RenderBackend::Warp,
        RenderBackend::DirectX12,
    };
}

int RenderManager::total_commit_count() const {
    return total_commit_count_;
}

void RenderManager::notify_commit() {
    ++total_commit_count_;
}

ResourceManager& RenderManager::resource_manager() {
    return resource_manager_;
}

const ResourceManager& RenderManager::resource_manager() const {
    return resource_manager_;
}

DeviceRecovery& RenderManager::device_recovery() {
    return device_recovery_;
}

const DeviceRecovery& RenderManager::device_recovery() const {
    return device_recovery_;
}

DiagnosticsCenter& RenderManager::diagnostics() {
    return diagnostics_;
}

const DiagnosticsCenter& RenderManager::diagnostics() const {
    return diagnostics_;
}

void RenderManager::enqueue_command(RenderCommand command) {
    std::lock_guard<std::mutex> lock(command_queue_mutex_);
    command_queue_.push(std::move(command));
}

std::vector<RenderCommand> RenderManager::drain_commands() {
    std::vector<RenderCommand> drained;
    std::lock_guard<std::mutex> lock(command_queue_mutex_);
    while (!command_queue_.empty()) {
        drained.push_back(std::move(command_queue_.front()));
        command_queue_.pop();
    }
    return drained;
}

void RenderManager::start_render_thread() {
    std::lock_guard<std::mutex> lock(render_thread_mutex_);
    if (render_thread_running_) {
        return;
    }

    render_thread_running_ = true;
    render_thread_ = std::thread([this]() {
        while (true) {
            {
                std::lock_guard<std::mutex> guard(render_thread_mutex_);
                if (!render_thread_running_) {
                    break;
                }
            }

            auto commands = drain_commands();
            if (!commands.empty()) {
                diagnostics_.log(LogLevel::Info, "Render thread drained batched commands");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds {1});
        }
    });
}

void RenderManager::stop_render_thread() {
    {
        std::lock_guard<std::mutex> lock(render_thread_mutex_);
        if (!render_thread_running_) {
            return;
        }
        render_thread_running_ = false;
    }

    if (render_thread_.joinable()) {
        render_thread_.join();
    }
}

bool RenderManager::is_render_thread_running() const {
    std::lock_guard<std::mutex> lock(render_thread_mutex_);
    return render_thread_running_;
}

}  // namespace dcompframe
