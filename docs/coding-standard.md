# C++ 开发规范

## 目的

提供本项目一致的 C++ 编码、构建与提交流程，提升可维护性、可读性与安全性。

## 适用范围

适用于本仓库中所有 C++ 源代码（使用 C++20 标准）。

## 代码风格

- 语言标准：使用 `C++20`。
- 缩进：4 个空格，不使用 Tab。
- 行宽：尽量 <= 100 字符。
- 命名约定：
  - 类型/类：`PascalCase`（例如 `HttpHandler`）。
  - 变量/函数：`snake_case` 或 `camelCase`（团队统一风格：`snake_case`）。
  - 常量：全大写并用下划线分隔 `MAX_BUFFER_SIZE`。
  - 私有成员以后缀 `_` 或前缀 `m_`（团队约定：使用后缀 `_`）。
- 大括号风格：K&R（函数与控制语句的起始大括号同行）。

## 现代 C++ 实践

- 优先使用值语义与 RAII 管理资源。
- 避免裸指针作为所有权类型；使用 `std::unique_ptr`、`std::shared_ptr` 或 `std::weak_ptr`。
- 使用 `constexpr`、`consteval` 与 `const` 以表达不变性。
- 避免宏定义影响可见性；使用 `inline` 变量、`constexpr` 或模板代替宏。

## 错误处理

- 优先使用异常处理错误路径；对性能关键路径可使用返回错误码/`std::expected`（若引入）。
- 捕获异常时仅捕获能处理的类型，使用最小作用域的 try/catch。

## 头文件管理

- 头文件使用 include-guard 或 `#pragma once`（推荐 `#pragma once`）。
- 在头文件中尽量避免包含不必要的其他头，使用前向声明以缩短编译时间。

## 构建与依赖

- 使用 CMake 构建：遵循现代 CMake（`target_*`）实践。
- 每个库/可执行文件使用独立 `target` 并显式设置 `target_include_directories`、`target_compile_features` 和 `target_link_libraries`。

## 测试与 CI

- 所有新功能必须配套单元测试，放在 `tests/` 目录。
- CI（GitHub Actions / 内部 CI）必须在合并前运行构建与测试流。

## 格式化与静态分析

- 使用 `clang-format`（团队统一配置文件放在仓库根目录），推送前运行格式化。
- 引入静态分析工具（如 clang-tidy）并在 CI 中启用关键检查。

## 日志与错误信息

- 日志应包含足够的上下文（模块、函数、请求 id），避免记录敏感信息。

## 安全性

- 对外输入进行严格校验，避免整数溢出、越界与未定义行为。
- 使用安全的字符串/缓冲区操作，优先使用标准库容器。

## 代码审查与提交规范

- 小步提交，清晰的提交信息，遵循 `<范围>: <描述>` 格式（例如 `net: fix packet parsing`）。
- 合并前必须通过代码审查（至少一名审查者）。

## 分支策略

- 使用 `main`（稳定）、`develop`（开发）与 feature 分支工作流；合并使用 Pull Request。

## 参考工具链

- 编译器：MSVC / Clang / GCC（按平台）
- 包管理：`vcpkg`（仓库已配置）

---

文档维护：如需调整规范，提交 PR 并在 `docs/dev-progress.md` 中记录变更。
