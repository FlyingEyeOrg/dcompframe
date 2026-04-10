cmake_minimum_required(VERSION 3.21)
project(DCompFrame VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(dcompframe
    src/render_manager.cpp
    src/window_host.cpp
    src/ui_element.cpp
    src/layout/grid_panel.cpp
    src/layout/stack_panel.cpp
    src/controls/style.cpp
    src/controls/controls.cpp
    src/animation/animation_manager.cpp
    src/tools/window_render_target.cpp
    src/input/input_manager.cpp
    src/binding/observable.cpp
    src/config/app_config.cpp
)

target_include_directories(dcompframe
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(dcompframe PUBLIC cxx_std_20)
target_link_libraries(dcompframe PUBLIC fmt::fmt nlohmann_json::nlohmann_json)
target_compile_definitions(dcompframe PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
if(MSVC)
    target_compile_options(dcompframe PRIVATE /FS)
    target_compile_options(dcompframe PRIVATE $<$<CONFIG:Debug>:/Z7> $<$<CONFIG:RelWithDebInfo>:/Z7>)
endif()

if(WIN32)
    target_link_libraries(dcompframe PUBLIC d3d11 dxgi dcomp user32 gdi32)
endif()

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)

    add_executable(dcompframe_tests
        tests/render_and_window_tests.cpp
        tests/ui_and_layout_tests.cpp
        tests/controls_and_animation_tests.cpp
        tests/tools_and_layout_manager_tests.cpp
        tests/product_features_tests.cpp
        tests/integration_flow_tests.cpp
        tests/reliability_tests.cpp
    )

    target_compile_features(dcompframe_tests PRIVATE cxx_std_20)
    target_link_libraries(dcompframe_tests PRIVATE dcompframe GTest::gtest_main)
    if(MSVC)
        target_compile_options(dcompframe_tests PRIVATE /FS)
        target_compile_options(dcompframe_tests PRIVATE $<$<CONFIG:Debug>:/Z7> $<$<CONFIG:RelWithDebInfo>:/Z7>)
    endif()

    include(GoogleTest)
    gtest_discover_tests(dcompframe_tests)
endif()

add_executable(dcompframe_demo demo/main.cpp)
target_compile_features(dcompframe_demo PRIVATE cxx_std_20)
target_link_libraries(dcompframe_demo PRIVATE dcompframe fmt::fmt)
if(MSVC)
    target_compile_options(dcompframe_demo PRIVATE /FS)
    target_compile_options(dcompframe_demo PRIVATE $<$<CONFIG:Debug>:/Z7> $<$<CONFIG:RelWithDebInfo>:/Z7>)
endif()

install(TARGETS dcompframe dcompframe_demo
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "DCompFrame")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
