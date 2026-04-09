# DCompFrame

DCompFrame 是一个基于 C++20 + CMake + vcpkg 的 Windows UI 框架原型，面向
`Direct3D11 + DirectComposition + WS_EX_NOREDIRECTIONBITMAP` 技术路线。

## 当前实现范围（阶段 1/2 最小可用）

- 渲染层骨架：`RenderManager`、`CompositionBridge`
- 窗口层骨架：`WindowHost`（默认包含 `WS_EX_NOREDIRECTIONBITMAP`）
- UI 核心层：`UIElement` 视觉树与捕获/目标/冒泡事件分发
- 布局面板：`GridPanel`、`StackPanel`
- 测试：10 个单元测试（全部通过）
- 示例：`dcompframe_demo`

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

- 接入真实 D3D11 / DirectComposition 设备与目标
- 扩展输入系统（键鼠焦点细化）
- 引入控件层（Button/TextBlock/Card）
- 引入动画与资源恢复机制
