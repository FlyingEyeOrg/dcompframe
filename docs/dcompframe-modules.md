# 模块文档

## RenderManager / CompositionBridge

职责：管理渲染后端初始化、提交门禁与统计。

- `RenderManager::initialize(bool)`：模拟后端初始化。
- `RenderManager::initialize_with_backend(RenderBackend)`：按后端策略初始化（含 DirectX 路径）。
- DirectX 探测阶段使用 `IDCompositionDevice` 接口校验 DComp 能力，避免误判回退。
- `RenderManager::shutdown()`：清理状态和资源注册表。
- `RenderManager::supported_backends()`：返回可插拔后端列表。
- `RenderManager::enqueue_command/drain_commands`：命令缓冲与批量提交基础能力。
- `RenderManager::start_render_thread/stop_render_thread`：渲染线程生命周期控制。
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
- 鼠标移动与按键消息会触发重绘请求，保证交互可见状态实时刷新。

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
- `TextBox`：支持占位文本、可观察字符串绑定、文本选择和输入法组合提交。
- `ListView`：支持数据项、选中索引、分组与可视范围计算。
- `ScrollViewer`：支持滚动偏移与惯性滚动。
- `CheckBox`：支持勾选态与选中样式。
- `Slider`：支持范围和数值设置。

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
	- DirectX 后端下创建 `DXGI SwapChain for Composition` 并绑定 `IDCompositionVisual`。
	- 使用 `D2D/DirectWrite` 在 swapchain 上绘制可见卡片与控件内容。
	- 背景清屏色基于窗口尺寸计算，窗口缩放时背景呈现同步变化。
	- overlay 按钮支持 hover/press/toggle 交互，列表区域支持逐项 hover 高亮。
	- D2D 初始化失败或 `EndDraw()` 运行时失败时，使用 DX11.1 `ClearView` 绘制几何控件块兜底，保持控件可见。
	- `render_frame` 执行清屏、`Present` 与 `IDCompositionDevice::Commit`。
	- 设备丢失时通过 `DeviceRecovery` 触发恢复并重建渲染目标视图。

## 输入系统

职责：处理焦点环、点击/双击、拖拽与快捷键命令路由。

- `InputManager`：焦点切换、点击、双击、拖拽回调。
- `InputManager::register_shortcut/on_key_down`：快捷键注册与命令分发。

## 数据绑定

职责：将状态变化同步到控件/组件。

- `Observable<T>`：可观察值容器。
- `BindingContext`：业务字段绑定容器。
- `Card::bind` / `TextBox::bind_text` / `Button::bind_enabled`。

## 配置与错误

职责：配置驱动运行参数与统一错误边界。

- `AppConfigLoader`：JSON 配置加载。
- `Status` / `ErrorCode`：统一状态返回。
