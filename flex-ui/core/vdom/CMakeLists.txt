add_library(flexui_core_vdom STATIC)
target_include_directories(flexui_core_vdom
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(flexui_core_vdom PUBLIC flexui_common)
# Sources appended by later tasks.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc "")
target_sources(flexui_core_vdom PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc)
