add_library(flexui_napi SHARED
  napi/napi_init.cc
  napi/napi_engine.cc
  napi/napi_controller.cc
  napi/napi_node_content.cc
)
target_link_libraries(flexui_napi PRIVATE
  flexui_core_card_controller
  flexui_core_commit_pipeline
  flexui_core_bridge
  libace_napi.z.so
  libace_ndk.z.so
)
target_compile_definitions(flexui_napi PRIVATE FLEXUI_OHOS=1)
