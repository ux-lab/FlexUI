add_library(flexui_core_bridge STATIC)
target_include_directories(flexui_core_bridge
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(flexui_core_bridge PUBLIC flexui_common)
target_sources(flexui_core_bridge PRIVATE
  src/encoder.cc
  src/decoder.cc
  src/inprocess_bridge.cc
)
