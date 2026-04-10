# API 概览

## 核心入口

- RenderManager
- Application / Window
- WindowHost
- WindowRenderTarget
- UIElement / LayoutManager
- AnimationManager
- InputManager

## 渲染

- RenderManager::initialize_with_backend(RenderBackend)
- RenderManager::resource_manager()
- RenderManager::diagnostics()
- CompositionBridge::bind_target_handle(HWND)
- CompositionBridge::commit_changes(bool)

## 控件

- Button / TextBlock / Image / Panel / Card
- TextBox / ListView / ScrollViewer / CheckBox / Slider
- Theme::make_dark/light/brand

## 绑定

- Observable<T>
- BindingContext
- Card::bind(BindingContext&)
- TextBox::bind_text(Observable<string>&)
- Button::bind_enabled(Observable<bool>&)

## 输入

- InputManager::focus_next()
- InputManager::on_mouse_down/on_mouse_move/on_mouse_up

## 配置与错误

- AppConfigLoader::load_from_file(path, config)
- Status / ErrorCode
