# Changelog

## 0.2.0 - 2026-04-09

### Added
- 产品化扩展：InputManager、Binding、AppConfig JSON、新增控件族（TextBox/ListView/ScrollViewer/CheckBox/Slider）。
- 诊断增强：P95、提交频率、峰值资源统计、JSON 报告导出。
- 集成测试与产品特性测试。
- VS Code 构建/调试任务矩阵和 launch 配置。
- CPack ZIP 发布配置。

### Changed
- demo 升级为完整功能展示，并支持关闭窗口后退出。
- WindowHost 消息循环从一次性循环改为阻塞循环。

### Fixed
- 修复窗口“闪退退出”问题。
- 修复 ResourceManager 注册键顺序问题。
