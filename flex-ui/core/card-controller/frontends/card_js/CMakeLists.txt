add_library(flexui_core_card_js_frontend STATIC
  card_js_frontend.cc
)

target_include_directories(flexui_core_card_js_frontend PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
target_link_libraries(flexui_core_card_js_frontend PUBLIC
  flexui_common
  flexui_core_vdom
  flexui_core_plugin_host
  flexui_core_scope_manager
  flexui_core_js_engine
)
