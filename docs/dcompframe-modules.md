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

## Application / Window

职责：提供应用生命周期与单窗口宿主骨架，不承载 demo 业务控件本身。

- `Application`：配置加载、渲染管理器初始化/销毁、窗口创建、消息循环驱动、连续渲染节流与诊断导出。
- `Window`：`WindowHost` + `WindowRenderTarget` 宿主、`build()` 扩展点、根内容与布局回调接线。
- demo 层通过 `WindowFactory` 生成自己的 `DemoWindow`，在 demo 中完成控件创建和事件绑定。

## UIElement / LayoutManager

职责：维护视觉树、可视属性与脏标记，提供基础布局策略。

- 子节点增删、父子关系维护。
- 事件分发支持捕获、目标、冒泡阶段。
- 属性：边界、透明度、裁剪、边距、平移、缩放、旋转、焦点。
- 脏标记：变更时自动向上级传播。
- `LayoutManager`：支持 `Stack` / `Grid` 策略。

## GridPanel

职责：基于行列将子元素映射到矩形网格。

- 支持行列设置。
- 支持子元素单元格与跨度设置。
- 计算并写回每个子元素边界。

## StackPanel

职责：按方向堆叠子元素并支持间距与换行。

- `Vertical`：按 Y 轴顺序排布。
- `Horizontal`：按 X 轴排布，可在溢出时换行。
- 默认行为：容器填满父容器，交叉轴对子项做拉伸。

## 控件层

职责：提供可复用基础控件与卡片组件。

- `Panel`：容器控件。
- `Panel::arrange`：按可用区域安排子元素，默认拉伸填满。
- `Panel` 默认背景透明、边框透明、边框厚度为 0。
- `GridPanel` / `StackPanel` 仅负责排布，不绘制背景，因此默认保持透明布局语义。
- 控件声明已从 `controls.h` 拆分为独立头文件，`controls.h` 仅保留聚合入口，降低头文件耦合。
- `TextBlock`：文本控件。
- `Image`：图像资源引用控件。
- `Button`：支持回调点击与禁用态。
- `Card`：支持标题、正文、图标、标签与主操作按钮组合。
- `TextBox`：支持占位文本、可观察字符串绑定、文本选择和输入法组合提交。
- `RichTextBox`：富文本输入控件，默认左上对齐，支持光标、选区、插入、删除、全选、上下行导航与内部滚动偏移。
- `ListView`：支持数据项、选中索引、分组、内部滚动偏移、可视范围计算与滚动条拖拽。
- `ItemsControl`：支持列表项集合、选中索引、项间距、内部滚动偏移、可见范围计算与滚动条拖拽，可作为 `ScrollViewer` 内容源，默认按垂直堆叠布局呈现。
- `ScrollViewer`：支持滚动偏移、惯性滚动与滚动条拖拽，默认不参与焦点轮转。
- `CheckBox`：支持勾选态与选中样式。
- `ComboBox`：支持数据项、选中索引与选中文本读取。
- `Slider`：支持范围和数值设置。
- `Label`：轻量标签控件，用于状态文案和可访问名称承载。
- `Progress`：支持范围、当前值、确定/不确定模式。
- `Loading`：支持激活态、overlay 模式与状态文案。
- `TabControl`：支持 tab 集合、选中索引和前后切换。
- `Popup`：支持 open/modal/title/body 状态模型。
- `Expander`：支持头部、正文和展开/收起状态。
- 文本对齐：除 `RichTextBox` 外默认水平/垂直居中。

## 样式与主题

职责：统一管理控件视觉风格。

- `Style`：背景色、前景色、边框、圆角等。
- `Theme`：按 key 解析样式，缺失 key 时直接抛异常（开发阶段无 fallback）。

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
	- `GridPanel` / `StackPanel` / `Panel` 保持 WPF 风格容器语义；按钮、输入框、下拉框、复选框、滑块、滚动容器统一收敛到 Element Plus 风格的圆角、边框、悬停和聚焦反馈。
	- demo 主内容区按纵向分区重排：标题区固定，表单区、列表区、滚动区依次堆叠，避免页面滚动内容覆盖标题。
	- 交互卡片布局改为左右表单分栏，`ComboBox` dropdown 使用 overlay 弹层，不再推挤后续控件，并在顶层顺序重绘以避免被遮挡。
	- 单行表单标签与值使用 no-wrap 文本格式，富文本说明区域使用独立 wrap 文本格式。
	- `ScrollViewer` 通过 `ItemsControl::visible_range()` 仅绘制 viewport 内可见项，并绘制轻量滚动 thumb。
	- `ListView` 与 `ItemsControl` 均支持内部滚动、视口裁剪与轻量滚动条；`ItemsControl` 默认以类 `StackPanel` 的纵向列表方式渲染。
	- `ScrollViewer`、`ListView`、`ItemsControl` 三类滚动条除 thumb 拖拽外，新增轨道点击跳转语义。
	- `LogBox` 滚动条接入轨道点击、thumb 拖拽与聚焦态视觉。
	- `ComboBox` dropdown 新增滚动条轨道/滑块命中与聚焦态。
	- 滚动聚焦态在 `WM_LBUTTONUP` / `WM_CAPTURECHANGED` / `WM_CANCELMODE` 时统一恢复，避免 thumb 松开后残留选中色。
	- `Panel` 默认样式透明无边框，保持布局容器语义。
	- demo 分区布局改为先在总高度内分配，再绘制各 section，避免窗口收缩时的区域重叠。
	- 新版 demo 布局进一步提升顶部表单区和 preview 卡片可用高度，减少底部滚动区对主交互面的挤压。
	- 新增控件在 demo 右侧预览区完成可视接入：`Label` chip、`TabControl` 页签、`Expander` 折叠区、`Progress` 进度条、`Loading` 状态徽标、`Popup` 预览层。
	- `TabControl` 新增鼠标点击切换，demo 预览卡片增加动画条与当前 tab 正文说明，避免“有控件无示例”。
	- `ScrollViewer`、`ListView`、`ItemsControl` 的滚动条 thumb 支持鼠标拖拽，滚轮滚动严格依赖当前 hover 区域。
	- 页面级滚轮偏移新增兜底路径：当未命中内层滚动容器时，滚轮作用于窗口主内容偏移，支持向下扩展布局内容。
	- 文本输入样式收口：`TextBox` 输入区内边距与 caret 高度统一，修复占位符裁剪和光标高度异常。
	- 视觉统一：背景清屏改为纯白，字体族统一为 `Microsoft YaHei UI`，字号向 Element Plus 14px 体系靠齐。
	- `RichTextBox` 采用内部滚动偏移 + 视口裁剪，长文本选择与 caret 绘制不会越出富文本框边界。
	- 父容器内容区使用 D2D clip，子元素溢出时自动裁剪。
	- 背景清屏色基于窗口尺寸计算，窗口缩放时背景呈现同步变化。
	- DirectWrite 默认字体族统一为 `Microsoft YaHei`，改善中文界面统一性。
	- overlay 按钮支持 hover/press/toggle 交互，按钮文字基于完整按钮矩形居中绘制。
	- checkbox/combobox/slider/scrollviewer 补齐专属视觉特征绘制。
	- D2D 初始化失败或 `EndDraw()` 运行时失败时，返回显式错误并记录诊断。
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
