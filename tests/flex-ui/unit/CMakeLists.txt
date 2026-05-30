add_executable(flexui_unit_tests
  common_smoke_test.cc
  base_primitives_test.cc
  string_view_test.cc
  string_utils_test.cc
  logging_test.cc
  log_sink_test.cc
  log_tag_test.cc
  task_runner_test.cc
  # Task 12: worker family
  worker_test.cc
  worker_manager_test.cc
  # Task 13: timer family
  timer_test.cc
  # Task 14: persistent_object_map + hash
  persistent_object_map_test.cc
  # Task 15: flexui_value (renamed from hippy_value)
  flexui_value_test.cc
  # Task 16: serializer + deserializer roundtrip
  serializer_roundtrip_test.cc
  # Task 17: error module (FlexUI-original)
  error_test.cc
  # Task 19: coverage gate — extended tests
  string_view_extended_test.cc
  serializer_extended_test.cc
  task_runner_extended_test.cc
  worker_extended_test.cc
  coverage_boost_test.cc
  # Task T2: vdom DomNode + DomArgument + NodeProps headers
  vdom_dom_node_test.cc
  # Task T4: reconciler diff_utils + Mutation typedef
  reconciler_diff_test.cc
  # Task T5: layout yoga
  layout_yoga_test.cc
  # Task T10: QuickJS backend unit tests
  js_engine_quickjs_test.cc
  # Task T12: JSVM smoke test (host: factory returns null)
  js_engine_jsvm_test.cc
  # Task T14: core coverage boost
  core_coverage_boost_test.cc
)
target_link_libraries(flexui_unit_tests PRIVATE
  flexui_common
  flexui_core_vdom
  flexui_core_reconciler
  flexui_core_layout
  flexui_core_js_engine
  flexui_test_support
  GTest::gtest
  GTest::gtest_main
)
add_test(NAME flexui_unit_tests COMMAND flexui_unit_tests)

# TSan sub-target: concurrency-sensitive subset only (task_runner / worker / timer).
# Build with: cmake -DFLEXUI_TESTS_ENABLE_TSAN=ON ... then ctest -R flexui_unit_tests_tsan
add_executable(flexui_unit_tests_tsan
  task_runner_test.cc
  worker_test.cc
  worker_manager_test.cc
  timer_test.cc
)
target_link_libraries(flexui_unit_tests_tsan PRIVATE
  flexui_common
  flexui_test_support
  GTest::gtest
  GTest::gtest_main
)
add_test(NAME flexui_unit_tests_tsan COMMAND flexui_unit_tests_tsan)
