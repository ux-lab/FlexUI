add_library(flexui_api STATIC
  src/api_base_package.cc
  console/console_api.cc
  timer/timer_api.cc
  log/log_api.cc
)

target_include_directories(flexui_api PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_include_directories(flexui_api PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/console
  ${CMAKE_CURRENT_SOURCE_DIR}/timer
  ${CMAKE_CURRENT_SOURCE_DIR}/log
)
target_link_libraries(flexui_api PUBLIC
  flexui_common
  flexui_core_plugin_host
)
