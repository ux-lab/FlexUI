add_library(flexui_common STATIC)
target_include_directories(flexui_common
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_sources(flexui_common PRIVATE
  # Sources appended incrementally by later tasks in the W1-W2 absorption plan.
)
# Force at least one TU so the static lib produces an archive even when
# every source so far is header-only.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc "")
target_sources(flexui_common PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc)
