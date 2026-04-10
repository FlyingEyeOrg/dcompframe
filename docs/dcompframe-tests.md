# 测试文档

## 测试策略

采用 TDD，先编写测试再补实现。

## 用例清单

1. `RenderManagerTests.InitializeAndShutdown`
2. `RenderManagerTests.ResourceManagerAndRecoveryWorkflows`
3. `RenderManagerTests.CommitRequiresInitBindingAndDirtyFlag`
4. `WindowHostTests.DefaultConfigIncludesNoRedirectionBitmap`
5. `WindowHostTests.ResizeAndDpiChangeTriggerRedraw`
6. `WindowHostTests.StateTransitionsAreTracked`
7. `WindowHostTests.CreateMessageLoopAndDestroy`
8. `UIElementTests.VisualTreeAddAndRemoveChild`
9. `UIElementTests.EventDispatchSupportsCaptureTargetAndBubble`
10. `UIElementTests.DirtyAndFocusFlagsPropagateCorrectly`
11. `GridPanelTests.ArrangeSplitsCellsAndAppliesPlacement`
12. `StackPanelTests.VerticalArrangeRespectsSpacing`
13. `StackPanelTests.HorizontalArrangeWrapsWhenEnabled`
14. `ControlsTests.ThemeResolvesAndButtonClickStateWorks`
15. `ControlsTests.CardStoresMetadataAndAction`
16. `AnimationTests.PropertyAnimationUpdatesElementAndCompletes`
17. `AnimationTests.PositionAnimationChangesBounds`
18. `LayoutManagerTests.StackStrategyAppliesSequentialBounds`
19. `WindowRenderTargetTests.InitializeAndPresentFrames`
20. `WindowHostTests.MessageLoopExitsWhenQuitPosted`
21. `WindowHostTests.DestroySingleWindowDoesNotQuitWhenOtherWindowAlive`
22. `ThemeTests.BuiltinPalettesWork`
23. `BindingTests.CardAndTextBoxBindingsUpdateState`
24. `ControlExtensionTests.AdditionalControlsStoreAndExposeState`
25. `InputManagerTests.FocusDoubleClickAndDragAreHandled`
26. `ConfigTests.JsonConfigCanBeLoaded`
27. `DiagnosticsTests.ExportReportAndMetricsWork`
28. `IntegrationTests.WindowRenderAnimationAndInputFlow`
29. `IntegrationTests.DeviceLossRecoveryStressLoopRemainsStable`
30. `ReliabilityTests.SoakBaselineLoopMaintainsConsistency`
31. `ReliabilityTests.ResourcePeakPatrolStaysBoundedAfterReleaseCycles`
32. `ReliabilityTests.FaultInjectionCoversDeviceLostConfigMissingAndCorruptJson`
33. `TextBoxTests.CompositionAndSelectionWorkflow`
34. `RenderManagerTests.BackendRegistryAndCommandBatchingWork`
35. `ControlsTests.ComboBoxStoresItemsAndSelectedText`
36. `ControlsTests.TextAlignmentDefaultsToCenterExceptRichTextBox`
37. `ControlsTests.PanelArrangeStretchesChildrenToAvailableSize`
38. `ControlsTests.ItemsControlStoresItemsSelectionAndVisibleRange`
39. `ControlsTests.RichTextBoxSupportsEditingSelectionAndCaretMovement`
40. `ControlsTests.ListViewAndItemsControlTrackScrollOffsets`

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

## 最近结果

- 运行命令：`ctest --preset vs2022-x64-debug-tests`
- 结果：43/43 通过
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
