# 框架设计说明

## 目标

DCompFrame 的目标不是把 demo 逻辑封进框架核心，而是提供一套可复用的 Windows UI 基础设施：

- 应用程序生命周期管控
- 窗口宿主与消息循环
- Direct3D11 + DirectComposition 呈现链路
- 基础视觉树与布局容器
- 可扩展控件模型与交互渲染目标

demo 只是使用这套骨架能力的示例层。

## 分层

### 1. Application

`Application` 只负责应用级生命周期：

- 加载配置
- 初始化 `RenderManager`
- 启停渲染线程
- 维护窗口集合
- 驱动消息循环与连续渲染节流
- 导出诊断报告

它不负责创建具体控件，也不负责 demo 布局。

### 2. Window

`Window` 是窗口骨架，职责保持在宿主层：

- 创建 `WindowHost`
- 绑定 `WindowRenderTarget`
- 暴露 `build()` 扩展点供上层实现内容
- 暴露 `set_root`、`set_arrange_handler`、`set_interactive_controls` 等接线能力

这意味着具体窗口里有哪些控件、怎么绑定事件、怎么安排布局，都应由业务层或 demo 层决定。

### 3. WindowHost

`WindowHost` 负责 Win32 窗口原语：

- 创建/销毁原生窗口
- DPI 与窗口状态管理
- 消息分发
- 请求重绘

### 4. WindowRenderTarget

`WindowRenderTarget` 负责渲染与交互表现：

- DX11 swapchain 与 DComp target 初始化
- D2D/DirectWrite overlay 绘制
- 命中测试
- 滚动条、页签、下拉框等交互反馈

它不拥有业务控件，只消费外部传入的 `InteractiveControls`。

### 5. UIElement / 布局容器

基础视觉树由 `UIElement` 提供。

- `GridPanel`
- `StackPanel`
- `Panel`

其中：

- `GridPanel` 和 `StackPanel` 只负责排布，不承担背景绘制职责，因此天然是透明布局容器。
- `Panel` 作为可样式化容器，默认背景透明、边框透明、边框厚度为 0。

## 设计原则

### 骨架与示例分离

框架核心只保留稳定的生命周期与宿主接口，示例内容全部放在 demo 中实现。这样做有三个直接收益：

- 框架 API 更清晰
- demo 改动不会污染主库边界
- 后续接入真实业务窗口时，不需要先删 demo 逻辑

### 布局容器默认透明

布局容器不承担视觉装饰职责，默认保持透明，以免影响业务控件统一的视觉语言。

### demo 区块不重叠

demo 渲染区不再依赖“固定最小高度 + 继续累加”的方式，而是先在总可用高度内完成分配，再按优先级压缩非关键区块。这样在窗口收缩时，各区域只会缩小，不会互相侵占。

## demo 如何使用框架

当前 demo 的组织方式：

1. `main()` 创建 `Application`
2. demo 注册 `WindowFactory`
3. `WindowFactory` 生成 `DemoWindow`
4. `DemoWindow::build()` 中完成控件创建、事件绑定、根布局与渲染目标接线
5. `Application::create_window()` 创建真正的示例窗口

这套结构证明了框架与示例内容已经完成职责分离。

## 原理摘要

- 生命周期由 `Application` 统一收口
- 单窗口宿主由 `Window` + `WindowHost` 组成
- 呈现与交互由 `WindowRenderTarget` 处理
- 控件状态由 demo 提供，渲染目标只做消费与反馈
- 布局容器保持透明和无装饰，视觉统一下沉到具体控件

## 当前验证结果

- x64 Debug 构建通过
- demo 可运行
- `ctest --preset vs2022-x64-debug-tests` 结果 `49/49` 通过