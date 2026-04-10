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
	- 多窗口安全退出：仅最后一个窗口销毁时退出消息循环
	- 鼠标消息驱动重绘请求，支持实时 hover/click 视觉反馈
	- `WindowRenderTarget`（DirectX 后端执行真实清屏 + D2D/DirectWrite 内容绘制 + Present + DComp Commit）
	- D2D 初始化/绘制失败时采用 Fail-Fast 返回错误（开发阶段禁用自动兜底）
	- demo overlay 支持按钮交互和 List 区域逐项 hover 高亮
	- demo 表单区按 Element Plus 风格重绘，`ComboBox` 使用 overlay 下拉，`ScrollViewer` 展示真实滚动内容并仅绘制可见项
- UI 核心：
	- `UIElement`（视觉树、事件捕获/目标/冒泡、脏标记、焦点、变换、裁剪）
	- `LayoutManager`（策略仅保留 `Grid/Stack`）
- 布局层：`GridPanel`、`StackPanel`
- 控件层：`Panel`、`TextBlock`、`Image`、`Button`、`Card`
- 扩展控件：`TextBox`、`RichTextBox`、`ListView`、`ItemsControl`、`ScrollViewer`、`CheckBox`、`ComboBox`、`Slider`
- 文本对齐：除 `RichTextBox` 外默认水平+垂直居中
- Panel 布局：提供 `arrange`，默认将子元素拉伸填满可用区域（WPF 风格容器语义）
- 主题样式：`Theme` / `Style`
- 动画层：`AnimationManager`（属性动画 + 缓动）
- 输入系统：`InputManager`（焦点、双击、拖拽、快捷键命令路由）
- 绑定与配置：`Observable`、`BindingContext`、`AppConfigLoader(JSON)`
- 错误与状态：`Status` / `ErrorCode`
- 测试：43 个测试（x64 Debug 全部通过）
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
- 增加数据绑定与更多控件（ListView/TextBox）
- 增强设备丢失恢复与性能压测
- 细化 DWM 兼容与后端回退矩阵
