# DWM 兼容说明与回退策略

## 目标

确保在不同 Windows 版本、不同 GPU 能力与不同 DWM 状态下，窗口仍具备可用渲染与可预测行为。

## 主路径

1. 窗口样式使用 `WS_EX_NOREDIRECTIONBITMAP`。
2. 渲染路径使用 `DirectX (D3D11) + DirectComposition`。
3. `WindowRenderTarget` 通过 `CreateSwapChainForComposition` 绑定到 `IDCompositionVisual`。
4. 每帧执行 `ClearRenderTargetView -> Present -> DComp Commit`。

## 兼容探测

1. 后端初始化阶段：
   - 首选 `D3D_DRIVER_TYPE_HARDWARE`。
   - 调试层不可用时，仅退回到不带调试层的硬件设备创建路径。
2. 运行阶段：
   - `Present` 返回 `DXGI_ERROR_DEVICE_REMOVED` 或 `DXGI_ERROR_DEVICE_RESET` 时，标记设备丢失。
   - 通过 `DeviceRecovery` 执行恢复并重建渲染目标视图。

## 回退策略

1. DirectX 后端初始化失败：demo 入口直接返回错误，保持 Fail-Fast。
2. DirectX 运行期故障：优先执行恢复流程；恢复失败时保留日志并禁止无效提交。
3. 无法绑定 HWND 或 DComp 目标时：回退到 `CompositionBridge` 的逻辑提交路径，避免进程异常退出。

## DWM 相关建议

1. 保持系统 DWM 开启（Windows 8.1+ 默认开启）。
2. 混合 DPI 场景下验证窗口缩放与重绘请求是否同步。
3. 针对远程桌面或硬件受限场景，关注帧率与输入响应，而不是绝对渲染性能。

## 验证清单

1. 启用 DirectX 后端，确认窗口可见且有清屏颜色变化。
2. 执行设备丢失恢复压测，确认恢复计数与提交计数匹配。
3. 关闭窗口后确认消息循环退出且资源释放无崩溃。
4. 检查 diagnostics 日志中不存在持续增长的错误日志。
