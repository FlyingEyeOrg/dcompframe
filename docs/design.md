# 设计决策

## 1. 分层架构

采用 7 层架构并保持低耦合：

- 渲染层：`RenderManager`、`CompositionBridge`
- 窗口层：`WindowHost`
- UI 核心层：`UIElement`、`LayoutManager`
- 布局层：`GridPanel`、`StackPanel`
- 控件层：`Panel`、`TextBlock`、`Image`、`Button`、`Card`、`Theme`
- 动画层：`AnimationManager`
- 工具辅助层：`ResourceManager`、`DeviceRecovery`、`DiagnosticsCenter`、`WindowRenderTarget`

## 2. 渲染后端策略

- 保留 `Simulated` 后端用于测试与快速验证。
- 提供可插拔后端枚举：`Simulated` / `DirectX` / `Warp` / `DirectX12(预留)`。
- 当前渲染实现覆盖 DX11（硬件与 WARP），DX12 保留插件位并给出明确未实现状态。
- 统一通过 `CompositionBridge` 执行提交门禁和节流。

## 2.2 渲染线程与命令缓冲

- `RenderManager` 提供线程安全命令队列（enqueue/drain）。
- 提供后台渲染线程生命周期（start/stop）以支持 UI 与渲染解耦。
- 当前阶段以基础设施和测试验证为主，后续接入真实异步提交与批处理调度策略。

## 2.1 窗口消息循环策略

- `WindowHost` 采用阻塞式消息循环（`GetMessage`）作为默认运行模式。
- 在 `WM_CLOSE/WM_DESTROY` 上触发退出消息，保证应用可预测退出。
- 测试场景可使用有限迭代循环（`max_iterations`）确保用例可控。

## 3. 事件与脏标记策略

- 事件路由固定为 Capture -> Target -> Bubble。
- 属性变更统一触发 `mark_dirty()`，向父节点递归传播。
- 通过 `clear_dirty_recursive()` 在渲染提交后清空脏状态。

## 4. 控件与样式策略

- 控件统一继承 `StyledElement`，共享状态机和样式入口。
- `Theme` 提供 key-style 映射，支持默认值回退。
- `Card` 通过组合子控件（主按钮）实现复杂组件复用。

## 5. 动画策略

- 使用时间片 `tick` 驱动，避免每帧重建视觉树。
- 提供 `Linear/EaseIn/EaseOut/EaseInOut` 缓动。
- 动画按属性维度独立，支持并发且可在 target 失效时自动回收。

## 6. 工程与依赖

- 构建：CMake（Preset + CTest）
- 包管理：vcpkg manifest
- 测试框架：GoogleTest
- 调试与任务：VS Code `tasks.json` + `launch.json`（x64/x86、Debug/Release）

## 7. DX11 + DComp 呈现策略

- `WindowRenderTarget` 在 DirectX 后端下创建并维护：
	- `ID3D11Device/ID3D11DeviceContext`
	- `IDXGISwapChain1`（`CreateSwapChainForComposition`）
	- `IDCompositionDevice/Target/Visual`
- 每帧执行 `ClearRenderTargetView -> Present -> DComp Commit`，确保 demo 有实际可见绘制内容。
- `Present` 返回 `DXGI_ERROR_DEVICE_REMOVED/RESET` 时进入设备丢失处理路径。

## 8. DWM 兼容与回退策略

- 主路径：`WS_EX_NOREDIRECTIONBITMAP + DirectComposition + DX11`。
- 初始化回退：D3D11 设备创建按 `Hardware -> WARP` 顺序降级。
- 运行时回退：demo 在 DirectX 后端初始化失败时自动降级到 `Simulated`，避免直接退出。

## 9. 交互系统策略

- `TextBox` 增加文本选择与输入法组合提交模型（selection + composition）。
- `ListView` 增加分组模型和虚拟窗口范围计算（virtual range）。
- `ScrollViewer` 增加惯性滚动速度模型与阻尼衰减。
- 输入命令系统基于快捷键路由（如 Ctrl+S）实现轻量 command dispatch。
