add_subdirectory(frontends)

add_library(flexui_core_card_controller STATIC)
target_include_directories(flexui_core_card_controller
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_sources(flexui_core_card_controller PRIVATE
  src/flex_ui_engine.cc
  src/flex_card_controller.cc
)
target_link_libraries(flexui_core_card_controller PUBLIC
  flexui_core_js_engine
  flexui_core_scope_manager
  flexui_core_plugin_host
  flexui_core_commit_pipeline
  flexui_core_bridge
  flexui_core_card_js_frontend
  flexui_plugin_a2ui
  flexui_components
  flexui_api
  flexui_common
)
