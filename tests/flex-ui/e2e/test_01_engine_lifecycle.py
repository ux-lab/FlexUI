"""Test 01: Engine Lifecycle.

Cold start, navigate to FlexCard page, leave the page, check no leaks.
"""
import pytest
from test_base import TestBase


@pytest.mark.device
class TestEngineLifecycle(TestBase):
    def test_engine_init_and_destroy(self):
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")
        # Navigate back; aboutToDisappear should destroy controllers and shutdown engine.
        self.click_text("← Back")
        log = self.grab_log("FLEXUI", lines=500)
        assert "FLEXUI/Engine/Init" in log, "engine init not logged"
        assert "FLEXUI/Engine/Shutdown" in log, "engine shutdown not logged"
