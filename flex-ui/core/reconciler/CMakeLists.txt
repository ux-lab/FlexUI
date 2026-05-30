add_library(flexui_core_reconciler STATIC)
target_include_directories(flexui_core_reconciler
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(flexui_core_reconciler PUBLIC flexui_common flexui_core_vdom)
target_sources(flexui_core_reconciler PRIVATE
  src/diff_utils.cc
)
