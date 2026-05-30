add_library(flexui_components STATIC
  src/components_base_package.cc
  text/text_component.cc
  image/image_component.cc
  view/view_component.cc
  button/button_component.cc
  scrollview/scrollview_component.cc
)

if(FLEXUI_OHOS)
  target_sources(flexui_components PRIVATE
    text/platform/text_component_harmony.cc
    image/platform/image_component_harmony.cc
    view/platform/view_component_harmony.cc
    button/platform/button_component_harmony.cc
    scrollview/platform/scrollview_component_harmony.cc
  )
  target_link_libraries(flexui_components PRIVATE libace_ndk.z.so)
else()
  target_sources(flexui_components PRIVATE
    text/platform/text_component_android.cc
    image/platform/image_component_android.cc
    view/platform/view_component_android.cc
    button/platform/button_component_android.cc
    scrollview/platform/scrollview_component_android.cc
  )
endif()

target_include_directories(flexui_components PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(flexui_components PUBLIC
  flexui_common
  flexui_core_vdom
  flexui_core_layout
  flexui_core_plugin_host
  flexui_core_commit_pipeline
)
