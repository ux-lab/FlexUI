cmake_minimum_required(VERSION 3.18)
project(flex_ui C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FLEXUI_OHOS "Build Harmony NAPI + card host" OFF)

add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(components)
add_subdirectory(api)
add_subdirectory(plugin-a2ui)
add_subdirectory(platforms)
