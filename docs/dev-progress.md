# 开发进度

## 已完成

- 2026-04-10（布局系统根因修复）
  - 修正 `StackPanel` / `GridPanel` 的 `Arrange` 语义：不再重置父容器已安排的坐标，开始正确保留容器偏移并应用 `margin`。
  - 为 `UIElement` 增加绝对边界计算能力，支持渲染层读取真实布局结果。
  - 移除 `window_render_target.cpp` 中的 demo 分区公式 `compute_demo_stack_metrics(...)`，改为直接消费控件真实边界。
  - `demo/main.cpp` 中补齐各 section 控件的 `margin` 与 `desired_size`，使 `StackPanel` / `GridPanel` 的布局结果可直接用于绘制与命中测试。
  - 更新布局单测与集成测试，验证布局面板与渲染层已按同一边界语义工作。
  - 验证：x64 Debug demo 构建通过，`ctest --preset vs2022-x64-debug-tests` 结果 `49/49` 通过。

- 2026-04-10（demo 容器化布局重排）
  - demo 主体布局重构为“外层纵向 `StackPanel` section 栈 + 局部双列 `GridPanel`”。
  - 顶部展示区拆分为左右两列：左列表单编辑区、右列预览区；中部集合区拆分为 `ListView` / `ItemsControl` 双列。
  - `ScrollViewer` 与 `LogBox` 改为独立 section，不再与上方内容自由拼贴，降低版面混乱与重叠风险。
  - `demo/main.cpp`、`src/tools/window_render_target.cpp` 与 `tests/integration_flow_tests.cpp` 已按同一 section 模型对齐。
  - 验证：x64 Debug demo 构建通过，`ctest --preset vs2022-x64-debug-tests` 结果 `49/49` 通过。

- 2026-04-10（滚动条状态恢复与 demo 版面重排）
  - 修复带滚动视图的 `ScrollBar` thumb 在鼠标松开后仍保持聚焦色的问题：`WM_LBUTTONUP`、`WM_CAPTURECHANGED`、`WM_CANCELMODE` 路径统一清理 `focused_scroll_target_` 并触发重绘。
  - 重构 demo overlay 布局比例：压缩底部 `ScrollViewer` / `LogBox` 区高度，把更多可用空间分配给上方表单区与右侧 preview 区。
  - 重构 preview 卡片内部布局：缩小非关键头部和图片区，限制 `Expander body` 的最大高度，避免 `TabControl`、进度条、Loading、Popup` 挤在一起。
  - 依据 Microsoft Learn 官方文档复核鼠标捕获和窗口布局：确认释放捕获后需要反映新的捕获态；确认当前 demo 旧布局属于过度拥挤、未有效利用可用空间的问题。
  - 验证：x64 Debug demo 构建通过，`ctest --preset vs2022-x64-debug-tests` 结果 `49/49` 通过。

- 2026-04-10（骨架层回归与 demo 职责纠偏）
  - `Application` 收敛为应用生命周期骨架：配置加载、渲染线程、窗口集合、消息循环、连续渲染节流、析构清理。
  - `Window` 收敛为窗口宿主骨架：窗口创建、渲染目标初始化、根内容/布局回调/交互控件接线扩展点。
  - demo 控件创建、事件绑定、根布局与窗口标题全部回迁到 `demo/main.cpp` 中的 `DemoWindow`。
  - 明确 `GridPanel` / `StackPanel` 为透明布局容器；`Panel` 默认透明背景、透明边框、0 边框厚度。
  - `WindowRenderTarget` 的 demo 分区算法改为“总高度内分配 + 空间不足时压缩非关键区块”，修复窗口收缩时的区域重叠。
  - 新增框架设计文档 `docs/framework-design.md`，说明层次、原理与 demo 接入方式。
  - 新增测试：`WindowTests.SkeletonWindowInitializesWithoutDemoControls`、`ApplicationTests.InitializeDoesNotCreateWindowUntilRequested`、`ControlsTests.PanelDefaultsToTransparentBackgroundAndBorder`。
  - 验证：x64 Debug demo 构建通过，`ctest --preset vs2022-x64-debug-tests` 结果 `49/49` 通过。

- 2026-04-10（按用户 12 项清单收口：布局/性能/控件/迁移）
  - 布局容器：`Panel` 默认样式改为透明背景 + 无边框。
  - 性能：`WindowRenderTarget::needs_continuous_rendering()` 仅在 `Loading active` 或 `Progress indeterminate` 时返回 true，移除仅 `TabControl` 即持续重绘的路径；渲染中移除 `overlay_scene_.items` 每帧拷贝。
  - Tab/预览区错乱：重写右侧 preview 纵向分配，`TabControl`、`tab body`、动画条、`Expander`、`Progress +/-`、`Loading`、`Popup` 分区不再重叠。
  - Progress 控件：确认并保留 `+/-` 按钮交互，点击后同步更新 `Progress` 与 `Loading`，并写入日志。
  - Loading 控件：在 demo 中保持接线与状态联动展示。
  - 滚动条聚焦态：`ScrollViewer`、`ListView`、`ItemsControl`、`LogBox` 轨道/滑块点击后高亮；`ComboBox` dropdown 新增轨道与滑块命中及聚焦态。
  - Expander 占位：收起时 body 高度为 0，不占正文布局；展开时按剩余空间占位。
  - 日志框：`LogBox` 已纳入 demo 交互接线与渲染（含滚动条）。
  - Slider 视觉：轨道改为更接近 Element Plus 的横向布局，值显示改为右侧 badge，去除拥挤的左右端标签。
  - 标题区分层：标题带与内容区采用不同背景色，提高可读性。
  - 架构迁移：将 demo 内 `Application`/`Window` 迁移到主项目：新增 `include/dcompframe/application.h`、`src/application.cpp`，`demo/main.cpp` 仅保留入口调用。
  - 构建测试：x64 Debug 构建通过，`ctest --preset vs2022-x64-debug-tests` 全部通过（`46/46`）。

- 2026-04-10（demo 布局与交互收口：纵向分区、Tab、滚动条、动画示例）
  - 将 demo 主内容区重排为纵向堆叠式分区：标题区与内容滚动区解耦，滚动时不再覆盖标题。
  - 右侧预览卡片压缩内部布局，确保 `TabControl`、动画条、`Expander`、`Progress`、`Loading` 在常见窗口尺寸下可稳定展示。
  - `WindowRenderTarget` 新增 `TabControl` 点击切换命中与滚动条轨道点击跳转逻辑，覆盖 `ScrollViewer`、`ListView`、`ItemsControl` 三类带滚动条控件。
  - 为 demo 增补明确的动画示例，并扩展 `ListView` / `ItemsControl` / `ScrollViewer` 数据量，保证滚动与内容可见性可复现。
  - 新增集成测试 `IntegrationTests.WindowRenderTargetProcessesTabExpanderAndScrollbarTrackClicks`，覆盖页签切换和三类滚动条轨道点击。
  - 验证：`cmake --build --preset vs2022-x64-debug --target dcompframe_tests` 与 `ctest --preset vs2022-x64-debug-tests --output-on-failure` 结果 `46/46` 全通过。

- 2026-04-10（UI 体验修复：输入、下拉、布局与视觉一致性）
  - 修复 `TextBox` 光标高度异常与占位符文本裁剪：统一输入字体尺寸并调整文本内边距与 caret 高度。
  - 修复 `ComboBox` 展开内容异常：下拉展开时重置内部滚动偏移，避免历史偏移造成“展开即错位”。
  - 修复右侧内容错乱：重分配预览卡片、Tab/Expander/Progress/Loading/Popup 区域，避免互相覆盖。
  - 新增页面级滚动偏移处理：在非内层滚动命中时，窗口主区域可滚动，支持继续向下扩展布局。
  - 背景改为纯白底，符合 Element Plus 页面基调。
  - 字体对齐 Element Plus：切换 `Microsoft YaHei UI`，并统一正文/输入字号到 14 体系。
  - 验证：`ctest --preset vs2022-x64-debug-tests` 结果 `45/45` 全通过。

- 2026-04-10（按 UI 规范补齐控件并全量接入 demo）
  - 依据 `docs/ui-requirements/ui-framework-design-spec.md` 完成控件族补齐：`Label`、`Progress`、`Loading`、`TabControl`、`Popup`、`Expander`。
  - `controls.h` 聚合头导出新增控件，`WindowRenderTarget::InteractiveControls` 完成新增控件接线。
  - Demo 完成新增控件实例化与展示，保持 `Direct3D11 + DirectComposition + WS_EX_NOREDIRECTIONBITMAP` 技术栈不变。
  - Element Plus 风格延续：新增控件采用浅色面板、蓝色主色与弱对比边框体系。
  - 输入与滚轮链路继续收口：ComboBox 下拉打开时滚轮优先消费于下拉层，避免外层 ScrollViewer 穿透滚动。
  - 新增测试：`ControlsTests.AdditionalControlsSupportCoreStateTransitions`、`IntegrationTests.WindowRenderTargetProcessesTextInputAndConsumesComboWheel`。
  - 验证结果：`cmake --build --preset vs2022-x64-debug` 与 `ctest --preset vs2022-x64-debug-tests` 通过，`45/45`。

- 2026-04-10（UI 需求基线文档建立）
  - 新增独立目录 `docs/ui-requirements/`，用于承载后续 UI 开发强制规范。
  - 新增框架总规范 `ui-framework-design-spec.md`，明确设计概要、技术要求、验收标准与质量门禁。
  - 新增控件规范 `ui-controls-design-spec.md`，覆盖 TextBox、ComboBox、Progress、Loading、TabControl、Grid、StackPanel、Panel、Label、Popup、Expander、ItemsControl 的设计概要、设计要求、功能标准、验收标准。
  - 文档内容对齐 WinUI、WPF、WAI-ARIA APG、Material 3 公开标准，并建立可追溯标准来源。
  - README 与设计决策文档已同步引用新规范，后续 UI 开发将按该基线执行。

- 2026-04-10（第四轮输入与滚动修复）
  - Demo 根容器切换为 `GridPanel`，运行时按窗口客户区尺寸重新 `arrange()`，内容区与外层卡片一起铺满窗口。
  - 重新收口 overlay 自适应布局：右列列表区与底部 `ScrollViewer` 分区不再互相遮挡，`ItemsControl` 不再被底部滚动区覆盖。
  - `TextBox` 交互链路复验并保留可编辑行为，继续支持鼠标定位、选区、退格/删除、复制粘贴与双向绑定。
  - `RichTextBox` 新增内部 `scroll_offset`、鼠标滚轮滚动、拖拽选区裁剪、caret 可见性自动跟随，以及 `Up/Down/PageUp/PageDown` 多行导航。
  - `ScrollViewer` 取消焦点参与，不再因鼠标离开仍依赖焦点继续滚动；新增可拖拽滚动条 thumb。
  - `ListView` 与 `ItemsControl` 补齐拖拽滚动条 thumb，滚轮和拖拽两条链路统一走内部偏移。
  - 回归测试新增 `RichTextBox` 滚动偏移和 `ScrollViewer` 非 focusable 校验。
  - 验证：`cmake --build --preset vs2022-x64-debug --target dcompframe_tests`、`ctest --preset vs2022-x64-debug-tests --output-on-failure`、`cmake --build --preset vs2022-x64-debug --target dcompframe_demo` 全部通过。

- 2026-04-10（第三轮 UI 收口与交互修复）
  - 视觉继续向 Element Plus 收敛：`Card`、`ListView`、`ItemsControl` 的边框、分区、留白与浅色层次进一步统一，保持 `GridPanel` / `StackPanel` / `Panel` 的 WPF 容器语义不变。
  - Demo 自适应优化：`WindowRenderTarget::compute_layout()` 改为按窗口宽高动态分配卡片、表单区、列表区和滚动区高度，内容可随窗口尺寸拉伸铺满。
  - 修复 `ComboBox` 弹层被后续控件遮挡：dropdown 在主内容裁剪结束后以顶层 overlay 顺序重新绘制，不再被兄弟控件覆盖。
  - 修复 `ScrollViewer` 可视高度过低：底部滚动区重新分配剩余空间，Demo 中可稳定显示更长内容。
  - 完成 `RichTextBox` 可编辑化：新增光标、选区、插入、删除、快捷键与回车换行编辑模型，并与 `TextBox` 保持一致的退格语义。
  - `ListView` 默认支持内容溢出滚动：新增内部 `scroll_offset`，鼠标滚轮与点击选中均按视口偏移工作。
  - `ItemsControl` 默认切换为类 `StackPanel` 垂直排布，并在内容溢出时启用内部滚动与轻量滚动条。
  - 字体统一：DirectWrite 文本格式默认字体族切换为 `Microsoft YaHei`，提升中文界面一致性。
  - 新增测试 `ControlsTests.RichTextBoxSupportsEditingSelectionAndCaretMovement` 与 `ControlsTests.ListViewAndItemsControlTrackScrollOffsets`。
  - 验证：`ctest --preset vs2022-x64-debug-tests --output-on-failure` 全量通过，结果 `43/43`。

- 2026-04-10（Element Plus 交互层第二轮收口）
  - `WindowRenderTarget` 重新整理为左右分栏表单布局：保留 `GridPanel` / `StackPanel` / `Panel` 的 WPF 容器语义，交互控件展示统一收敛到 Element Plus 风格。
  - 修复 `CheckBox` 右侧文案异常换行：单行标签改用显式 `DWRITE_WORD_WRAPPING_NO_WRAP` 与更宽文本区域，不再被右侧窄列挤压。
  - 修复 `ComboBox` 展开会顶开后续元素：下拉列表改为 overlay 浮层，不再参与主布局流。
  - 扩展 `ScrollViewer`：接入 `ItemsControl` 内容模型，Demo 中加入足够多滚动项，并基于可见范围只绘制 viewport 内元素。
  - 新增 `ItemsControl` 控件与测试，Demo 中补齐 `RichTextBox`、`TextBlock`、`Image`、`Card`、`ListView`、`ItemsControl`、`ScrollViewer` 的可视示例。
  - 修复 Demo 中 TextBox 输入链路：`WindowHost` / `WindowRenderTarget` 新增 `WM_GETDLGCODE`、`WM_MOUSEWHEEL` 路由，TextBox 输入与 ScrollViewer 滚轮/键盘滚动恢复正常。
  - 架构优化：把滚动内容虚拟范围计算前移到 `ItemsControl::visible_range()`，减少 ScrollViewer 全量绘制开销。
  - 验证：`cmake --build --preset vs2022-x64-debug --target dcompframe_tests` 与 `ctest --preset vs2022-x64-debug-tests` 均通过，结果 `41/41`。

- 2026-04-10（UIElement3 风格升级与交互优化）
  - 优化按钮居中：在 draw_centered_text 内部显式临时设置 DWRITE_TEXT_ALIGNMENT_CENTER 解决水平与垂直方向未能完全绝对居中的问题。
  - CheckBox UI 升级：改绘为 24x24 矩形块，应用 WinUI 3 风格的蓝底白钩  标志；同时更新文本起始偏移量（label_offset），避免文本与 CheckBox 钩子重合并提供更好的布局展示。
  - ComboBox 体验优化：为下拉项增加额外的上下 margin 与内边距 padding，彻底改善原布局带来的拥挤挤推感；在 Demo 中新增“Settings”、“Account”等选项以展示更丰富内容。
  - Slider 居中与展示：缩短并居中了滑动轨道的两端边距使其规避在最右端；为 Slider 数值提供右对齐锚点 DWRITE_TEXT_ALIGNMENT_TRAILING，解决了滑块与部分文本展示不居中重合的问题。
  - ScrollViewer：将其补充为 InteractiveControls 可视控件中的一员并补充到底部渲染区域进行绘制展示。

- 2026-04-10（真交互与多窗口落地）
  - 实现真实多窗口创建与调度：Demo 主程序升级为应用级控制器，支持通过点击按钮动态创建新窗口。
  - TextBox 真实交互：支持字符输入（WM_CHAR）、选区高亮、光标导航（Left/Right/Home/End 等）、快捷键复制粘贴剪切（Ctrl+C/V/X/A）及 Backspace/Delete 功能。
  - CheckBox 真实交互：支持点击状态切换及相应的视觉反馈。
  - ComboBox 真实交互：支持点击展开/收起下拉列表，支持鼠标点选改变当前数据项，完善相关渲染状态。
  - Slider 真实交互：支持鼠标拖拽即时计算归一化数值并反馈，同时支持键盘左右箭头步进调整互动数据。
  - 核心机制强化：完善 WindowHost 消息派发体系与生命周期收尾，在交互层构建完善的 Hit-Testing 模型以支撑全界面操作。

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
- 阶段 6（Element Plus 交互收口与 demo 完整示例）：已完成当前轮需求收口。

## 后续待办

- 增加设备丢失/恢复的 8h 长稳压测与统计报表自动归档。
- 按 `docs/future-todo.md` 持续推进剩余 P3 增强事项。


- 2026-04-10（测试收口与构建稳定化）
  - 测试收口：修正 `RenderManagerTests.BackendRegistryAndCommandBatchingWork` 的旧后端数量断言，并同步 `RichTextBox` 编辑语义测试预期。
  - 构建稳定：确认 x64 Debug 下使用 `/Z7` 规避 MSVC 共享 PDB 写入冲突，`cmake --build --preset vs2022-x64-debug --target dcompframe_tests` 可稳定完成。
  - 回归结果：`ctest --preset vs2022-x64-debug-tests` 现为 `43/43` 全部通过。


- 2026-04-10（编译修复与控件头拆分收口）
  - 修复拆分后编译失败：恢复 `src/controls/controls.cpp` 为唯一控件实现入口，CMake 不再引用不完整的拆分 `.cpp` 文件。
  - 保留控件头拆分成果：`controls.h` 退化为聚合头，`Button/TextBox/ComboBox/...` 各自独立头文件继续对外暴露。
  - 样式收敛：保持 `GridPanel` / `StackPanel` / `Panel` 的 WPF 布局语义不变，其余表单控件统一切换为 Element Plus 风格的浅色面板、蓝色主按钮、细边框与 focus ring。
  - 删除回退机制：移除 `RenderManager` 探测阶段和 `WindowRenderTarget` 初始化中的 WARP 回退，demo 启动保留显式失败。
  - 代码一致性：同步修正文档中关于 `Warp`、隐式降级和兜底策略的历史描述，并更新后端数量测试。


* 完成了 `controls.h` 聚合头与 13 个独立控件头文件的收口，继续由 `controls.cpp` 作为唯一实现入口，降低了头文件耦合度。
* 更新 `window_render_target.cpp` 的渲染逻辑，移除所有硬编码的圆角弧度和配色，应用了符合 `Element Plus` (Web) 风格的扁平化圆角（`4.0F`）和标准品牌蓝配色体系（Normal / Hover / Active）。
* 修复了 `Button` 文本绘制时未重置前景色画刷导致文本不显示的 BUG。
* 彻底废除了 WARP（软件加速）后端的代码和回退枚举，确保了严格的硬件加速策略流；并移除了 `main.cpp` 初始化时的隐式 `Simulated` 回退。
* 删除了误提交且内容截断的 `src/controls/card.cpp` 残留文件，避免后续拆分继续污染工程目录。
