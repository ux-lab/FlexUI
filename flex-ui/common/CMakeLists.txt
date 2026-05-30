add_library(flexui_common STATIC)
target_include_directories(flexui_common
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_sources(flexui_common PRIVATE
  # Sources appended incrementally by later tasks in the W1-W2 absorption plan.
  src/string_view.cc
  src/string_utils.cc
  src/logging.cc
  src/log_settings.cc
  src/log_settings_state.cc
  src/log_sink.cc
  src/log_tag.cc
  src/platform/log_sink_harmony.cc
  src/platform/log_sink_android.cc
  # Task 11: task + task_runner
  src/task.cc
  src/idle_task.cc
  src/task_runner.cc
  # Task 12: worker family + cv_driver (replaces worker_stub.cc)
  src/cv_driver.cc
  src/worker.cc
  src/worker_impl.cc
  src/worker_manager.cc
  # Task 13: timer family
  src/base_timer.cc
  src/one_shot_timer.cc
  src/repeating_timer.cc
  src/idle_timer.cc
  # Task 15: flexui_value (renamed from hippy_value)
  src/flexui_value.cc
)
# Force at least one TU so the static lib produces an archive even when
# every source so far is header-only.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc "")
target_sources(flexui_common PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc)
