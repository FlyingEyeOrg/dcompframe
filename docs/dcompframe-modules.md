# 模块文档

## RenderManager / CompositionBridge

职责：管理渲染初始化状态与提交计数。

- `RenderManager::initialize(bool)`：模拟设备初始化结果。
- `RenderManager::shutdown()`：清理状态。
- `CompositionBridge::bind_target_handle(HWND)`：绑定提交目标。
- `CompositionBridge::commit_changes(bool)`：执行提交门禁检查并记录提交计数。

## WindowHost

职责：管理窗口逻辑状态与重绘请求。

- 默认扩展样式包含 `WS_EX_NOREDIRECTIONBITMAP`。
- `on_size_changed`：更新客户区并触发重绘。
- `apply_dpi`：基于 96 DPI 基线更新缩放并触发重绘。
- `consume_redraw_request`：消费一次重绘信号。
- `set_window_state`：维护窗口状态。

## UIElement

职责：维护视觉树与事件分发。

- 子节点增删、父子关系维护。
- 事件分发支持捕获、目标、冒泡阶段。
- 支持元素边界与期望尺寸属性。

## GridPanel

职责：基于行列将子元素映射到矩形网格。

- 支持行列设置。
- 支持子元素单元格与跨度设置。
- 计算并写回每个子元素边界。

## StackPanel

职责：按方向堆叠子元素并支持间距与换行。

- `Vertical`：按 Y 轴顺序排布。
- `Horizontal`：按 X 轴排布，可在溢出时换行。
