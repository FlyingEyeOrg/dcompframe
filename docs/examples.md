# 示例

## 1. 加载配置并初始化

```cpp
AppConfig config;
auto status = AppConfigLoader::load_from_file("demo/demo-config.json", config);
RenderManager manager;
manager.initialize_with_backend(config.use_directx_backend ? RenderBackend::DirectX : RenderBackend::Simulated);
```

## 2. 绑定 Card 与 TextBox

```cpp
BindingContext context;
Card card;
TextBox box;
card.bind(context);
box.bind_text(context.title);
context.title.set("Hello");
```

## 3. 输入与焦点

```cpp
InputManager input;
input.set_focus_ring_root(root);
input.focus_next();
input.on_mouse_down(button, {10, 20});
```

## 4. 导出诊断报告

```cpp
manager.diagnostics().export_report("build/diagnostics-report.json");
```
