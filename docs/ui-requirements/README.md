# UI Requirements Baseline

本目录存放 DCompFrame UI 框架后续开发的强制需求基线。

## 文档清单

- `ui-framework-design-spec.md`：UI 框架总体设计文档（设计概要、技术要求、验收标准）。
- `ui-controls-design-spec.md`：控件设计文档（TextBox / ComboBox / Progress / Loading / TabControl / Grid / StackPanel / Panel / Label / Popup / Expander / ItemsControl）。

## 使用原则

- 后续 UI 开发、修复和测试必须以本目录文档为准。
- 实现与文档不一致时，优先修正文档后再改代码，避免“代码漂移”。
- 每次控件行为变更必须同步更新对应条目并补齐测试用例。

## 变更管理

- 每次需求更新在 `docs/dev-progress.md` 记录日期、范围和影响。
- 破坏性变更必须在 `docs/design.md` 中记录设计决策与迁移策略。
