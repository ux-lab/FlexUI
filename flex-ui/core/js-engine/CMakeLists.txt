add_library(flexui_core_js_engine STATIC)
target_include_directories(flexui_core_js_engine
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(flexui_core_js_engine PUBLIC flexui_common)
# Sources appended by later tasks.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc "")
target_sources(flexui_core_js_engine PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc)
