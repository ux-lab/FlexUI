# ---- QuickJS vendored amalgamation ----
add_library(flexui_quickjs STATIC
  third_party/quickjs/quickjs.c
  third_party/quickjs/cutils.c
  third_party/quickjs/dtoa.c
  third_party/quickjs/libregexp.c
  third_party/quickjs/libunicode.c
)
target_include_directories(flexui_quickjs PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs
)
target_compile_definitions(flexui_quickjs PRIVATE
  CONFIG_VERSION="2025-09-13"
  _GNU_SOURCE
)
# QuickJS uses a few patterns sanitizers flag; suppress the noisier ones.
target_compile_options(flexui_quickjs PRIVATE
  -Wno-implicit-fallthrough
  -Wno-unused-but-set-variable
  -Wno-array-bounds
  -Wno-error
)

# ---- js-engine library ----
add_library(flexui_core_js_engine STATIC)
target_include_directories(flexui_core_js_engine
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(flexui_core_js_engine PUBLIC flexui_common flexui_quickjs)
# Sources appended by later tasks.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc "")
target_sources(flexui_core_js_engine PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc)
