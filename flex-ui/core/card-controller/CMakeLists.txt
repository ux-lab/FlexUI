add_library(flexui_core_card_controller STATIC)
target_include_directories(flexui_core_card_controller
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(flexui_core_card_controller PUBLIC flexui_common)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc "")
target_sources(flexui_core_card_controller PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc)
