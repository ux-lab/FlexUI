add_executable(flexui_unit_tests
  common_smoke_test.cc
  base_primitives_test.cc
  string_view_test.cc
)
target_link_libraries(flexui_unit_tests PRIVATE
  flexui_common
  GTest::gtest
  GTest::gtest_main
)
add_test(NAME flexui_unit_tests COMMAND flexui_unit_tests)
