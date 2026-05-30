add_library(flexui_core_commit_pipeline STATIC)
target_include_directories(flexui_core_commit_pipeline
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(flexui_core_commit_pipeline PUBLIC
  flexui_common flexui_core_reconciler flexui_core_plugin_host
)
target_sources(flexui_core_commit_pipeline PRIVATE
  src/commit_pipeline.cc
)
