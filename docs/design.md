# 设计决策

## 1. 分层架构

采用 7 层架构并保持低耦合：

- 渲染层：`RenderManager`、`CompositionBridge`
- 窗口层：`WindowHost`
- UI 核心层：`UIElement`、`LayoutManager`
- 布局层：`GridPanel`、`StackPanel`
- 控件层：`Panel`、`TextBlock`、`Image`、`Button`、`Card`、`Theme`
- 动画层：`AnimationManager`
- 工具辅助层：`ResourceManager`、`DeviceRecovery`、`DiagnosticsCenter`、`WindowRenderTarget`

## 2. 渲染后端策略

- 保留 `Simulated` 后端用于测试与快速验证，但不做运行时自动回退。
- 提供可插拔后端枚举：`Simulated` / `DirectX` / `DirectX12(预留)`。
- 当前渲染实现覆盖 DX11 硬件路径，DX12 保留插件位并给出明确未实现状态。
- 统一通过 `CompositionBridge` 执行提交门禁和节流。
- Fail-Fast 原则：开发阶段禁止静默降级与隐式 fallback，初始化或绘制失败直接返回错误并记录诊断。

## 2.2 渲染线程与命令缓冲

- `RenderManager` 提供线程安全命令队列（enqueue/drain）。
- 提供后台渲染线程生命周期（start/stop）以支持 UI 与渲染解耦。
- 当前阶段以基础设施和测试验证为主，后续接入真实异步提交与批处理调度策略。

## 2.1 窗口消息循环策略

- `WindowHost` 采用阻塞式消息循环（`GetMessage`）作为默认运行模式。
- 多窗口场景下仅在最后一个窗口销毁时投递退出消息，避免单窗口关闭导致全局循环提前退出。
- 测试场景可使用有限迭代循环（`max_iterations`）确保用例可控。
- 交互反馈依赖鼠标消息触发重绘请求（`WM_MOUSEMOVE/WM_LBUTTONDOWN/WM_LBUTTONUP`）。

## 3. 事件与脏标记策略

- 事件路由固定为 Capture -> Target -> Bubble。
- 属性变更统一触发 `mark_dirty()`，向父节点递归传播。
- 通过 `clear_dirty_recursive()` 在渲染提交后清空脏状态。

## 4. 控件与样式策略

- 控件统一继承 `StyledElement`，共享状态机和样式入口。
- `Theme` 提供 key-style 映射，开发阶段缺失 key 直接抛错，不做默认值回退。
- `Card` 通过组合子控件（主按钮）实现复杂组件复用。
- 默认文本对齐策略：除 `RichTextBox` 外，控件文本水平/垂直居中。
- 新增 `ComboBox` 与 `RichTextBox`，满足常见表单输入需求。
- `Panel` 作为基础容器提供 `arrange`：默认将子元素拉伸到容器可用区域，贴近 WPF Panel 容器职责。

## 5. 动画策略

- 使用时间片 `tick` 驱动，避免每帧重建视觉树。
- 提供 `Linear/EaseIn/EaseOut/EaseInOut` 缓动。
- 动画按属性维度独立，支持并发且可在 target 失效时自动回收。

## 6. 工程与依赖

- 构建：CMake（Preset + CTest）
- 包管理：vcpkg manifest
- 测试框架：GoogleTest
- 调试与任务：VS Code `tasks.json` + `launch.json`（x64/x86、Debug/Release）

## 7. DX11 + DComp 呈现策略

- `WindowRenderTarget` 在 DirectX 后端下创建并维护：
	- `ID3D11Device/ID3D11DeviceContext`
	- `IDXGISwapChain1`（`CreateSwapChainForComposition`）
	- `IDCompositionDevice/Target/Visual`
- 每帧执行 `ClearRenderTargetView -> Present -> DComp Commit`，确保 demo 有实际可见绘制内容。
- `Present` 返回 `DXGI_ERROR_DEVICE_REMOVED/RESET` 时进入设备丢失处理路径。
- 在 swapchain 上叠加 D2D/DirectWrite 绘制卡片与控件内容。
- D2D 初始化失败或运行时绘制失败时直接返回错误，不做几何 fallback。
- Overlay 交互状态（按钮 hover/press/toggle、列表 hover）统一在 D2D 主路径内处理。

## 8. DWM 兼容与后端策略

- 主路径：`WS_EX_NOREDIRECTIONBITMAP + DirectComposition + DX11`。
- `RenderManager` 对 `DirectX` 与 `Simulated` 后端采用显式选择，不做隐式自动切换。
- DComp 探测使用 `IDCompositionDevice` 接口，避免探测误失败导致透明窗口误判。
- demo 启动策略：首选后端失败时直接显式失败，保证问题可观测。

## 9. 交互系统策略

- `TextBox` 增加文本选择与输入法组合提交模型（selection + composition）。
- `ListView` 增加分组模型和虚拟窗口范围计算（virtual range）。
- `ScrollViewer` 增加惯性滚动速度模型与阻尼衰减。
- `ComboBox` 下拉层采用 overlay 策略，不参与主文档流布局，避免展开时推挤后续控件。
- `WindowHost` 必须转发 `WM_GETDLGCODE`、`WM_CHAR`、`WM_MOUSEWHEEL` 给渲染目标，确保文本输入和滚轮滚动走标准 Win32 消息链路。
- 输入命令系统基于快捷键路由（如 Ctrl+S）实现轻量 command dispatch。

## 9.1 Element Plus 表单布局策略

- 非布局控件采用表单式两列排布：左列承载输入控件，右列承载展示类控件，底部保留独立滚动区。
- 单行表单标签统一禁止自动换行；说明性文本和富文本区域使用单独的 wrap 文本格式。
- `ScrollViewer` 与 `ItemsControl` 组合使用可见范围裁剪，优先控制绘制成本而不是在渲染阶段遍历全部项。
- `ListView` 与 `ItemsControl` 默认持有独立滚动偏移，溢出内容不再依赖外层容器才能滚动。
- `ItemsControl` 默认采用类 `StackPanel` 的垂直堆叠呈现，避免标签云式布局影响表单阅读节奏。
- `ComboBox` 弹层必须在主内容裁剪结束后以顶层 overlay 顺序绘制，保证不被同层控件遮挡。
- 中文界面统一使用 `Microsoft YaHei`，减少 DirectWrite 默认西文字体与中文混排时的观感割裂。
- `ScrollViewer` 不参与焦点轮转，滚轮响应严格依赖鼠标 hover；需要键盘导航的场景由内部内容控件承担。
- 多行输入控件的滚动与 caret 可见性由控件自身偏移管理，渲染层只负责命中、裁剪和视口同步。

## 10. 布局系统收敛策略

- 布局系统保留 `GridPanel` 与 `StackPanel` 两种核心面板。
- 面板容器默认填满父容器，减少显式尺寸配置负担。
- `StackPanel` 交叉轴默认拉伸，主轴按子项期望尺寸堆叠，行为与 WPF 设计理念保持一致。

## 11. UI 需求基线约束

- 从 2026-04-10 起，UI 相关开发统一以 `docs/ui-requirements/` 下文档为准：
	- `ui-framework-design-spec.md`
	- `ui-controls-design-spec.md`
- 输入、焦点、滚轮冒泡、弹层层级、容器裁剪行为必须按文档验收条款实现。
- 所有 UI BUG 修复必须建立“问题映射 -> 测试用例 -> 验收条款”的追踪关系，防止回归。

## 12. demo 纵向分区决策

- demo 页面采用“标题区固定 + 内容区滚动”的纵向分区策略，而不是让整张卡片在统一裁剪区内混合滚动。
- 右侧预览卡片内部采用紧凑排布，优先保证 `TabControl`、动画示例、`Expander` 和状态控件在 960x720 级别窗口中全部可见。
- 列表型控件从右侧窄列中抽离为独立列表分区，以提高 `ListView` 与 `ItemsControl` 的有效可视高度。
