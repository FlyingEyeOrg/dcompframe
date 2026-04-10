# Flex 布局引擎设计文档

## 1. 文档目的

本文档定义 DCompFrame 新版布局引擎的设计目标、术语映射、算法边界和实现约束。
设计依据来自以下 Web 官方资料：

- W3C CSS Flexible Box Layout Module Level 1
- WHATWG DOM Standard 的 tree / event dispatch 模型
- MDN 对 `event.target`、`event.currentTarget`、capture、bubble 的解释性文档

目标不是“做一个像 Flex 的桌面布局”，而是把项目后续的容器布局收敛到可验证、可复用、可测试的 Flexbox 语义。

## 2. 官方语义映射

### 2.1 Flex 容器

当前实现引入 `FlexPanel` 作为唯一推荐容器，对齐以下 Web 语义：

- `flex-direction`: `Row` / `RowReverse` / `Column` / `ColumnReverse`
- `flex-wrap`: `NoWrap` / `Wrap` / `WrapReverse`
- `justify-content`: `Start` / `End` / `Center` / `SpaceBetween` / `SpaceAround`
- `align-items`: `Start` / `End` / `Center` / `Stretch`
- `align-content`: `Start` / `End` / `Center` / `SpaceBetween` / `SpaceAround` / `Stretch`
- `row-gap` / `column-gap`

### 2.2 Flex 子项

`UIElement` 新增以下 Web 对齐字段：

- `flex_grow`
- `flex_shrink`
- `flex_basis`
- `order`
- `align_self`

其中：

- `flex_basis < 0` 表示 `auto`
- `align_self = Auto` 时回退到容器的 `align_items`
- `order` 仅影响布局排序，不改变所有权和树结构

## 3. 布局算法

### 3.1 Measure 阶段

`FlexPanel::measure()` 采用以下流程：

1. 读取子元素的 intrinsic size / `desired_size`。
2. 结合 `flex_basis` 生成每个 item 的 base main size。
3. 按 `order` 和声明顺序排序。
4. 根据 `flex-wrap` 和可用主轴空间拆分为 flex lines。
5. 汇总 line 的 main / cross 占用，生成容器测量结果。

这个阶段的职责是“确定内容想要多大”，不是最终像素落位。

### 3.2 Arrange 阶段

`FlexPanel::arrange()` 负责最终分配：

1. 重新计算 flex lines。
2. 根据剩余空间应用 `flex-grow`。
3. 在主轴不足时按 shrink weight 应用 `flex-shrink`。
4. 按 `justify-content` 分配主轴剩余空白。
5. 按 `align-items` / `align-self` 计算交叉轴对齐。
6. 在多行场景下按 `align-content` 计算 line 间空白。
7. 对每个 child 写入最终 bounds，并递归执行 child arrange。

### 3.3 当前实现边界

当前版本明确支持：

- 单行和多行 flex 布局
- grow / shrink / basis
- row / column 双主轴模型
- reverse 主轴方向
- wrap-reverse
- row-gap / column-gap
- capture / target / bubble 事件路由所需的稳定最终 bounds

当前版本暂不支持：

- CSS grid track 算法
- absolute / fixed / sticky 等定位模式
- baseline 对齐
- min-content / max-content 级别的完整 Web intrinsic sizing
- writing-mode / RTL 特殊流向

## 4. 元素树命中测试与事件路由

### 4.1 命中测试

实现对齐 Web tree hit-testing 的最小必要模型：

- 从 root 开始递归判断 `contains_point()`
- 子节点优先于父节点
- children 逆序遍历，保证后加入元素具备更高视觉命中优先级
- `hit_test_visible = false` 的节点及其分支不参与命中

### 4.2 事件流

输入事件先通过 `InputManager::hit_test()` 找到最深命中节点，再调用 `UIElement::dispatch_event()` 生成路径并按三段流转：

1. Capture: root 到 target parent
2. Target: target 自身
3. Bubble: target parent 回到 root

事件对象新增：

- `position`
- `target`
- `current_target`
- `phase`

这与 WHATWG DOM 里的 `target` / `currentTarget` / `eventPhase` 语义保持一致。

## 5. 兼容策略

### 5.1 StackPanel

`StackPanel` 保留为兼容适配层，但内部已改为基于 `FlexPanel` 工作：

- Vertical -> `flex-direction: column`
- Horizontal -> `flex-direction: row`
- `spacing` -> 同时映射 `row-gap` / `column-gap`
- 兼容层默认把 `align-content` 固定为 `Start`，以保持旧测试预期

### 5.2 GridPanel

`GridPanel` 仍保留用于历史测试和旧代码兼容，但从设计规范层面不再作为后续页面布局推荐方案。
新的界面布局必须优先使用嵌套 `FlexPanel` 表达二维结构。

## 6. Demo 设计

新版 demo 的布局树完全使用 `FlexPanel`：

- 页面根容器：column
- 顶部主内容：row
- 左侧表单列：column
- 表单选项行：row + wrap
- 右侧预览列：column
- 中部集合区：row
- 底部滚动区和日志区：page 的后续 flex items

同时 demo 为根容器、行容器、列容器和关键 leaf 控件绑定路由日志，点击任意元素都可观察 capture / target / bubble 顺序。

## 7. 验证策略

本轮实现用以下自动化验证收口：

- `FlexPanelTests.RowLayoutDistributesGrowBasisAndGap`
- `FlexPanelTests.WrapMovesOverflowItemsToNextLine`
- `UIElementTests.HitTestFindsDeepestVisibleDescendant`
- `InputManagerTests.HitTestRoutingDispatchesCaptureTargetAndBubble`
- 全量 `ctest --preset vs2022-x64-debug-tests`

当前结果：58/58 通过。

## 8. 后续演进方向

后续如果继续向 Web 语义逼近，优先级应为：

1. `min/max` 约束与更细粒度 intrinsic sizing
2. baseline 对齐
3. 更完整的 pointer / wheel / keyboard 事件对象
4. 事件取消与默认动作分离
5. 把 `GridPanel` 明确降为 legacy compatibility 层