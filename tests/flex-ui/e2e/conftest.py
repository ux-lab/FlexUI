"""Pytest configuration for FlexUI HarmonyOS E2E tests.

Adapted from dview/tests/integration/harmonyos/conftest.py.
"""
import os
import pytest


def pytest_configure(config):
    config.addinivalue_line("markers", "device: requires a HarmonyOS device via HDC")
    config.addinivalue_line("markers", "slow: takes longer than 10 seconds")


@pytest.fixture(autouse=True)
def dump_diagnostics_on_failure(request):
    """When a test fails, attach FlexUIEngine.DumpDiagnostics + final screenshot + HiLog."""
    yield
    if request.node.rep_call.failed if hasattr(request.node, "rep_call") else False:
        inst = request.instance
        if inst is None:
            return
        diag = inst.dump_diagnostics()
        log = inst.grab_log("FLEXUI", lines=500)
        ss = inst.screenshot(f"FAIL_{request.node.name}")
        with open(os.path.join(os.path.dirname(ss), f"FAIL_{request.node.name}.log"), "w") as f:
            f.write("=== DumpDiagnostics ===\n" + diag + "\n\n=== HiLog ===\n" + log)
        print(f"Failure evidence: {ss} + sibling .log")


@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    outcome = yield
    rep = outcome.get_result()
    setattr(item, f"rep_{rep.when}", rep)
