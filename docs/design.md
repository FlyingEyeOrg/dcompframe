# 设计决策

## 1. 分层

采用轻量分层，先满足阶段 1~3 的核心职责：

- 渲染层：`RenderManager` + `CompositionBridge`
- 窗口层：`WindowHost`
- UI 核心层：`UIElement`
- 布局层：`GridPanel`、`StackPanel`

## 2. TDD 驱动实现

使用 Red-Green-Refactor 循环：

1. 先写测试定义行为边界。
2. 使用最小实现通过测试。
3. 在测试保护下修正构建和行为问题。

## 3. 关键行为约束

- `WindowHost` 默认包含 `WS_EX_NOREDIRECTIONBITMAP`。
- `CompositionBridge::commit_changes` 仅在以下条件满足时成功：
  - 渲染系统已初始化
  - 已绑定有效目标句柄
  - 本次存在脏更新
- 事件路由遵循捕获 -> 目标 -> 冒泡。
- 布局采用确定性计算，避免隐式状态。

## 4. 工程与依赖

- 构建：CMake（Preset + CTest）
- 包管理：vcpkg manifest
- 测试框架：GoogleTest
