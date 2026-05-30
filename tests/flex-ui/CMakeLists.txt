# =============================================================================
# FlexUI C++ Test Suite (independent of tests/cpp/ for AGenUI)
# =============================================================================
cmake_minimum_required(VERSION 3.18)
project(flexui_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FLEXUI_TESTS_ENABLE_ASAN "Enable AddressSanitizer + UBSan" ON)
option(FLEXUI_TESTS_ENABLE_TSAN "Enable ThreadSanitizer (mutex with ASan)" OFF)
option(FLEXUI_TESTS_ENABLE_COVERAGE "Enable coverage instrumentation" OFF)

if(FLEXUI_TESTS_ENABLE_ASAN AND FLEXUI_TESTS_ENABLE_TSAN)
  message(FATAL_ERROR "ASan and TSan are mutually exclusive; pick one.")
endif()

set(FLEXUI_TESTS_SAN_FLAGS "")
set(FLEXUI_TESTS_SAN_LINK_FLAGS "")
if(FLEXUI_TESTS_ENABLE_ASAN)
  list(APPEND FLEXUI_TESTS_SAN_FLAGS -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
  list(APPEND FLEXUI_TESTS_SAN_LINK_FLAGS -fsanitize=address -fsanitize=undefined)
endif()
if(FLEXUI_TESTS_ENABLE_TSAN)
  list(APPEND FLEXUI_TESTS_SAN_FLAGS -fsanitize=thread -fno-omit-frame-pointer)
  list(APPEND FLEXUI_TESTS_SAN_LINK_FLAGS -fsanitize=thread)
endif()
if(FLEXUI_TESTS_ENABLE_COVERAGE)
  list(APPEND FLEXUI_TESTS_SAN_FLAGS --coverage -O0 -g)
  list(APPEND FLEXUI_TESTS_SAN_LINK_FLAGS --coverage)
endif()

add_compile_options(${FLEXUI_TESTS_SAN_FLAGS})
add_link_options(${FLEXUI_TESTS_SAN_LINK_FLAGS})

# Pull in flex-ui as a subdirectory build.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../flex-ui ${CMAKE_CURRENT_BINARY_DIR}/flex-ui-build)

# GoogleTest: use pre-built local cache from tests/cpp if available,
# otherwise fall back to FetchContent (requires network).
set(_GTEST_LOCAL "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/cpp/build/_deps/googletest-src")
if(EXISTS "${_GTEST_LOCAL}/CMakeLists.txt")
  # Suppress install rules from googletest when included this way.
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
  add_subdirectory("${_GTEST_LOCAL}" "${CMAKE_CURRENT_BINARY_DIR}/googletest-build" EXCLUDE_FROM_ALL)
else()
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

add_library(flexui_test_support STATIC
  support/capturing_log_sink.cc
  unit/support/fake_js_engine.cc
)
target_include_directories(flexui_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(flexui_test_support PUBLIC flexui_common flexui_core_js_engine)

add_subdirectory(unit)
