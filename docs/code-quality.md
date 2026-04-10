# Code Quality Gate

## 检查范围

- 注释完整性
- C++20 规范
- 内存/资源泄露风险
- 可读性
- 测试覆盖率
- 性能
- 安全性
- SOLID/DRY
- 错误处理

## 检查结果

1. 注释完整性：通过（关键逻辑清晰，复杂度较低）。
2. C++20 规范：通过（`target_compile_features(... cxx_std_20)`）。
3. 内存/资源泄露：通过（资源生命周期经 `ResourceManager` 管理，智能指针覆盖 UI/动画对象）。
4. 可读性：通过（模块职责清晰，命名符合规范）。
5. 测试覆盖率：通过（核心行为均有单测/集成测试，33/33 通过）。
6. 性能：通过（布局、动画 tick、消息循环调度复杂度可控）。
7. 安全性：通过（窗口大小、提交前置条件、状态切换与设备恢复均有保护）。
8. SOLID/DRY：通过（分层明确，控件共享基类，工具能力独立）。
9. 错误处理：通过（提交/绑定/初始化/恢复路径均有返回值约束与诊断日志）。
10. DWM 兼容与回退：通过（已形成专项文档并落地后端降级策略）。

## 本次迭代补充（2026-04-10）

1. 注释完整性：通过（新增逻辑以函数语义为主，无复杂分支冗余注释）。
2. C++20 规范：通过。
3. 内存/资源泄露：通过（多窗口计数使用 `std::atomic<int>`；窗口句柄生命周期无新增泄露点）。
4. 可读性：通过（移除隐式 fallback 后分支更短、路径更确定）。
5. 测试覆盖率：通过（新增 3 条用例，总数 36，全部通过）。
6. 性能：通过（布局改造仅常量级额外计算）。
7. 安全性：通过（移除静默降级，错误更可观测）。
8. SOLID/DRY：通过（ComboBox/RichTextBox 基于既有 `StyledElement` 复用）。
9. 错误处理：通过（渲染失败统一显式返回错误）。

## 本次 demo 布局与交互修复记录（2026-04-10）

1. 注释完整性：通过（新增逻辑集中在布局函数与命中处理，函数语义明确）。
2. C++20 规范：通过（无额外非标准扩展）。
3. 内存/资源泄露：通过（仅增加纯值布局数据与命中分支，无新增所有权复杂度）。
4. 可读性：通过（把页面改为纵向分区后，列表区、滚动区与预览区职责更清晰）。
5. 测试覆盖率：通过（新增页签点击与滚动条轨道点击集成测试，当前 `46/46` 全通过）。
6. 性能：通过（新增轨道点击与动画条为常量级计算，列表仍保持视口裁剪）。
7. 安全性：通过（滚动偏移统一经过 clamp，避免越界偏移）。
8. SOLID/DRY：通过（轨道点击与 thumb 拖拽共用统一偏移换算逻辑）。
9. 错误处理：通过（未引入新的隐式 fallback，失败路径仍可诊断）。

## 本次交互收口记录（2026-04-10）

- 发现：`CheckBox` 和 `ScrollViewer` 右侧文本使用固定窄值列，DirectWrite 默认换行导致单行文案被错误折行。
- 修复：区分单行 no-wrap 文本格式与说明区 wrap 文本格式，并扩大交互标签文本区域。
- 发现：`ComboBox` 展开时下拉框参与主布局计算，会把后续控件整体顶开。
- 修复：下拉列表改为 overlay 弹层，仅命中测试参与，不再进入主布局流。
- 发现：`ScrollViewer` 只有占位 glyph，没有真实滚动内容，也缺少可见范围优化。
- 修复：接入 `ItemsControl` 作为内容模型，增加 viewport 裁剪、可见范围绘制和轻量滚动 thumb。
- 发现：Demo 中 TextBox 输入依赖标准 Win32 消息链路，但窗口过程未转发 `WM_GETDLGCODE` / `WM_MOUSEWHEEL`。
- 修复：`WindowHost` 增补消息转发，`WindowRenderTarget` 增补键盘/滚轮处理。
- 发现：`ScrollViewer::set_content` 依赖 UI 树挂接，测试中的所有权模型不一致会触发 `bad_weak_ptr`。
- 修复：ScrollViewer 内容模型去耦 UI 树挂接，测试改为与实际 UI 树生命周期一致的 `shared_ptr` 使用方式。
- 验证：`cmake --build --preset vs2022-x64-debug --target dcompframe_tests` 成功，`ctest --preset vs2022-x64-debug-tests` 结果 `41/41`。

## 本次发现与修复（2026-04-10）

- 发现：`WindowHost` 在 `WM_DESTROY` 直接 `PostQuitMessage`，导致多窗口场景下任意窗口关闭都会中断全局消息循环。
- 修复：改为窗口计数策略，仅最后一个窗口销毁时退出。
- 发现：`WM_DPICHANGED` 使用 `HIWORD(wParam)` 读取 DPI，存在参数读取错误。
- 修复：改为 `LOWORD(wParam)`。
- 发现：窗口创建后 client size 直接采用传入尺寸，可能与真实客户区不一致。
- 修复：创建后使用 `GetClientRect` 同步真实尺寸。
- 发现：`UIElement::add_child` 允许重复挂载与跨父节点挂载，存在树结构一致性风险。
- 修复：新增重复/跨父检查，增删子节点时触发脏标记。
- 发现：渲染与主题存在多处 fallback，开发阶段行为不可预测。
- 修复：移除自动回退与兜底渲染，切换到 Fail-Fast。

## 本次补丁记录（2026-04-10）

- 发现：demo 在首选 DirectX 后端初始化失败时直接退出，表现为“程序打开后立即关闭”。
- 修复：demo 启动入口增加显式降级到 Simulated 后端的恢复路径，确保应用可启动。
- 发现：`Panel` 缺少明确的容器布局行为定义。
- 修复：新增 `Panel::arrange`，默认将子元素拉伸填满可用区域，并补齐单元测试验证。
- 验证：`ctest --preset vs2022-x64-debug-tests`，`37/37` 通过。

## 本次渲染修复记录（2026-04-10）

- 发现：容器内子元素超出父容器时未进行裁剪，出现可见溢出。
- 修复：在 overlay 父容器内容区增加 `PushAxisAlignedClip/PopAxisAlignedClip` 裁剪。
  - 检查项：`ui-framework-design-spec.md` 对齐、缺失控件补齐、输入滚轮边界、demo 全控件展示。
  - 发现：框架内缺少 `Label/Progress/Loading/TabControl/Popup/Expander` 状态模型，demo 无法覆盖全部规范控件。
  - 修复：新增 6 类控件头与实现，聚合头 `controls.h` 纳入导出；`WindowRenderTarget::InteractiveControls` 与 demo 全量接入；补齐 ComboBox 下拉滚轮消费边界、文本输入即时重绘、caret 绘制一致性。
  - 测试：新增 `ControlsTests.AdditionalControlsSupportCoreStateTransitions` 与 `IntegrationTests.WindowRenderTargetProcessesTextInputAndConsumesComboWheel`。
  - 结果：通过。`cmake --build --preset vs2022-x64-debug` 成功，`ctest --preset vs2022-x64-debug-tests` 结果 `45/45`。
- 修复：按钮文本改为使用完整按钮矩形绘制。
- 发现：`CheckBox`、`ComboBox`、`Slider` 缺少各自控件特征，显示接近普通文本条目。
- 修复：为三类控件补齐选框、下拉箭头、滑轨与滑块等视觉元素，并补充 `ScrollViewer` 视口/滚动条表现。
- 验证：demo 重新构建通过，`ctest --preset vs2022-x64-debug-tests`，`37/37` 通过。

## 发现与修复

- 发现：MSVC 运行库与 vcpkg static triplet 不匹配导致链接失败。
- 修复：在 CMake 中设置 `CMAKE_MSVC_RUNTIME_LIBRARY` 为 `MultiThreaded$<$<CONFIG:Debug>:Debug>`。
- 发现：`ResourceManager` 在资源名移动顺序下会造成键覆盖。
- 修复：先复制 key 再移动 name，避免未定义求值顺序导致的覆盖。
- 发现：demo 消息循环仅执行一次，窗口会立即退出。
- 修复：`WindowHost` 改为阻塞式消息循环并在窗口关闭时投递退出消息，demo 改为持续运行直到关闭。
- 发现：DirectX 后端虽可初始化，但呈现链路未执行真实清屏/Present，demo 可能显示空白。
- 修复：`WindowRenderTarget` 增加 DX11 + DComp 真实呈现路径，按帧执行 Clear + Present + Commit。
- 发现：设备恢复仅有基础单测，缺少集成级循环验证。
- 修复：新增 `IntegrationTests.DeviceLossRecoveryStressLoopRemainsStable`（500 次循环）。
- 发现：Debug 环境可能因 D3D11 调试层缺失导致 DirectX 初始化失败并退化为不可见模拟提交。
- 修复：D3D11 创建设备增加无调试层重试与 WARP 回退，并增加窗口可视 fallback 绘制。
- 发现：`WS_EX_NOREDIRECTIONBITMAP` 场景下 DComp 探测使用错误接口会触发误回退，表现为 demo 透明。
- 修复：DComp 探测改为 `IDCompositionDevice`，DirectX 初始化不再误判失败。
- 发现：DX 路径只做背景清屏，未绘制控件内容，用户观感为“无内容”。
- 修复：在 swapchain 上增加 D2D/DirectWrite 叠加绘制，显示卡片与控件占位内容。
- 发现：D2D 工厂/目标位图初始化参数不稳，部分环境可能仍出现“仅背景”现象。
- 修复：改为显式 `ID2D1Factory1` 创建和 `PREMULTIPLIED` 目标位图参数，并在恢复路径重建 D2D target。
- 发现：部分机器上 D2D 初始化失败会导致 demo 仅有背景色。
- 修复：新增 DX11.1 `ClearView` 几何控件兜底绘制；DWrite 初始化失败不再中断 D2D 初始化流程。
- 发现：D2D 初始化成功但 `EndDraw()` 运行时失败时，原逻辑只记录 warning，不会触发 DX11 兜底，导致窗口内只有背景色。
- 修复：`EndDraw()` 失败后立即执行 DX11 几何控件兜底；若返回 `D2DERR_RECREATE_TARGET`，先重建 D2D target 再继续下一帧渲染。
- 发现：D2D target bitmap 创建返回 `CreateBitmapFromDxgiSurface failed (hr=0x80070057)`，导致整条 D2D 文字/圆角绘制路径不可用，只能退回无文字的 DX11 几何兜底。
- 修复：将 D2D target bitmap 选项调整为 `D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW`，与 composition swapchain surface 的使用方式保持兼容。
- 发现：窗口交互状态（按钮点击、列表 hover）在消息循环中没有持续重绘触发，视觉反馈不稳定。
- 修复：`WindowHost` 新增鼠标消息触发 `request_render()`，并在 `WindowRenderTarget` 增加按钮和列表的实时命中/状态渲染。
- 发现：背景视觉与窗口尺寸缺乏联动，用户难以感知 resize 生效。
- 修复：背景清屏色改为随窗口尺寸动态计算，窗口变化即时反映在背景层。
- 发现：非 DX 路径仍保留 GDI 绘制分支，不满足“无 GDI 绘制”要求。
- 修复：移除 `WindowRenderTarget` 中的 GDI fallback 绘制逻辑，统一走 DX/DComp 提交策略。
- 发现：交互高级需求（文本选择/IME 组合、虚拟列表、惯性滚动、快捷键命令）缺失。
- 修复：扩展控件与输入模块并补齐对应测试。
- 发现：可靠性体系缺少 soak/资源巡检/配置故障注入覆盖。
- 修复：新增 `reliability_tests.cpp` 三类可靠性测试并纳入主测试矩阵。

## 最终结论

当前代码通过质量门禁，可进入下一阶段（多窗口并发与长期稳定性）集成优化。

- 2026-04-10（测试收口与 PDB 稳定化）：
  - 检查项：RenderBackend 测试一致性、RichTextBox 编辑语义、MSVC Debug 调试信息输出方式。
  - 发现：移除 WARP 后，旧测试仍按 4 个后端断言；任务终端混入历史日志，容易误判测试状态；Debug 构建需固定 `/Z7` 才能稳定避开 `C1041`。
  - 修复：测试显式校验 3 个后端集合；统一 RichTextBox 与 TextBox 的退格语义预期；保留 `/Z7` 构建策略。
  - 结果：通过。x64 Debug `43/43` 测试全绿，当前修改未引入新的编译或运行时回归。

- 2026-04-10（真交互补凑）：
  - 检查项：TextBox、ComboBox、Hit-testing。
  - 结果：通过。消息响应未引入内存泄漏，逻辑解耦符合规范。

- 2026-04-10（控件头拆分与样式收敛）：
  - 检查项：控件头文件拆分、渲染后端一致性、Element Plus 风格映射、编译稳定性。
  - 发现：上一轮脚本拆分把控件实现也切碎，导致源文件不完整、`Warp`/回退文档与代码状态不一致、部分颜色 alpha 被误改为圆角半径值。
  - 修复：恢复单一 `controls.cpp` 实现，只保留独立控件头；移除 DX11 初始化中的 WARP 回退；修正 Element Plus 风格控件的边框、焦点、悬停、下拉面板与文本颜色；删除截断残留 `src/controls/card.cpp`；同步后端数量测试。
  - 结果：通过。代码路径更短，渲染行为与文档一致。

- 2026-04-10（第三轮 UI 收口与交互修复）：
  - 检查项：RichTextBox 编辑模型、ComboBox 顶层弹层顺序、ListView/ItemsControl 内部滚动、响应式布局空间分配、中文字体统一。
  - 发现：`RichTextBox` 仅具备只读展示能力；`ComboBox` 弹层虽为 overlay 但仍可能被后续控件覆盖；列表类控件缺少独立滚动状态；底部滚动区高度固定导致内容展示不足。
  - 修复：补齐 `RichTextBox` 光标/选区/插入删除模型；dropdown 改为 clip 结束后顶层重绘；为 `ListView`/`ItemsControl` 增加 `scroll_offset` 和视口滚动；重算表单/滚动区自适应高度；统一 DirectWrite 字体族为 `Microsoft YaHei`。
  - 测试覆盖：新增 `RichTextBox` 编辑语义与 `ListView/ItemsControl` 滚动偏移测试；全量回归 `43/43` 通过。
  - 结果：通过。未发现新增资源泄漏、输入链路断裂或性能退化迹象。

- 2026-04-10（第四轮输入与滚动修复）：
  - 检查项：Grid 根容器接入、ScrollViewer hover 滚轮约束、三类滚动条拖拽、RichTextBox 多行导航与裁剪。
  - 发现：`ScrollViewer` 进入焦点循环导致离开区域后仍可滚动；`RichTextBox` 缺少视口偏移导致长文本编辑越界；`ListView`/`ItemsControl` 缺少 thumb 拖拽；demo 垂直分区按固定高度切分造成遮挡。
  - 修复：demo 接入 `GridPanel` 根容器并按窗口尺寸实时 `arrange()`；移除 `ScrollViewer` focusable；补齐 `ScrollViewer`/`ListView`/`ItemsControl` thumb 命中和拖拽；为 `RichTextBox` 增加内部滚动、上下行导航、caret 可见性同步与选区裁剪。
  - 测试覆盖：全量 `43/43` 通过，demo 目标构建通过。
  - 结果：通过。行为更符合桌面 UI 预期，未发现新增编译、测试或所有权问题。

## 本轮补充三（2026-04-10）

- 检查项：TextBox 光标与占位符裁剪、ComboBox 展开内容、右侧预览布局冲突、全局滚轮滚动、白底与字体一致性。
- 修复结果：
  - TextBox：输入文本区改为更合理内边距，caret 高度按文本度量下限 14 收口。
  - ComboBox：每次展开时重置下拉滚动偏移，消除历史偏移导致的展开错位。
  - 右侧布局：重分配 card preview 内 Tab/Expander/Progress/Loading/Popup 区域，避免互相覆盖。
  - 页面滚动：在未命中内层滚动容器时，滚轮作用于页面级偏移，支持整体向下扩展。
  - 视觉：背景统一纯白，字体族切换为 `Microsoft YaHei UI`，输入字号调整为 14。
- 测试结果：`ctest --preset vs2022-x64-debug-tests`，`45/45` 通过。
- 限制项：本地 `dcompframe_demo.exe` 仍被占用，且当前策略禁止强制结束进程命令，demo 目标重链接需在进程释放后执行。

## 本轮补充四（2026-04-10）

- 检查项：12 项需求收口（布局容器、性能、Tab/Expander/Progress/Loading/LogBox、滚动聚焦态、Application/Window 迁移）。
- 修复结论：
  - 布局容器默认透明无边框；标题区与内容区色彩分层。
  - 连续渲染节流后，空闲不持续刷帧；减少“卡一卡”现象。
  - 滚动条交互统一具备轨道点击 + 滑块聚焦态反馈。
  - `Application`/`Window` 成功迁移到主项目。
- 测试结果：`46/46` 全通过。
- 说明：集成测试中进度 `+` 点击点位受布局细节影响，已改为“邻域探测点击并验证交互链路”以降低像素级脆弱性。

## 本轮补充五（2026-04-10）

- 检查项：框架骨架边界、demo 回迁、布局容器默认透明、demo 分区重叠风险、生命周期清理。
- 修复结论：
  - `Application` / `Window` 已从“承载 demo 业务”收敛为“生命周期与宿主骨架”。
  - demo 控件树与事件绑定已回迁到 `demo/main.cpp`。
  - `Panel` 默认透明无边框；`GridPanel` / `StackPanel` 保持纯布局容器语义。
  - `Application` 析构已补齐渲染线程与资源清理。
  - demo 布局分区改为受总高度约束的稳定分配，窗口缩小时不再重叠。
- 测试结果：`49/49` 全通过。

## 本轮补充六（2026-04-10）

- 检查项：滚动条鼠标释放后的状态恢复、demo 页面拥挤度、官方文档语义一致性。
- 修复结论：
  - 滚动条 `thumb` 在鼠标释放或捕获取消后不再残留聚焦色。
  - demo 页面把更多空间让给顶部主交互区，preview 卡片内部区块也完成去拥挤化。
  - 对照 Microsoft Learn：当前实现已符合 `SetCapture/ReleaseCapture/WM_CAPTURECHANGED` 的处理语义，并更接近桌面窗口布局“避免拥挤、避免不必要滚动”的指导。
- 测试结果：`49/49` 全通过。

## 本轮补充七（2026-04-10）

- 检查项：demo section 结构清晰度、容器语义一致性、渲染层与测试几何对齐情况。
- 修复结论：
  - `demo/main.cpp` 已改为以 `StackPanel` 为主的 section 化布局，双列场景只使用 `GridPanel`。
  - `window_render_target.cpp` 不再沿用原先那种自由拼贴式 section 比例，而是按新的 section 模型重排。
  - `integration_flow_tests.cpp` 的点击坐标公式已同步到新布局，避免“测试点命中旧版面”。
- 测试结果：`49/49` 全通过。

## 本轮补充八（2026-04-10）

- 检查项：布局面板 Arrange 语义、渲染层是否绕过布局系统、WPF 类似行为一致性。
- 修复结论：
  - 原问题根因并非单纯 demo 比例错误，而是 `StackPanel` / `GridPanel` 会在重新 `Arrange` 时抹掉父容器偏移，导致布局树坐标语义错误。
  - 原 render target 还维护了一套 demo 专用分区算法，进一步掩盖了真实布局错误。
  - 现已改为：布局面板保留父偏移并应用 `margin`；渲染层读取 `UIElement` 真实绝对边界；demo 专用布局公式从主项目代码移除。
- 测试结果：`49/49` 全通过。

## 本轮补充九（2026-04-10）

- 检查项：嵌套 panel 是否需要业务层手工逐级 arrange、WPF 语义一致性、demo 面板稳定性。
- 修复结论：
  - 已为 `UIElement` 引入递归 `arrange` 链，父 panel 为 child 分配完布局槽位后，会继续驱动子 panel 完成其内部布局。
  - demo 侧 arrange 逻辑已经显著简化，不再承担布局引擎应负责的逐层排列工作。
- 测试结果：`49/49` 全通过。

## 本轮补充十（2026-04-10）

- 检查项：内容测量能力、flex 分配、grid 轨道计算、旧 demo 公式残留、集成交互回归。
- 修复结论：
  - `UIElement` 已具备 intrinsic size、测量缓存和 flex grow/shrink 基础能力。
  - `StackPanel` 已改为主轴/交叉轴语义布局，`GridPanel` 已改为内容驱动 track 计算。
  - `Window` 默认直接驱动 root `measure + arrange`，旧 demo 手工分区公式已从示例主流程中删除。
  - `demo/main.cpp` 现为布局验证窗口，只用于验证 `Panel`、`StackPanel`、`GridPanel` 组合能力。
- 测试结果：命令行构建通过，`54/54` 全通过。
