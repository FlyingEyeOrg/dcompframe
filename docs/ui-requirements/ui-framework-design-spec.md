# DCompFrame UI 框架设计文档（总规范）

## 1. 文档目的与范围

本文档定义 DCompFrame UI 框架的统一设计基线，覆盖：

- 设计概要
- 技术要求
- 验收标准
- 质量门禁

本规范用于约束后续所有 UI 控件和布局容器开发，避免交互不一致、渲染异常和输入链路缺失。

## 2. 参考标准（互联网公开标准）

- Microsoft WinUI Controls 指南：
  - https://learn.microsoft.com/en-us/windows/apps/develop/ui/controls/
- Microsoft Keyboard Interactions：
  - https://learn.microsoft.com/en-us/windows/apps/develop/input/keyboard-interactions
- Microsoft WPF Controls 概览（结构与职责参考）：
  - https://learn.microsoft.com/en-us/dotnet/desktop/wpf/controls/
- W3C WAI-ARIA APG（可访问性与键盘行为）：
  - Combobox: https://www.w3.org/WAI/ARIA/apg/patterns/combobox/
  - Tabs: https://www.w3.org/WAI/ARIA/apg/patterns/tabs/
  - Listbox: https://www.w3.org/WAI/ARIA/apg/patterns/listbox/
  - Slider: https://www.w3.org/WAI/ARIA/apg/patterns/slider/
  - Modal Dialog: https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/
- Material Design 3 Components（交互一致性与视觉行为参考）：
  - https://m3.material.io/components

## 3. 设计概要

### 3.1 架构目标

- 可组合：控件行为通过统一输入、布局、渲染协议组合。
- 可预测：鼠标、键盘、滚轮、焦点行为在所有控件中一致。
- 可测试：每个控件必须定义可自动化验证的输入-状态-输出链路。
- 可追踪：每个交互规则有标准出处，避免“拍脑袋实现”。

### 3.2 层次职责

- 输入层：命中测试、焦点管理、键盘/鼠标/滚轮路由。
- 布局层：`measure/arrange` 双阶段，保证容器裁剪和可见区域一致。
- 渲染层：只负责当前状态绘制，不承担业务逻辑和状态推断。
- 状态层：控件内部状态机（Normal/Hover/Pressed/Focused/Disabled/Expanded/Selected）。

### 3.3 框架关键行为（必须统一）

- 焦点可见性：可聚焦控件必须有可视焦点环。
- 滚动归属：滚轮事件只由最内层可滚动且命中的控件消费。
- 弹层层级：Popup/ComboBox 下拉层必须位于内容层之上，独立 z-order。
- 裁剪边界：父容器负责对子内容裁剪，禁止越界绘制。
- 输入优先级：文本输入控件优先处理文本编辑键，不被全局快捷键抢占。

## 4. 技术要求

### 4.1 输入系统要求

- 必须支持并验证：
  - `WM_CHAR`, `WM_KEYDOWN`, `WM_KEYUP`
  - `WM_MOUSEMOVE`, `WM_LBUTTONDOWN`, `WM_LBUTTONUP`
  - `WM_MOUSEWHEEL`
  - `WM_GETDLGCODE`（文本输入控件声明需要字符键）
- Tab 导航：
  - 交互控件默认可 tab 到，非交互控件默认不可 tab 到。
  - Tab 顺序与视觉阅读顺序一致（从上到下、从左到右）。
- 键盘语义：
  - Enter/Space 激活命令控件。
  - Esc 关闭临时 UI（下拉框、弹层、对话框）。
  - Home/End/PageUp/PageDown 语义与滚动区域一致。

### 4.2 布局系统要求

- 统一约定：
  - `measure(available_size)` 只计算期望尺寸。
  - `arrange(final_rect)` 才决定最终位置和可见区域。
- 容器控件（Grid/StackPanel/Panel/ItemsControl）必须保证：
  - 子元素排列后不被同层覆盖。
  - 子元素越界被父容器裁剪。
  - 可滚动容器可见区和内容区分离，不遮挡相邻区域。

### 4.3 渲染系统要求

- 光标、选区、滚动条、hover/pressed/focused 必须走统一样式变量。
- 文本绘制要求：
  - 单行输入控件与标签文本基线一致。
  - 光标宽度全局一致（建议固定为 1.0f 或 2.0f，不允许按控件随机变化）。
- 动画与重绘：
  - 滑块拖动必须增量更新，不允许在拖动回调中触发重入崩溃。

### 4.4 可访问性要求

- 每个可交互控件有可感知的名称（Label 或等价属性）。
- 键盘可达：仅键盘可完成核心业务流。
- 焦点不丢失：弹层打开与关闭时，焦点位置可预期。

### 4.5 稳定性与线程安全要求

- UI 状态更新只允许在 UI 线程进行。
- 渲染线程不得直接修改控件树结构。
- 拖拽和滚轮事件不得在迭代容器时销毁当前节点。

## 5. 缺陷映射与防回归要求

### 5.1 现有问题映射

- 文本框无法输入：输入消息链路与焦点声明不完整。
- 光标宽度不一致：文本控件光标样式未统一。
- 滑块滑动闪退：拖动期间状态更新与渲染路径存在重入或悬挂引用。
- ComboBox 下拉滚轮引发 ScrollViewer 滚动：滚轮事件冒泡边界不正确。
- ScrollViewer 遮挡 ItemsControl：布局分区与裁剪边界定义不清。

### 5.2 防回归技术要求

- 为每类问题定义至少 1 个自动化回归测试（单元或集成）。
- 每次修改输入/布局/滚动/弹层逻辑，必须跑全量 UI 测试。
- PR 合并门禁：
  - 新增功能必须附带验收项对应测试。
  - 修复 BUG 必须附带复现与回归测试。

## 6. 验收标准（全局）

### 6.1 功能验收

- 文本输入、键盘导航、鼠标交互、滚轮滚动、弹层关闭行为全部可用。
- 控件状态在 hover/press/focus/disable 之间切换无异常。

### 6.2 视觉验收

- 文本与光标不抖动、不漂移。
- 下拉层和弹层不被内容层遮挡。
- ScrollViewer 与 ItemsControl 可见区域无互相覆盖。

### 6.3 稳定性验收

- 拖动 Slider 连续 10,000 次不崩溃。
- ComboBox 弹层打开状态连续滚轮 2,000 次无错误滚动穿透。
- 连续创建/销毁弹层 5,000 次无资源泄漏或悬挂句柄。

### 6.4 可访问性验收

- 全流程支持仅键盘操作。
- Esc 可关闭临时弹层并恢复焦点到触发控件。
- Tab 顺序与视觉顺序一致，且无焦点陷阱（Modal 除外）。

## 7. 测试与质量门禁

### 7.1 测试分层

- 单元测试：控件状态机、键盘语义、滚动边界。
- 集成测试：布局容器与控件组合行为。
- 可靠性测试：高频输入事件压测（拖动、滚轮、切换焦点）。

### 7.2 强制门禁

- 编译通过 + 全量测试通过。
- 新增/修复代码必须覆盖对应验收条款。
- 未满足验收标准不得标记“完成”。

## 8. 后续执行约束

- 本文档与 `ui-controls-design-spec.md` 是后续 UI 开发强制依据。
- 功能开发前先对照验收条款写测试（TDD）。
- 需求变更需先更新文档，再改代码。
