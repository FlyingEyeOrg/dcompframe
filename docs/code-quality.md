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
- 发现：按钮文本虽然使用居中格式，但绘制矩形与按钮本体不一致，视觉上未居中。
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
