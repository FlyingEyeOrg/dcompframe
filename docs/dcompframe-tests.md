# 测试文档

## 测试策略

采用 TDD，先编写测试再补实现。

## 用例清单

1. `RenderManagerTests.InitializeAndShutdown`
2. `RenderManagerTests.ResourceManagerAndRecoveryWorkflows`
3. `RenderManagerTests.CommitRequiresInitBindingAndDirtyFlag`
4. `RenderManagerTests.BackendRegistryAndCommandBatchingWork`
5. `WindowHostTests.DefaultConfigIncludesNoRedirectionBitmap`
6. `WindowHostTests.ResizeAndDpiChangeTriggerRedraw`
7. `WindowHostTests.StateTransitionsAreTracked`
8. `WindowHostTests.CreateMessageLoopAndDestroy`
9. `WindowHostTests.MessageLoopExitsWhenQuitPosted`
10. `WindowHostTests.DestroySingleWindowDoesNotQuitWhenOtherWindowAlive`
11. `WindowHostTests.DestroyNotificationClearsInternalState`
12. `WindowTests.SkeletonWindowInitializesWithoutDemoControls`
13. `ApplicationTests.InitializeDoesNotCreateWindowUntilRequested`
14. `UIElementTests.VisualTreeAddAndRemoveChild`
15. `UIElementTests.EventDispatchSupportsCaptureTargetAndBubble`
16. `UIElementTests.HitTestFindsDeepestVisibleDescendant`
17. `UIElementTests.DirtyAndFocusFlagsPropagateCorrectly`
18. `GridPanelTests.ArrangeSplitsCellsAndAppliesPlacement`
19. `GridPanelTests.ArrangePreservesParentOffsetAndAppliesMargins`
20. `GridPanelTests.MeasureUsesLargestContentContributionPerTrack`
21. `StackPanelTests.VerticalArrangeRespectsSpacing`
22. `StackPanelTests.HorizontalArrangeWrapsWhenEnabled`
23. `StackPanelTests.ArrangePreservesParentOffsetAndRespectsMargin`
24. `StackPanelTests.FlexGrowConsumesRemainingVerticalSpace`
25. `LayoutPanelsTests.NestedPanelsArrangeChildrenRecursively`
26. `FlexPanelTests.RowLayoutDistributesGrowBasisAndGap`
27. `FlexPanelTests.WrapMovesOverflowItemsToNextLine`
28. `InputManagerTests.HitTestRoutingDispatchesCaptureTargetAndBubble`
29. `InputManagerTests.FocusDoubleClickAndDragAreHandled`
30. `ControlsTests.ThemeResolvesAndButtonClickStateWorks`
31. `ControlsTests.CardStoresMetadataAndAction`
32. `ControlsTests.ComboBoxStoresItemsAndSelectedText`
33. `ControlsTests.ItemsControlStoresItemsSelectionAndVisibleRange`
34. `ControlsTests.TextBoxSupportsEditingSelectionAndTwoWayBinding`
35. `ControlsTests.RichTextBoxSupportsEditingSelectionAndCaretMovement`
36. `ControlsTests.ListViewAndItemsControlTrackScrollOffsets`
37. `ControlsTests.CheckBoxComboBoxAndSliderSupportInteractiveStateChanges`
38. `ControlsTests.TextAlignmentDefaultsToCenterExceptRichTextBox`
39. `ControlsTests.AdditionalControlsSupportCoreStateTransitions`
40. `ControlsTests.PanelArrangeStretchesChildrenToAvailableSize`
41. `ControlsTests.PanelDefaultsToTransparentBackgroundAndBorder`
42. `AnimationTests.PropertyAnimationUpdatesElementAndCompletes`
43. `AnimationTests.PositionAnimationChangesBounds`
44. `LayoutManagerTests.StackStrategyAppliesSequentialBounds`
45. `WindowRenderTargetTests.InitializeAndPresentFrames`
46. `ThemeTests.BuiltinPalettesWork`
47. `BindingTests.CardAndTextBoxBindingsUpdateState`
48. `ControlExtensionTests.AdditionalControlsStoreAndExposeState`
49. `TextBoxTests.CompositionAndSelectionWorkflow`
50. `ConfigTests.JsonConfigCanBeLoaded`
51. `DiagnosticsTests.ExportReportAndMetricsWork`
52. `IntegrationTests.WindowRenderAnimationAndInputFlow`
53. `IntegrationTests.DeviceLossRecoveryStressLoopRemainsStable`
54. `IntegrationTests.WindowRenderTargetProcessesTextInputAndConsumesComboWheel`
55. `IntegrationTests.WindowRenderTargetProcessesTabExpanderAndScrollbarTrackClicks`
56. `ReliabilityTests.SoakBaselineLoopMaintainsConsistency`
57. `ReliabilityTests.ResourcePeakPatrolStaysBoundedAfterReleaseCycles`
58. `ReliabilityTests.FaultInjectionCoversDeviceLostConfigMissingAndCorruptJson`

## 覆盖点

- 渲染初始化、提交门禁、资源管理、设备恢复、诊断统计
- 窗口样式、生命周期、消息循环、DPI、状态切换
- 视觉树结构、事件分发、脏标记、焦点
- Grid / Stack / LayoutManager 布局结果
- 控件状态机、样式主题、Card 组合结构
- 动画时间线、属性更新、完成回收
- 窗口渲染目标呈现链路
- 设备丢失/恢复集成压测循环与恢复稳定性
- 交互高级能力：文本选择/输入法组合、列表虚拟范围、惯性滚动、快捷键命令路由
- 可靠性：soak baseline、资源峰值巡检、配置缺失与损坏故障注入
- 架构能力：可插拔后端清单、命令缓冲与批量 drain
- Element Plus 交互回归：TextBox 输入、ComboBox overlay、ItemsControl 可见范围、ScrollViewer 内容模型与滚轮链路
- 第三轮交互回归：RichTextBox 可编辑模型、ListView/ItemsControl 内部滚动、ComboBox 顶层弹层遮挡修复、微软雅黑字体切换后的文本布局稳定性
- 第四轮交互回归：RichTextBox 内部滚动与上下行导航、ScrollViewer hover 限定滚轮、三类滚动条 thumb 拖拽、GridPanel 根容器下的 demo 自适应布局
- 第五轮 demo 交互回归：标题区与内容滚动区分离、TabControl 点击切换、滚动条轨道点击跳转、右侧动画示例可见性
- Flexbox Only 回归：`FlexPanel` 主轴/换行分配、元素树最深命中、capture/target/bubble pointer route

## 最近结果

- 运行命令：`ctest --preset vs2022-x64-debug-tests`
- 结果：58/58 通过
- 补充验证：新增多窗口退出安全、ComboBox、文本对齐规则用例全部通过
- 运行时复验：x64 Debug demo 启动后 `warning_count=0`，确认 D2D 运行时失败兜底后不再出现“只有背景色”。
- 运行时复验：鼠标移动可触发列表逐项 hover 高亮，按钮按下/释放后状态可切换并保持可见反馈。
- 本次回归重点：验证 `controls.h` 聚合头 + 独立控件头组合下的编译稳定性，以及 Element Plus 风格控件渲染不影响既有测试语义。
- 后端回归：`RenderManagerTests.BackendRegistryAndCommandBatchingWork` 已按当前实现收口为 3 个显式后端（`Simulated` / `DirectX` / `DirectX12` 预留）。
- 文本编辑回归：`RichTextBox` 的 `move_caret_left() + backspace()` 语义已与 `TextBox` 保持一致，当前测试按“删除光标左侧字符”校验。
- 本次交互回归重点：验证 `ItemsControl`、`ScrollViewer` 内容模型、TextBox 标准 Win32 输入链路、ComboBox overlay 布局不回流。
- 本次第三轮回归重点：验证 `RichTextBox` 编辑能力、`ListView/ItemsControl` 默认滚动、响应式布局下滚动区高度分配和弹层顶层绘制顺序。
- 本次第四轮回归重点：验证 `ScrollViewer` 取消焦点后不会离开区域继续滚动、`RichTextBox` 选区被裁剪在视口内，以及 `ListView/ItemsControl` 拖拽滚动条语义不破坏既有滚轮行为。

## 本轮补充（2026-04-10）

- 新增用例：
	- `ControlsTests.AdditionalControlsSupportCoreStateTransitions`
	- `IntegrationTests.WindowRenderTargetProcessesTextInputAndConsumesComboWheel`
- 覆盖目标：
	- `Label/Progress/Loading/TabControl/Popup/Expander` 状态机与核心属性。
	- ComboBox 下拉层滚轮消费优先级，防止穿透到外层 `ScrollViewer`。
	- TextBox 输入链路（`WM_CHAR`）端到端回归。
- 最新结果：`ctest --preset vs2022-x64-debug-tests`，`45/45` 通过。

## 本轮补充二（2026-04-10）

- 回归目标：
	- TextBox 光标高度与占位符裁剪。
	- ComboBox 展开内容错位。
	- 右侧预览区控件覆盖错乱。
	- 页面级滚动与内层滚动事件边界。
	- Element Plus 字体/字号与白底视觉一致性。
- 结果：现有 45 条测试继续全通过，未引入新增回归。

## 本轮补充三（2026-04-10）

- 新增用例：
	- `IntegrationTests.WindowRenderTargetProcessesTabExpanderAndScrollbarTrackClicks`
- 覆盖目标：
	- `TabControl` 鼠标点击切换。
	- `ScrollViewer` / `ListView` / `ItemsControl` 滚动条轨道点击跳转。
	- demo 纵向分区布局调整后，右侧预览区与底部滚动区交互不互相遮挡。
- 最新结果：`ctest --preset vs2022-x64-debug-tests --output-on-failure`，`46/46` 通过。

## 本轮补充四（2026-04-10）

- 回归目标：
	- Application/Window 从 demo 迁移到主项目后的构建稳定性。
	- 滚动条轨道/滑块聚焦态与 LogBox 滚动链路。
	- Preview 区重排后 Tab/Expander/Progress/Loading 交互不回归。
- 结果：`ctest --preset vs2022-x64-debug-tests`，`46/46` 通过。

## 本轮补充五（2026-04-10）

- 新增用例：
	- `WindowTests.SkeletonWindowInitializesWithoutDemoControls`
	- `ApplicationTests.InitializeDoesNotCreateWindowUntilRequested`
	- `ControlsTests.PanelDefaultsToTransparentBackgroundAndBorder`
- 覆盖目标：
	- 验证 `Application` / `Window` 已回到骨架层职责。
	- 验证 `Panel` 默认透明无边框。
	- 验证 demo 业务已回迁后，框架宿主仍可独立初始化与渲染。
- 结果：`ctest --preset vs2022-x64-debug-tests`，`49/49` 通过。

## 本轮补充六（2026-04-10）

- 回归目标：
	- 滚动条 thumb 在释放后恢复普通态，不残留聚焦色。
	- demo 版面重排后，preview、列表区、滚动区在常用窗口尺寸下不再显得拥挤。
	- 既有页签、进度条、滚动条轨道点击交互不回归。
- 结果：`ctest --preset vs2022-x64-debug-tests`，`49/49` 通过。

## 本轮补充七（2026-04-10）

- 回归目标：
	- demo 主体改为 `StackPanel` section 栈后，控件分组依然可正确交互。
	- 双列 `GridPanel` 场景下的 `ComboBox`、`TabControl`、滚动条轨道点击命中点保持有效。
	- 渲染层和测试中的几何推导与新版 section 布局一致。
- 结果：`ctest --preset vs2022-x64-debug-tests`，`49/49` 通过。

## 本轮补充八（2026-04-10）

- 回归目标：
	- `StackPanel` / `GridPanel` 在二次 `Arrange` 后仍保留父容器偏移。
	- `margin` 对布局结果产生实际影响。
	- `WindowRenderTarget` 在使用真实控件边界后，页签、滚动条和输入命中仍然正确。
- 结果：`ctest --preset vs2022-x64-debug-tests`，`49/49` 通过。

## 本轮补充九（2026-04-10）

- 回归目标：
	- 嵌套 `StackPanel` / `GridPanel` 在一次 root arrange 后即可完成递归布局。
	- demo 不再需要逐层对子 panel 手工 arrange 才能获得正确边界。
- 结果：`ctest --preset vs2022-x64-debug-tests`，`49/49` 通过。

## 本轮补充十（2026-04-10）

- 回归目标：
	- `UIElement` 支持自测量尺寸，`StackPanel` 支持 flex grow，`GridPanel` 支持按内容计算轨道。
	- `Window` 默认通过 root `measure + arrange` 驱动布局，不再依赖 demo 专用布局公式。
	- `WindowRenderTarget` 的页签、滚动条轨道点击与新版边界模型保持一致。
- 新增/更新用例：
	- `StackPanelTests.FlexGrowConsumesRemainingVerticalSpace`
	- `GridPanelTests.MeasureUsesLargestContentContributionPerTrack`
	- 既有布局测试切换为 `measure + arrange` 调用顺序
	- 集成测试滚动条轨道点击坐标同步到新版滚动条几何
- 结果：`ctest --preset vs2022-x64-debug-tests --output-on-failure`，`54/54` 通过。

## 本轮补充十一（2026-04-10）

- 回归目标：
	- `FlexPanel` 的 grow / basis / gap 分配是否稳定。
	- wrap 场景下第二行落位是否正确。
	- `UIElement::hit_test()` 是否优先返回最深可命中子节点。
	- `InputManager` 是否按 capture -> target -> bubble 执行 pointer route，并在 handled 时阻断默认 click handler。
- 新增用例：
	- `UIElementTests.HitTestFindsDeepestVisibleDescendant`
	- `FlexPanelTests.RowLayoutDistributesGrowBasisAndGap`
	- `FlexPanelTests.WrapMovesOverflowItemsToNextLine`
	- `InputManagerTests.HitTestRoutingDispatchesCaptureTargetAndBubble`
- 结果：`ctest --preset vs2022-x64-debug-tests`，`58/58` 通过。
