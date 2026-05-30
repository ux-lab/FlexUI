include(FetchContent)

FetchContent_Declare(
  yoga
  GIT_REPOSITORY https://github.com/facebook/yoga.git
  GIT_TAG        v3.0.4
  GIT_SHALLOW    TRUE
  # Patch out the tests/ subdirectory from the top-level CMakeLists to avoid
  # Yoga pulling in its own GTest which conflicts with ours.
  PATCH_COMMAND   ${CMAKE_COMMAND} -E copy_if_different
                  ${CMAKE_CURRENT_LIST_DIR}/yoga_toplevel_cmake_patch.cmake
                  <SOURCE_DIR>/CMakeLists.txt
)
FetchContent_MakeAvailable(yoga)

# Yoga v3.x exports the target as "yogacore". Try it; fall back to "yoga" for
# older versions that used a different name.
if(TARGET yogacore)
  set(YOGA_TARGET yogacore)
elseif(TARGET yoga)
  set(YOGA_TARGET yoga)
else()
  message(FATAL_ERROR "FetchContent for Yoga did not produce a known target (yogacore or yoga)")
endif()

add_library(flexui_core_layout STATIC)
target_include_directories(flexui_core_layout
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_sources(flexui_core_layout PRIVATE
  src/layout_node.cc
  src/yoga_layout_node.cc
)
target_link_libraries(flexui_core_layout PUBLIC flexui_common flexui_core_vdom ${YOGA_TARGET})
