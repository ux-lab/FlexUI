add_library(flexui_plugin_a2ui STATIC
  src/a2ui_plugin.cc
  src/a2ui_json_frontend.cc
  src/a2ui_json_parser.cc
  src/a2ui_data_binder.cc
  src/a2ui_handlers_runtime.cc
)

target_include_directories(flexui_plugin_a2ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(flexui_plugin_a2ui PUBLIC
  flexui_common
  flexui_core_vdom
  flexui_core_plugin_host
  flexui_core_scope_manager
)
