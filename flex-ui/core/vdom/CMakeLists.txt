add_library(flexui_core_vdom STATIC)
target_include_directories(flexui_core_vdom
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(flexui_core_vdom PUBLIC flexui_common)
target_sources(flexui_core_vdom PRIVATE
  src/dom_argument.cc
  src/dom_node.cc
)
