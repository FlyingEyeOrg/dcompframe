cmake_minimum_required(VERSION 3.21)
project(DCompFrame VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(fmt CONFIG REQUIRED)

add_library(dcompframe
    src/render_manager.cpp
    src/window_host.cpp
    src/ui_element.cpp
    src/layout/grid_panel.cpp
    src/layout/stack_panel.cpp
)

target_include_directories(dcompframe
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(dcompframe PUBLIC cxx_std_20)
target_link_libraries(dcompframe PUBLIC fmt::fmt)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)

    add_executable(dcompframe_tests
        tests/render_and_window_tests.cpp
        tests/ui_and_layout_tests.cpp
    )

    target_compile_features(dcompframe_tests PRIVATE cxx_std_20)
    target_link_libraries(dcompframe_tests PRIVATE dcompframe GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(dcompframe_tests)
endif()

add_executable(dcompframe_demo demo/main.cpp)
target_compile_features(dcompframe_demo PRIVATE cxx_std_20)
target_link_libraries(dcompframe_demo PRIVATE dcompframe fmt::fmt)
