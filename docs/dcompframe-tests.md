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
21. `ThemeTests.BuiltinPalettesWork`
22. `BindingTests.CardAndTextBoxBindingsUpdateState`
23. `ControlExtensionTests.AdditionalControlsStoreAndExposeState`
24. `InputManagerTests.FocusDoubleClickAndDragAreHandled`
25. `ConfigTests.JsonConfigCanBeLoaded`
26. `DiagnosticsTests.ExportReportAndMetricsWork`
27. `IntegrationTests.WindowRenderAnimationAndInputFlow`
28. `IntegrationTests.DeviceLossRecoveryStressLoopRemainsStable`

## 覆盖点

- 渲染初始化、提交门禁、资源管理、设备恢复、诊断统计
- 窗口样式、生命周期、消息循环、DPI、状态切换
- 视觉树结构、事件分发、脏标记、焦点
- Grid / Stack / LayoutManager 布局结果
- 控件状态机、样式主题、Card 组合结构
- 动画时间线、属性更新、完成回收
- 窗口渲染目标呈现链路
- 设备丢失/恢复集成压测循环与恢复稳定性

## 最近结果

- 运行命令：`ctest --preset vs2022-x64-debug-tests`
- 结果：28/28 通过
- 补充验证：`ctest --preset vs2022-x86-debug-tests`，28/28 通过
