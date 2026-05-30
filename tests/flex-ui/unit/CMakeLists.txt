add_executable(flexui_unit_tests
  common_smoke_test.cc
  base_primitives_test.cc
  string_view_test.cc
  string_utils_test.cc
  logging_test.cc
  log_sink_test.cc
)
target_link_libraries(flexui_unit_tests PRIVATE
  flexui_common
  flexui_test_support
  GTest::gtest
  GTest::gtest_main
)
add_test(NAME flexui_unit_tests COMMAND flexui_unit_tests)
