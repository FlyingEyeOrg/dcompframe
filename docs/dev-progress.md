# 开发进度

## 已完成

- 2026-04-10（渲染修复补丁）
  - 修复容器溢出显示：overlay 卡片内容区增加裁剪，子元素超出父容器时不再外溢显示。
  - 修复按钮文字未居中：按钮文本改为使用完整按钮矩形进行水平/垂直居中绘制。
  - 增强控件可视特征：为 `CheckBox`、`ComboBox`、`Slider`、`ScrollViewer` 补齐专属 glyph/轨道/箭头/滚动条视觉。
  - 回归验证：x64 Debug 测试 `37/37` 全部通过。

- 2026-04-10（补丁）
  - 修复“程序打开后直接退出”问题：当首选 DirectX 后端初始化失败时，demo 自动降级到 Simulated 后端继续运行。
  - 对齐 Panel 布局语义：新增 `Panel::arrange`，默认让容器与其子元素填满可用区域，行为贴近 WPF Panel 容器用法。
  - 新增测试 `ControlsTests.PanelArrangeStretchesChildrenToAvailableSize`。
  - 回归验证：x64 Debug 测试 `37/37` 全部通过。

- 2026-04-10（本次迭代）
  - 移除开发阶段兜底逻辑：
    - 移除 `RenderManager::initialize_with_backend(DirectX)` 的自动 WARP 降级。
    - 移除 demo 中 DirectX 失败自动回退到 Simulated 的逻辑。
    - 移除 `WindowRenderTarget` 的 DX11 几何 fallback 绘制，D2D 不可用/绘制失败改为显式失败。
    - `Theme::resolve` 去除 fallback 参数，缺失样式 key 直接抛异常。
  - 多窗口支持增强：
    - `WindowHost` 改为按窗口计数控制 `PostQuitMessage`，仅最后一个窗口销毁时退出消息循环。
    - 修复 DPI 消息读取错误（`WM_DPICHANGED` 使用 `LOWORD(wParam)`）。
    - 修复窗口创建后 client size 初始化误差（改为 `GetClientRect`）。
  - 布局系统收敛与容器行为更新：
    - 布局系统仅保留 `Grid/Stack` 策略，移除 `Absolute`。
    - `GridPanel`/`StackPanel` 默认填满父容器。
    - `StackPanel` 在交叉轴默认拉伸子项（更接近 WPF StackPanel 体验）。
  - 控件能力增强：
    - 新增 `ComboBox`。
    - 新增 `RichTextBox`，并作为“非居中”例外。
    - `StyledElement` 新增文本水平/垂直对齐属性，默认居中。
  - 文本对齐策略落地：
    - 默认控件文本居中（水平+垂直）。
    - `RichTextBox` 默认左上对齐。
    - `WindowRenderTarget` 的 `DirectWrite TextFormat` 设置为居中对齐。
  - 隐患 bug 修复：
    - 修复 `UIElement::add_child` 重复挂载与跨父节点挂载隐患。
    - `add_child/remove_child` 触发脏标记，避免布局或状态更新丢帧。
  - 测试新增与回归：
    - 新增 3 个测试（多窗口退出安全、ComboBox、文本对齐规则）。
    - 全量测试通过：`36/36`（x64 Debug）。

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
  - 新增鼠标消息触发重绘（`WM_MOUSEMOVE/WM_LBUTTONDOWN/WM_LBUTTONUP`），支持悬停与点击的实时视觉反馈。
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
  - 在 DX 路径增加 D2D/DirectWrite 叠加绘制，demo 可见卡片与控件内容，不再仅有背景色。
  - 修复 D2D 目标创建参数与工厂初始化细节，提升控件可见绘制稳定性。
  - 新增 DX11 `ClearView` 几何兜底层：当 D2D 初始化失败时仍绘制控件块，确保 no-redirection 场景下不出现“只有背景”。
  - 修复 D2D `EndDraw()` 运行时失败未触发兜底的问题：现在失败后立即切换 DX11 几何控件绘制，demo 不再只显示背景色。
  - 修复 `CreateBitmapFromDxgiSurface(E_INVALIDARG)`：composition swapchain 的 D2D target bitmap 参数改回兼容的 `TARGET | CANNOT_DRAW`，文字层恢复可绘制。
  - 背景色改为基于窗口尺寸计算，窗口大小变化时可见背景同步变化。
  - demo overlay 按钮新增 hover/press/toggle 交互与点击计数显示。
  - demo list 区域新增逐项 hover 高亮（D2D 与 DX11 fallback 路径一致）。
  - 去除非 DX 路径中的 GDI 绘制逻辑，保持渲染链路为 DX/DComp 提交模型。
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

- 增加设备丢失/恢复的 8h 长稳压测与统计报表自动归档。
- 按 `docs/future-todo.md` 持续推进剩余 P3 增强事项。
