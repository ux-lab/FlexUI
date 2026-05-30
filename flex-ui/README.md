# flex-ui

FlexUI framework body. See `docs/superpowers/specs/2026-05-30-flexui-layering-design.md`
for the design.

This directory is independent of the legacy AGenUI tree at `core/` +
`platforms/agenui/`; see §5.1 of the spec for the coexistence discipline.

Sub-modules (added incrementally over W1-W10):

| Module      | Status     | Plan |
|-------------|------------|------|
| `common/`   | W1-W2      | docs/superpowers/plans/2026-05-30-flexui-w1-w2-absorption.md |
| `core/`     | W3-W6      | (TBD) |
| `components/` | W7-W8    | (TBD) |
| `api/`      | W7-W8      | (TBD) |
| `plugin-a2ui/` | W7-W8   | (TBD) |
| `platforms/` | W5-W10    | (TBD) |

## Known TSan findings (absorbed)

All 34 tests pass functionally. The TSan sub-target (`flexui_unit_tests_tsan`) covering
`task_runner`, `worker`, `worker_manager`, and `timer` reports **43 TSan warnings** from 7
distinct race sites in code absorbed unchanged from Hippy footstone. These are inherited
from upstream and are **not introduced by FlexUI**. Per the W1-W2 plan, we document but
do not modify absorbed source to fix them.

| Count | Location | Description |
|------:|----------|-------------|
| 36 | `worker.cc:212` — `Worker::Bind()` | Worker's internal task-runner list read/written without holding the mutex; background worker thread races with `Bind()` caller |
| 36 | `list:958` — `std::list::__link_nodes` | Same race propagated into libc++ list internals via `Worker::Bind()` |
| 4 | `base_timer.cc:78` — `BaseTimer::OnScheduledTaskInvoked()` | Timer state flag (`running_`) read on callback thread, written on control thread without atomic/lock |
| 3 | `cv_driver.cc:52` — `CVDriver::Start()` | CV driver thread-start flag checked before the worker thread has fully registered its memory stores |
| 2 | `worker.cc:221` — `Worker::Bind()` (list overload) | Same root cause as `worker.cc:212`, different list method |
| 2 | `vector.h:750` / `list:594` — libc++ internals | Propagated from the `Worker::Bind()` races above |
| 1 | `cv_driver.cc:59` — `CVDriver::Terminate()` | Symmetric to the `Start()` race; the termination flag checked on the calling thread races with the worker thread |

**Fix direction (deferred):** The `Worker::Bind()` races require extending the existing mutex
coverage to also protect the internal `runners_list_` during the initial bind. The timer and
`CVDriver` races require either `std::atomic` flags or an additional happens-before edge
(e.g., `std::atomic_thread_fence`) around the start/stop paths. These fixes belong to a
dedicated TSan-clean-up task in W3+, after the absorbed code stabilizes.
