# tests/flex-ui

Test tree for the FlexUI framework body (`flex-ui/`). Independent of
`tests/cpp/` (the AGenUI tree).

Run:
  ./scripts/flex-ui/build.sh                # ASan + UBSan
  ./scripts/flex-ui/build.sh --tsan-only    # TSan
  ./scripts/flex-ui/build.sh --no-san       # No sanitizers
  ./scripts/flex-ui/build.sh --coverage     # gcov / llvm-cov instrumentation
