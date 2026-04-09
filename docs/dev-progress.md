# 开发进度

## 已完成

- 完成 CMake 工程初始化，启用 C++20。
- 确认并使用 vcpkg 管理第三方依赖（fmt、gtest）。
- 采用 TDD 流程完成首批能力：
  - 红灯：先编写 10 个测试并执行失败。
  - 绿灯：实现 `RenderManager`、`WindowHost`、`UIElement`、`GridPanel`、`StackPanel`。
  - 回归：10/10 测试通过。
- 编写并验证 demo：`dcompframe_demo`。
- 修复构建问题：MSVC Runtime 与 `x64-windows-static` triplet 一致化。

## 当前状态

- 阶段 1（基础渲染与窗口框架）: 最小骨架已完成。
- 阶段 2（UI 核心与视觉树）: 核心行为已完成并测试覆盖。
- 阶段 3（布局面板）: `GridPanel` / `StackPanel` 基础能力已完成。

## 待办

- 接入真实 Direct3D11 / DirectComposition 初始化与提交流程。
- 增加 DPI/窗口状态的更细粒度行为测试。
- 增加控件层（Button/Panel/TextBlock/Image/Card）。
- 引入动画、资源管理与设备恢复模块。
