# DCompFrame

DCompFrame 是一个基于 C++20 + CMake + vcpkg 的 Windows UI 框架原型，面向
`Direct3D11 + DirectComposition + WS_EX_NOREDIRECTIONBITMAP` 技术路线。

## 当前实现范围（完整工程版）

- 渲染层：
	- `RenderManager`（`Simulated` / `DirectX`，`DirectX12` 预留）
	- `CompositionBridge`
	- `ResourceManager`、`DeviceRecovery`、`DiagnosticsCenter`
- 窗口层：
	- `WindowHost`（创建/销毁、消息循环、状态管理、DPI、`WS_EX_NOREDIRECTIONBITMAP`）
	- 自定义 DWM 标题栏：caption 区绘制、caption 按钮命中、窗口拖拽、resize hit-test、Per-Monitor V2 DPI
	- 多窗口安全退出：仅最后一个窗口销毁时退出消息循环
	- 鼠标消息驱动重绘请求，支持实时 hover/click 视觉反馈
	- `WindowRenderTarget`（DirectX 后端执行真实清屏 + D2D/DirectWrite 内容绘制 + Present + DComp Commit）
	- `Application` / `Window` 提供生命周期与窗口宿主骨架（`include/dcompframe/application.h`、`src/application.cpp`），demo 通过 `WindowFactory` 在 `demo/main.cpp` 中实现具体控件树与事件接线。
	- D2D 初始化/绘制失败时采用 Fail-Fast 返回错误（开发阶段禁用自动兜底）
	- demo overlay 支持按钮交互、caption 按钮和 List 区域逐项 hover 高亮
	- demo 表单区按 Element Plus 风格重绘，内容按纵向分区堆叠；`ComboBox` 使用顶层 overlay 下拉，`ScrollViewer`、`ListView`、`ItemsControl` 展示真实滚动内容并仅绘制可见项，支持可拖拽 thumb 与轨道点击跳转
	- 滚动条 thumb 在鼠标释放、捕获取消后会恢复普通态，不再残留激活色
	- 连续渲染节流：仅在 `Loading active` 或 `Progress indeterminate` 时持续重绘，降低空转卡顿。
	- demo 分区布局改为“在可用高度内分配后再绘制”，避免窗口缩小时出现区域重叠
	- demo 进一步重排为更宽松的顶部主交互区和更克制的底部滚动区，避免内容拥挤
	- demo 当前已切换为纯 `FlexPanel` 页面树：页面、表单列、预览列、集合区和底部区全部按 Web Flexbox 语义组织
	- 布局引擎已修正为保留父容器安排结果并应用 `margin`，render target 直接读取真实控件边界进行绘制与命中
	- 布局容器现支持递归 Arrange，嵌套 panel 在一次 root arrange 后即可完成内部排版，设计思路参考 WPF 的布局链
	- 布局系统现已升级为 `measure + arrange` 双阶段，`UIElement` 支持 intrinsic size、`flex grow/shrink`，`FlexPanel` 负责主轴空间分配；`StackPanel` 与 `GridPanel` 仅保留兼容层职责
	- 新增 `FlexPanel`，支持 `flex-direction`、`wrap`、`justify-content`、`align-items`、`align-content`、`gap`、`flex-basis`、`order`
	- 元素树命中测试改为先命中最深节点，再按 capture -> target -> bubble 路由到祖先链
	- 旧 demo 公式布局已移除，当前 `demo/main.cpp` 是专门用于验证 `FlexPanel` 布局和元素树命中测试路由的示例窗口
	- 右侧示例区改为真实独立控件边界，不再把 `TabControl`、`Expander`、`Progress`、`Popup` 塞进单张 card 的派生布局里
- UI 核心：
	- `UIElement`（视觉树、事件捕获/目标/冒泡、脏标记、焦点、命中测试、变换、裁剪）
	- `LayoutManager`（Flex-only）
- 布局层：`FlexPanel`
- 控件层：`Panel`、`TextBlock`、`Image`、`Button`、`Card`
- 扩展控件：`TextBox`、`RichTextBox`、`ListView`、`ItemsControl`、`ScrollViewer`、`CheckBox`、`ToggleSwitch`、`ComboBox`、`RadioGroup`、`Slider`、`Badge`、`Divider`、`Label`、`Progress`、`Loading`、`TabControl`、`Popup`、`Expander`
- 文本输入：`TextBox` 与 `RichTextBox` 均支持编辑、选区、光标移动、回车/退格/删除与常用剪贴板快捷键；`RichTextBox` 额外支持内部滚动与上下行导航
- 视觉基线：默认白色背景，字体族使用 `Microsoft YaHei UI`，控件字号对齐 Element Plus 14px 体系
- 列表默认行为：`ListView` 与 `ItemsControl` 内容溢出时默认内部滚动，其中 `ItemsControl` 默认以垂直堆叠布局呈现
- 文本对齐：除 `RichTextBox` 外默认水平+垂直居中
- 默认中文字体：`Microsoft YaHei`
- Panel 布局：提供 `arrange`，默认将子元素拉伸填满可用区域，且默认背景/边框透明（WPF 风格容器语义）
- 主题样式：`Theme` / `Style`
- 动画层：`AnimationManager`（属性动画 + 缓动）
- 输入系统：`InputManager`（焦点、双击、拖拽、快捷键命令路由）
- 绑定与配置：`Observable`、`BindingContext`、`AppConfigLoader(JSON)`
- 错误与状态：`Status` / `ErrorCode`
- 测试：57 个测试（x64 Debug 全部通过）
- 示例：`dcompframe_demo`（已覆盖完整模块链路，窗口持续运行直到关闭）
- 已修复 `WS_EX_NOREDIRECTIONBITMAP` 场景下的透明窗口问题。

## 开发阶段原则

- 当前阶段采用 Fail-Fast：不保留自动回退、静默降级与兜底渲染路径。
- 后端选择需显式指定（`DirectX` 或 `Simulated`），框架不做隐式切换。
- demo 启动场景不再进行运行时隐式降级；初始化失败直接返回错误。

## VS Code 任务与调试

- 构建任务：已覆盖 x64/x86、Debug/Release 的 configure/build/test/demo。
- 调试任务：已提供 demo/tests 的调试启动配置。
- 配置文件：`.vscode/tasks.json`、`.vscode/launch.json`
- CI 文件：`.github/workflows/ci.yml`
- 打包：CPack ZIP（任务：`Package x64 Release (ZIP)`）
- DWM 兼容策略：`docs/dwm-compatibility.md`

## UI 需求基线文档

- UI 框架总规范：`docs/ui-requirements/ui-framework-design-spec.md`
- UI 控件规范：`docs/ui-requirements/ui-controls-design-spec.md`
- 文档入口：`docs/ui-requirements/README.md`

后续 UI 功能开发、缺陷修复和测试编写均以以上文档为强制基线。

## 框架设计文档

- 框架设计与原理说明：`docs/framework-design.md`
- Flex 布局设计：`docs/flex-layout-engine-design.md`
- Flexbox Only 规范：`docs/flexbox-only-spec.md`

## 依赖管理

依赖通过 vcpkg manifest 管理（见 `vcpkg.json`）：

- `fmt`
- `gtest`

## 构建

1. 配置（VS2022 x64）

```powershell
cmake --preset vs2022-x64
```

2. 构建测试

```powershell
cmake --build --preset vs2022-x64-debug --target dcompframe_tests
```

3. 运行测试

```powershell
ctest --preset vs2022-x64-debug-tests
```

4. 构建并运行 demo

```powershell
cmake --build --preset vs2022-x64-debug --target dcompframe_demo
.\build\vs2022-x64\Debug\dcompframe_demo.exe
```

## 目录结构

```text
include/dcompframe/      # 公共头文件
src/                     # 实现
tests/                   # 单元测试
demo/                    # 示例程序
docs/                    # 项目文档
```

## 后续规划

- 扩展输入系统（键盘命令、IME、组合交互）
- 增加更多高阶控件与虚拟化能力
- 增强设备丢失恢复与性能压测
- 细化 DWM 兼容与窗口 frame 行为矩阵
