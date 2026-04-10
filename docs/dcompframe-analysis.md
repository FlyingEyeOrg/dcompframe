# 2026-04-10 分析补记

## DWM 兼容默认路径与 Demo 重叠根因（2026-04-11）

- 这轮窗口回归的根因，不是“标题栏画得不够像 Chrome”，而是系统非客户区语义被接管后没有完整保住：默认窗口扩展样式过于激进，自定义 caption 按钮也没有回到系统 hit-test 结果。
- 调整后，默认窗口回到 `WS_EX_APPWINDOW`，自定义标题栏仍保留，但 caption 按钮返回 `HTMINBUTTON/HTMAXBUTTON/HTCLOSE`，拖拽区返回 `HTCAPTION`，边缘 resize 依据系统 frame metrics 计算，行为已经更接近 Chromium 在 Windows 上的 CustomFrame 处理思路。
- Demo 重叠的根因也已经明确：标签文本是画在控件边界之外的，而旧 Demo 没有给这些标签留出布局空间，所以视觉上像“Flex 算错了”，其实是 demo 设计把渲染占位漏掉了。
- 这也是为什么本轮修复落在 `demo/main.cpp` 的 margin/padding/固定高度，而不是继续去改 `FlexPanel` 主轴分配算法。

## Flex-only + Custom Caption 分析（2026-04-10）

- 本轮真正解决的不是“再修一次 demo 重叠”，而是把两个根因一起拆掉：
	- 旧布局入口仍然存在，团队仍可能继续写 `Grid/Stack`。
	- 渲染层仍维护一套 preview-card 派生几何，和真实控件边界脱节。
- 删除 `GridPanel` / `StackPanel` 主路径后，布局约束终于统一到 `FlexPanel`；补上 `padding`、`min/max`、`space-evenly` 后，Flex 栈已经足以承接 demo 页面。
- `WindowRenderTarget` 改为直接消费 `TabControl`、`Expander`、`Progress`、`Popup` 等控件的真实 bounds，意味着“控件在布局树里在哪里，就在那里绘制和命中”，不再依赖 card 内部数学推导。
- 自定义标题栏的价值也不在样式，而在行为闭环：`WM_NCCALCSIZE`、`WM_NCHITTEST`、caption 按钮、Per-Monitor DPI 和 `SWP_FRAMECHANGED` 终于进入同一条链路。
- 当前剩余风险已从“布局/命中模型分裂”下降为“后续新增复杂控件时是否继续坚持真实 bounds 驱动”，这是流程约束问题，不再是基础设施缺口。

- 编译失败主因不是业务逻辑错误，而是 MSVC 并发编译下的 PDB 写入冲突，以及控件拆分过程中遗留的截断源文件。
- 当前收口方案保留独立控件头提升可维护性，但仍以 `src/controls/controls.cpp` 作为唯一实现单元，避免半拆分状态继续扩散。
- UI 风格层面明确区分两类职责：`GridPanel` / `StackPanel` / `Panel` 维持 WPF 容器语义，其余控件统一映射为 Element Plus 的浅色表单视觉系统。

## Flexbox Only 分析（2026-04-10）

- 本轮布局收敛的关键点，不是“再把 StackPanel 做强一点”，而是显式引入 `FlexPanel` 作为 Web Flexbox 语义的宿主。
- 这样做之后，布局、hit-test、事件路由和 demo 文档终于能共享同一套术语：direction、wrap、basis、grow、target、currentTarget、capture、bubble。
- 原先“当前交互为 demo overlay 级命中测试，尚未与完整 UIElement 命中树统一”的风险已被显著收敛：窗口鼠标消息已开始经 `InputManager` 路由进元素树。
- 仍然保留的风险是：`GridPanel` 还在仓库中存在，若没有规范门禁，后续开发者仍可能把它继续用于新页面布局；因此必须配套 `flexbox-only-spec.md` 做流程约束。
# 专项分析：完整实现阶段可行性与风险

## 目标

评估在完整模块实现（渲染/窗口/UI/布局/控件/动画/工具）下的可维护性和稳定性。

## 结论

整体可行且已落地。当前实现通过 33 个测试覆盖核心行为，
并通过 `dcompframe_demo` 验证端到端链路（窗口 -> 控件/动画 -> 渲染提交 -> 诊断输出）。

## 风险

- 多窗口并发场景下的提交节流与诊断聚合尚需加强。
- 驱动级故障注入（真实 GPU reset）仍需补充，目前以集成循环压测替代。
- 命令缓冲与渲染线程目前为基础设施阶段，尚未引入真实异步管线性能优化。
- 开发阶段已移除渲染兜底路径，D2D 初始化/绘制失败会直接失败；需持续跟进环境兼容数据。
- 当前仍有一部分 demo 视觉由 render target 直接绘制 token 驱动，而不是完全走 `Theme` 管线；后续若继续扩展样式系统，需要把这套 token 正式沉到统一主题入口。

## 建议

- 新增集成测试：消息循环 + 提交节流 + 多窗口。
- 引入 D2D/DirectWrite 渲染到纹理的集成用例。
- 继续推进设备丢失故障注入与恢复耗时统计。
- 增加 DWM 兼容矩阵回归（Windows 8.1/10/11，混合 DPI）。
- 推进 DX12 后端插件实现并加入同构测试矩阵。
- 增加“无兜底策略”专项回归，确保失败路径可观测且行为稳定可复现。

## 本次补丁结论（2026-04-10）

- 启动稳定性：demo 增加首选后端失败后的显式 Simulated 降级路径，已消除“打开即退出”问题。
- Panel 语义：补齐 `Panel::arrange`，实现 WPF 风格容器默认拉伸行为。
- 回归结果：x64 Debug 下 37 项测试全部通过。

## 第三轮 UI 收口分析（2026-04-10）

- `RichTextBox` 的根因不是渲染缺失，而是数据模型仍停留在只读文本容器；本轮通过补齐选区、光标和编辑 API，从模型侧解决不可编辑问题。
- `ComboBox` 被遮挡的根因不是命中测试，而是绘制顺序仍位于内容裁剪区内；本轮改为在 clip 结束后再次以 overlay 顺序绘制 dropdown，问题被根治。
- `ScrollViewer`、`ListView`、`ItemsControl` 的共同问题是“只有可见样式，没有内聚的滚动状态”；本轮为列表类控件补齐内部 `scroll_offset`，把滚动能力收回控件自身。
- `ItemsControl` 从标签流式排布切回类 `StackPanel` 纵向列表后，阅读节奏和表单信息密度更符合 Element Plus 在桌面端的使用场景。
- 字体统一切换到 `Microsoft YaHei` 后，中文标题、说明和表单值在 DirectWrite 下的视觉重量更一致，解决了默认英文字体混排时的割裂感。
- 回归结果：x64 Debug `43/43` 全部通过，说明第三轮收口未破坏既有输入、渲染和稳定性基线。

## 第四轮滚动与多行输入分析（2026-04-10）

- `ScrollViewer` 的根因是“把容器当成可聚焦输入控件”，因此鼠标离开后仍会借助焦点继续消费滚轮；本轮通过取消焦点参与，行为收口为纯 hover 驱动。
- `RichTextBox` 的根因是“有编辑模型但没有视口模型”，因此长文本 caret、选区和鼠标拖拽都缺少滚动约束；本轮补上内部 `scroll_offset` 后，编辑与视口开始统一。
- `ListView`/`ItemsControl` 的滚动问题不在 wheel，而在缺少 thumb 命中与拖拽映射；本轮通过统一 thumb 几何和偏移换算，补齐了桌面列表应有的直接操控能力。
- Demo 中 `ItemsControl` 被 `ScrollViewer` 遮挡的根因是上下区块按固定高度切分；本轮改为先保留底部滚动区，再反推上半区高度，避免区块重叠。

## 规范驱动控件补齐分析（2026-04-10）

- 按 `ui-framework-design-spec.md` 复盘后，框架缺口主要在“有容器/输入控件，但缺状态展示控件”。
- 补齐 `Label/Progress/Loading/TabControl/Popup/Expander` 后，框架控件族从交互单点修复进入“可完整演示 + 可回归测试”状态。
- `WindowRenderTarget` 接入新增控件后，右侧卡片预览区形成稳定的 Element Plus 风格展示面板，避免 demo 与规范脱节。
- 新增集成测试对滚轮消费边界做了明确约束：ComboBox 下拉打开时滚轮不再穿透外层 `ScrollViewer`。
- 当前风险点已从“缺控件”转移为“复杂交互深度不足”（例如 Popup 完整焦点陷阱、TabControl 手动/自动激活切换策略），可在下一轮按 TDD 深化。

## UI 视觉与布局反馈修复分析（2026-04-10）

- 用户反馈集中在三类问题：文本输入视觉异常、ComboBox 展开异常、右侧预览布局拥挤重叠。
- 根因：
	- TextBox 输入字号与内边距不匹配，导致占位符被裁剪，caret 仍使用偏高固定值。
	- ComboBox 下拉滚动偏移在多次展开时未重置。
	- 预览卡片内部新增控件后仍沿用旧分区比例，导致纵向空间不足。
- 修复策略：
	- 输入区按 14px 体系重配内边距与 caret 度量下限。
	- ComboBox 展开时清零 dropdown 偏移，键盘上下导航再按需滚动。
	- 右侧卡片重分区，新增页面级滚轮偏移兜底路径，并维持内层滚动优先级。

## 本轮最终收口（2026-04-10）

- `Application` / `Window` 仍位于主库，但职责已从“demo 业务承载层”收敛为“生命周期与宿主骨架”。
- demo 控件树、事件绑定和窗口标题全部回迁到 `demo/main.cpp`，框架边界恢复清晰。
- 连续渲染触发条件收紧到动画态，空闲场景不持续刷帧。
- 布局容器默认透明无边框，视觉职责下沉到具体控件。
- demo 分区布局改为受总可用高度约束的分配模型，小窗口下不再互相覆盖。
- 最新回归：`ctest --preset vs2022-x64-debug-tests`，`49/49` 通过。

## 本轮补充六（2026-04-10）

- 根据 Microsoft Learn 的 `SetCapture` / `ReleaseCapture` / `WM_CAPTURECHANGED` 语义，原实现存在一个明确缺口：释放鼠标后虽然停止了拖拽，但没有同步清理滚动聚焦态，因此颜色残留。
- 根据 Windows UX Layout 指南，旧 demo 布局的问题不是“数学算错一点”，而是顶部核心交互区被底部滚动区过度挤压，整体显得 cramped。
- 本轮通过两步修正：
	- 拖拽结束和捕获变化时统一清空 `focused_scroll_target_` 并重绘。
	- 调整顶部/中部/底部区域比例，把空间让回给表单区和 preview 卡片，同时限制 preview 内部各子块的最大高度。
- 最新回归：`49/49` 通过。

## 本轮补充七（2026-04-10）

- 本轮进一步确认：仅靠继续修 section 比例，并不能根治 demo 的“布局混乱”，因为根因是结构没有容器语义。
- 修复方向从“继续手调 overlay 数学”切换为“先建立 section 容器结构，再让渲染层对齐它”。
- 当前 demo 已形成明确层次：
	- 外层纵向 `StackPanel`
	- 顶部展示区 `GridPanel(1x2)`
	- 中部集合区 `GridPanel(1x2)`
	- 底部 `ScrollViewer` section
	- 底部 `LogBox` section
- 最新回归：`49/49` 通过。

## 本轮补充八（2026-04-10）

- 进一步复盘后确认，`compute_demo_stack_metrics(...)` 之类的公式不是根因修复，而是掩盖问题的补丁层。
- 真正的根因是：
	- 布局面板自身的 `Arrange` 语义错误。
	- 渲染层没有消费真实布局边界。
- 本轮修复把问题拉回到框架层：
	- 布局树负责给出真实边界。
	- render target 负责读取和呈现这些边界。
	- demo 专用分区公式不再留在主项目渲染实现中。
- 最新回归：`49/49` 通过。

## 本轮补充十（2026-04-10）

- 继续复盘后确认，上一轮虽然修掉了 Arrange 根因，但元素尺寸模型仍然不完整，容器还缺“先测量内容、再分配空间”的能力。
- 新的重构把布局模型进一步拉齐到 WPF/Chromium 的共性：
	- `UIElement` 先给出所需尺寸
	- `StackPanel` 按主轴累计并分配剩余空间
	- `GridPanel` 按内容贡献求轨道尺寸
	- `Window` 直接驱动根节点 `measure + arrange`
- 旧 demo 已整体替换为布局验证窗口，目标从“展示很多控件”切换为“先证明容器系统本身成立”。
- 最新回归：命令行构建 `dcompframe_demo` / `dcompframe_tests` 通过，`54/54` 通过。
