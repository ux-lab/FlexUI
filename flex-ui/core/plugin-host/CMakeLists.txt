add_library(flexui_core_plugin_host STATIC)
target_include_directories(flexui_core_plugin_host
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(flexui_core_plugin_host PUBLIC flexui_common flexui_core_vdom)
target_sources(flexui_core_plugin_host PRIVATE
  src/plugin_host.cc
)
