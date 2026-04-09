# DCompFrame

DCompFrame 是一个基于 C++20 + CMake + vcpkg 的 Windows UI 框架原型，面向
`Direct3D11 + DirectComposition + WS_EX_NOREDIRECTIONBITMAP` 技术路线。

## 当前实现范围（完整工程版）

- 渲染层：
	- `RenderManager`（`Simulated` / `DirectX` / `Warp`，`DirectX12` 预留）
	- `CompositionBridge`
	- `ResourceManager`、`DeviceRecovery`、`DiagnosticsCenter`
- 窗口层：
	- `WindowHost`（创建/销毁、消息循环、状态管理、DPI、`WS_EX_NOREDIRECTIONBITMAP`）
	- `WindowRenderTarget`（DirectX 后端执行真实清屏 + Present + DComp Commit）
- UI 核心：
	- `UIElement`（视觉树、事件捕获/目标/冒泡、脏标记、焦点、变换、裁剪）
	- `LayoutManager`
- 布局层：`GridPanel`、`StackPanel`
- 控件层：`Panel`、`TextBlock`、`Image`、`Button`、`Card`
- 扩展控件：`TextBox`、`ListView`、`ScrollViewer`、`CheckBox`、`Slider`
- 主题样式：`Theme` / `Style`
- 动画层：`AnimationManager`（属性动画 + 缓动）
- 输入系统：`InputManager`（焦点、双击、拖拽、快捷键命令路由）
- 绑定与配置：`Observable`、`BindingContext`、`AppConfigLoader(JSON)`
- 错误与状态：`Status` / `ErrorCode`
- 测试：33 个测试（x64/x86 Debug 全部通过）
- 示例：`dcompframe_demo`（已覆盖完整模块链路，窗口持续运行直到关闭）

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

- 接入真实 D2D/DirectWrite 到 D3D11 纹理绘制路径
- 扩展输入系统（键盘命令、IME、组合交互）
- 增加数据绑定与更多控件（ListView/TextBox）
- 增强设备丢失恢复与性能压测
- 细化 DWM 兼容与后端回退矩阵
