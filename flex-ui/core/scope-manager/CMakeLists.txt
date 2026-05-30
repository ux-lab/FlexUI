add_library(flexui_core_scope_manager STATIC)
target_include_directories(flexui_core_scope_manager
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_sources(flexui_core_scope_manager PRIVATE
  src/scope.cc
  src/scope_manager.cc
)
target_link_libraries(flexui_core_scope_manager PUBLIC
  flexui_core_js_engine
  flexui_common
)
