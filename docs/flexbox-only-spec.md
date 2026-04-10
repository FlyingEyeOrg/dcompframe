# Flexbox Only 开发规范

## 1. 适用范围

从本规范生效起，DCompFrame 后续新增页面布局、示例布局和业务布局，统一只允许使用 Web Flexbox 模式表达。

适用对象：

- demo 页面
- 新增窗口页面
- 业务容器布局
- 命中测试依赖的元素树结构
- 新增布局相关测试和文档

## 2. 强制要求

### 2.1 容器层

后续新增容器必须优先使用 `FlexPanel`。

允许使用的布局能力：

- `flex-direction`
- `flex-wrap`
- `justify-content`
- `align-items`
- `align-content`
- `align-self`
- `flex-grow`
- `flex-shrink`
- `flex-basis`
- `order`
- `row-gap`
- `column-gap`

### 2.2 二维布局表达

如果需要二维页面，不允许新增 grid 语义容器。

必须使用：

- row 容器嵌套 column 容器
- column 容器嵌套 row 容器
- wrap + basis + grow 组合

换句话说，二维结构通过嵌套 flex 表达，而不是再引入新的轨道布局模型。

## 3. 禁止事项

后续开发中禁止：

- 新增 `GridPanel` 使用场景
- 新增基于固定 section 比例的 demo 几何公式
- 在 render target 中维护独立于元素树的布局坐标系统
- 通过硬编码像素块“拼页面”来绕过布局引擎
- 引入新的 Absolute 布局策略作为主页面组织方式

## 4. 兼容例外

以下内容允许暂时保留，但不得扩散：

- 现有 `GridPanel` 历史代码
- 现有 `GridPanel` 测试
- 基于 `StackPanel` 的兼容适配 API

这些能力仅视为兼容层，不得再作为新功能默认方案。

## 5. 命中测试与事件流要求

所有新交互页面必须满足：

1. 命中测试依据元素树最终 bounds。
2. 事件路由必须经过 `capture -> target -> bubble`。
3. 不允许直接在窗口消息层绕过元素树对业务控件分发点击。
4. 新增控件如需参与命中测试，必须挂入元素树。

## 6. 文档与测试要求

每次新增布局功能时必须同步：

- 更新设计文档中的 Flex 语义章节
- 更新 README 中的布局说明
- 补充至少一条布局测试
- 补充至少一条命中测试或事件路由测试（若涉及交互）

## 7. Code Review 门禁

涉及布局相关变更时，评审必须检查：

1. 是否使用 `FlexPanel` 或兼容层 `StackPanel`。
2. 是否试图把坐标计算塞回 render target。
3. 是否新增了不可测试的硬编码位置公式。
4. 是否存在新的 grid-only 设计依赖。
5. 是否补了布局和事件测试。

未满足以上条件的变更不得合入。

## 8. 推荐写法

推荐模式：

- 页面根：column flex
- 主内容区：row flex
- 表单列：column flex
- 卡片集合：row flex + wrap
- 底部工具区：column 或 row flex

推荐原则：

- 先表达结构，再表达尺寸
- 优先使用 grow / basis，而不是固定宽高
- gap 代替手写兄弟间距公式
- 用嵌套 flex 表达分区，不用独立 overlay 数学切块

## 9. 生效说明

本规范自 2026-04-10 起生效。

本规范与 `docs/flex-layout-engine-design.md` 一起构成后续布局开发的强制基线：

- 设计说明看 `flex-layout-engine-design.md`
- 执行约束看本文件