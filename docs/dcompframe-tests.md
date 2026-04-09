# 测试文档

## 测试策略

采用 TDD，先编写测试再补实现。

## 用例清单

1. `RenderManagerTests.InitializeAndShutdown`
2. `RenderManagerTests.CommitRequiresInitBindingAndDirtyFlag`
3. `WindowHostTests.DefaultConfigIncludesNoRedirectionBitmap`
4. `WindowHostTests.ResizeAndDpiChangeTriggerRedraw`
5. `WindowHostTests.StateTransitionsAreTracked`
6. `UIElementTests.VisualTreeAddAndRemoveChild`
7. `UIElementTests.EventDispatchSupportsCaptureTargetAndBubble`
8. `GridPanelTests.ArrangeSplitsCellsAndAppliesPlacement`
9. `StackPanelTests.VerticalArrangeRespectsSpacing`
10. `StackPanelTests.HorizontalArrangeWrapsWhenEnabled`

## 覆盖点

- 渲染初始化与提交门禁
- 窗口样式、DPI 处理、重绘触发
- 视觉树结构维护
- 事件分发顺序
- Grid / Stack 布局结果

## 最近结果

- 运行命令：`ctest --preset vs2022-x64-debug-tests`
- 结果：10/10 通过
