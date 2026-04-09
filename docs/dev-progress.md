# 开发进度

## 已完成

- 工程基础：完成 CMake + vcpkg + C++20 工程化配置。
- 渲染层：
  - `RenderManager` 支持 `Simulated` 与 `DirectX` 后端初始化入口。
  - `CompositionBridge` 完成绑定、提交与提交计数。
  - 新增 `ResourceManager`、`DeviceRecovery`、`DiagnosticsCenter`。
- 窗口层：
  - `WindowHost` 支持创建/销毁、可见性、状态切换、DPI、消息循环渲染调度。
  - 默认扩展样式保持 `WS_EX_NOREDIRECTIONBITMAP`。
- UI 核心层：
  - `UIElement` 新增透明度、裁剪、变换、边距、焦点、脏标记与递归清理。
  - 新增 `LayoutManager`，支持 `Absolute/Stack/Grid` 布局策略。
- 布局面板：`GridPanel` 与 `StackPanel` 完成布局计算和换行能力。
- 控件层：完成 `Panel`、`TextBlock`、`Image`、`Button`、`Card` 与主题样式系统。
- 动画层：完成 `AnimationManager`，支持位置/透明度/旋转/裁剪属性动画和缓动函数。
- 工具层：完成 `WindowRenderTarget`，实现窗口与提交流程桥接。
- 测试：从 10 个扩展到 19 个测试，`ctest` 全部通过。
- demo：升级 `dcompframe_demo`，覆盖窗口、控件、动画、资源、诊断与渲染目标链路。

## 当前状态

- 阶段 1（基础渲染与窗口框架）：完成。
- 阶段 2（UI 核心与视觉树）：完成。
- 阶段 3（布局面板与控件实现）：完成。
- 阶段 4（动画与效果）：核心能力完成。
- 阶段 5（稳定性、测试与文档）：进行中，当前测试和文档已同步到最新实现。

## 后续待办

- 补充真实 D2D/DirectWrite 到 D3D 纹理绘制链路。
- 增加设备丢失/恢复的集成级压测用例。
- 增加更复杂控件（ListView/TextBox）与数据绑定能力。
- 增强 DWM 兼容说明与回退策略文档。
