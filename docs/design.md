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
- 提供 `DirectX` 后端入口，使用 D3D11 设备并通过 DXGI 创建设备到 DComp。
- 统一通过 `CompositionBridge` 执行提交门禁和节流。

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
