# 模块文档

## RenderManager / CompositionBridge

职责：管理渲染后端初始化、提交门禁与统计。

- `RenderManager::initialize(bool)`：模拟后端初始化。
- `RenderManager::initialize_with_backend(RenderBackend)`：按后端策略初始化（含 DirectX 路径）。
- `RenderManager::shutdown()`：清理状态和资源注册表。
- `CompositionBridge::bind_target_handle(HWND)`：绑定提交目标。
- `CompositionBridge::commit_changes(bool)`：执行提交门禁并记录提交计数/帧统计。

## WindowHost

职责：管理窗口生命周期、消息循环与渲染调度。

- `create/destroy`：创建或销毁窗口资源。
- 默认扩展样式包含 `WS_EX_NOREDIRECTIONBITMAP`。
- `run_message_loop`：处理消息并调度渲染回调。
- `on_size_changed`：更新客户区并触发重绘。
- `apply_dpi`：基于 96 DPI 基线更新缩放并触发重绘。
- `set_window_state`：支持 Normal/Minimized/Maximized/Fullscreen。

## UIElement / LayoutManager

职责：维护视觉树、可视属性与脏标记，提供基础布局策略。

- 子节点增删、父子关系维护。
- 事件分发支持捕获、目标、冒泡阶段。
- 属性：边界、透明度、裁剪、边距、平移、缩放、旋转、焦点。
- 脏标记：变更时自动向上级传播。
- `LayoutManager`：支持 `Absolute` / `Stack` / `Grid` 策略。

## GridPanel

职责：基于行列将子元素映射到矩形网格。

- 支持行列设置。
- 支持子元素单元格与跨度设置。
- 计算并写回每个子元素边界。

## StackPanel

职责：按方向堆叠子元素并支持间距与换行。

- `Vertical`：按 Y 轴顺序排布。
- `Horizontal`：按 X 轴排布，可在溢出时换行。

## 控件层

职责：提供可复用基础控件与卡片组件。

- `Panel`：容器控件。
- `TextBlock`：文本控件。
- `Image`：图像资源引用控件。
- `Button`：支持回调点击与禁用态。
- `Card`：支持标题、正文、图标、标签与主操作按钮组合。

## 样式与主题

职责：统一管理控件视觉风格。

- `Style`：背景色、前景色、边框、圆角等。
- `Theme`：按 key 解析样式并支持 fallback。

## 动画层

职责：提供属性动画和缓动。

- `AnimationManager::add` / `tick`。
- 支持属性：位置、透明度、旋转、裁剪尺寸。
- 支持缓动：Linear、EaseIn、EaseOut、EaseInOut。

## 工具与辅助层

职责：增强稳定性、诊断与窗口呈现。

- `ResourceManager`：统一资源注册与统计。
- `DeviceRecovery`：设备丢失与恢复计数。
- `DiagnosticsCenter`：日志和帧耗时统计。
- `WindowRenderTarget`：窗口与提交桥接，记录呈现帧数。
