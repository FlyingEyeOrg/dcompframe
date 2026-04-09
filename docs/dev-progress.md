# 开发进度

## 已完成

- 工程基础：完成 CMake + vcpkg + C++20 工程化配置。
- 渲染层：
  - `RenderManager` 支持 `Simulated` / `DirectX` / `Warp` 后端初始化入口，并预留 `DirectX12` 插件位。
  - `CompositionBridge` 完成绑定、提交与提交计数。
  - 新增 `ResourceManager`、`DeviceRecovery`、`DiagnosticsCenter`（含 P95/提交频率/峰值资源/导出报告）。
  - 新增命令缓冲与后台渲染线程基础能力（渲染线程与 UI 线程解耦基础设施）。
  - 修复 `WS_EX_NOREDIRECTIONBITMAP` 场景下的 DirectX 误回退：DComp 探测改为 `IDCompositionDevice`，避免 demo 透明窗口。
- 窗口层：
  - `WindowHost` 支持创建/销毁、可见性、状态切换、DPI、消息循环渲染调度。
  - 默认扩展样式保持 `WS_EX_NOREDIRECTIONBITMAP`。
- UI 核心层：
  - `UIElement` 新增透明度、裁剪、变换、边距、焦点、脏标记与递归清理。
  - 新增 `LayoutManager`，支持 `Absolute/Stack/Grid` 布局策略。
- 布局面板：`GridPanel` 与 `StackPanel` 完成布局计算和换行能力。
- 控件层：完成 `Panel`、`TextBlock`、`Image`、`Button`、`Card`。
- 扩展控件：完成 `TextBox`、`ListView`、`ScrollViewer`、`CheckBox`、`Slider`。
- 样式主题：支持 `dark/light/brand` 预设主题与主题切换。
- 数据绑定：新增 `Observable` 与 `BindingContext`，支持控件绑定更新。
- 输入系统：新增 `InputManager`，支持焦点环、双击、拖拽与快捷键路由。
- 配置系统：新增 `AppConfigLoader`，支持 JSON 配置驱动。
- 动画层：完成 `AnimationManager`，支持位置/透明度/旋转/裁剪属性动画和缓动函数。
- 工具层：完成 `WindowRenderTarget`，实现窗口与提交流程桥接，并新增 DX11 + DirectComposition 实际清屏/Present 路径。
- 测试：扩展到 33 个测试，x64/x86 Debug 下 `ctest` 全部通过。
- demo：升级 `dcompframe_demo`，覆盖窗口、控件、动画、资源、诊断与渲染目标链路。
- 调试与构建任务：补齐 `.vscode/tasks.json` 与 `.vscode/launch.json`，覆盖 x64/x86、Debug/Release 的构建/测试/运行/调试流。
- 修复 demo 生命周期：消息循环改为阻塞运行，窗口关闭后退出，不再“弹一下就退出”。
- 工程交付：新增 CI workflow、clang 配置、CPack ZIP 打包与 CHANGELOG。
- 兼容文档：新增 DWM 兼容与回退策略文档，明确能力探测与降级路径。
- 稳定性：新增设备丢失/恢复集成级压测用例（500 次循环）。
- 稳定性：新增 soak baseline、资源峰值巡检、配置缺失/损坏故障注入测试。

## 当前状态

- 阶段 1（基础渲染与窗口框架）：完成。
- 阶段 2（UI 核心与视觉树）：完成。
- 阶段 3（布局面板与控件实现）：完成。
- 阶段 4（动画与效果）：核心能力完成。
- 阶段 5（稳定性、测试与文档）：已达到首版交付标准。

## 后续待办

- 补充真实 D2D/DirectWrite 到 D3D 纹理绘制链路。
- 增加设备丢失/恢复的 8h 长稳压测与统计报表自动归档。
- 按 `docs/future-todo.md` 持续推进剩余 P3 增强事项。
