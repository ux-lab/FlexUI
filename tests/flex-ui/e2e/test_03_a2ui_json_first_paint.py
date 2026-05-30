"""Test 03: a2ui-json first paint.

Verify the first a2ui-json bundle renders and the install/load lifecycle
reaches HiLog.
"""
import pytest
from test_base import TestBase


@pytest.mark.device
class TestA2UIJsonFirstPaint(TestBase):
    def test_a2ui_json_first_paint(self):
        self.click_text("FlexCard")
        assert self.wait_for_text("FlexCard Waterfall Demo", timeout=5)
        assert self.wait_for_text("Trending", timeout=5), \
            "first a2ui-json card not visible"
        log = self.grab_log("FLEXUI", lines=500)
        assert "FLEXUI/Frontend/A2UIInit" in log, "a2ui frontend init not logged"
