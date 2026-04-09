# 兼容性矩阵

| 维度 | 状态 | 说明 |
| --- | --- | --- |
| Windows 8.1 | 支持（目标） | DirectComposition 最低版本要求 |
| Windows 10 | 支持 | 推荐开发与测试环境 |
| Windows 11 | 支持 | 推荐开发与测试环境 |
| x64 Debug | 已验证 | CTest 全绿 |
| x64 Release | 已构建 | CI 覆盖 |
| x86 Debug | 已构建/测试 | CI 覆盖 |
| x86 Release | 已构建 | CI 覆盖 |
| 高 DPI | 已覆盖基础逻辑 | 需补充更多视觉回归测试 |
| DWM 回退策略 | 已覆盖 | 详见 `docs/dwm-compatibility.md` |
| 设备丢失恢复压测 | 已验证 | 500 次循环恢复通过 |
