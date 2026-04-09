# 技术栈文档

## 目标

本技术栈文档描述基于 `Direct3D11 + DirectComposition + WS_EX_NOREDIRECTIONBITMAP` 的 C++ UI 库架构方案，适合构建高性能、GPU 加速的 Windows 桌面应用 UI 框架。

## 方案概述

- `Direct3D11`：负责 GPU 渲染与纹理管理。
- `DirectComposition`：负责视觉树管理、层合成、动画与硬件加速最终输出。
- `WS_EX_NOREDIRECTIONBITMAP`：让顶层窗口绕过 DWM 的位图重定向，适用于直接呈现 GPU 内容的场景。

## 技术栈组成

### 1. Direct3D 11

- 创建 `ID3D11Device` 和 `ID3D11DeviceContext`
- 管理 GPU 资源：纹理、缓冲区、着色器、渲染目标
- 可与 `Direct2D`、`DirectWrite` 集成，支持矢量绘制与高质量文本
- 适用于离屏渲染、复杂特效、图形 UI 元素

### 2. DirectComposition

- 创建 `IDCompositionDevice`
- 使用 `IDCompositionTarget` 绑定到窗口 `HWND`
- 构造 `IDCompositionVisual` 树，表示 UI 元素层次和复合关系
- 支持透明度、变换、裁剪、动画、效果链
- 通过 `SetContent` 绑定 `ID3D11Texture2D` 或 `IDXGISurface`

### 3. WS_EX_NOREDIRECTIONBITMAP

- 作用：禁用 DWM 对顶层窗口的重定向位图处理
- 适用目标：需要直接显示 GPU 渲染内容，减少额外拷贝和延迟
- 适用平台：Windows 8.1 及以上
- 兼容性限制：不支持 Windows 7，不应与传统复杂子窗口重定向混用

## 架构建议

### 根窗口层

- 使用顶层 `HWND` 作根窗口
- 创建窗口时设置扩展样式：`WS_EX_NOREDIRECTIONBITMAP`
- 通过 `IDCompositionTarget::SetRoot` 将根视觉对象与窗口绑定

### 渲染管线

- 建议创建一个共享的 `ID3D11Device`，可复用上下文和资源
- 使用 `IDXGISwapChain1` 或 `ID3D11Texture2D` 作为 DirectComposition 内容源
- 通过 `IDCompositionVisual::SetContent` 将渲染纹理设置到视觉树中的某个节点
- 渲染完成后调用 `Commit` 将变更提交给 DirectComposition

### UI 层次与视觉树

- 将 UI 控件映射为 `Visual` 节点
- 每个控件可拥有独立内容纹理、透明度和变换属性
- 通过 DirectComposition 实现层次组合、嵌套布局和硬件加速动画

### 绘制与文本

- 推荐将 2D 绘制任务交给 `Direct2D` 与 `DirectWrite`
- 在 `ID3D11Texture2D` 上创建 `ID2D1DeviceContext`，实现高质量矢量绘制
- 对于复杂 UI 元素，可先离屏绘制到纹理，再提交给 DirectComposition

## 优势

- 高性能 GPU 加速
- 强大的视觉层合成与动画能力
- 适合现代桌面 UI 中的透明、变换、特效需求
- 可通过 DirectComposition 实现无缝更新与低延迟渲染

## 风险与限制

- 平台限制：仅支持 `Windows 8.1` 及以上
- DWM 交互：`WS_EX_NOREDIRECTIONBITMAP` 可能影响截图、窗口缩略图和桌面窗口管理器效果
- 窗口层级：不适合需要大量嵌套子窗口与传统窗口消息/绘制整合的方案
- 资源管理：需要谨慎管理 GPU 资源与纹理同步，避免内存泄露和性能抖动

## 适用场景

- 专注于硬件加速的自定义 UI 框架
- 需要高性能、平滑动画、复杂视觉效果的桌面应用
- 对文本显示、矢量绘制、透明合成有高要求的场景
- 需要直接渲染到窗口并最小化 DWM 重定向开销的窗口

## 不适用场景

- 需要兼容 Windows 7 的产品
- 依赖传统 Win32 窗口子级绘制和 GDI 复合的项目
- 需要窗口内容由 DWM 缩略图或屏幕录制无缝支持的场景

## 结论

`Direct3D11 + DirectComposition + WS_EX_NOREDIRECTIONBITMAP` 是一套可行的技术栈，用于构建高性能的 C++ UI 框架。该方案适合 Windows 8.1 及以上，需重点处理 GPU 资源管理、窗口兼容性与 DWM 行为差异。
