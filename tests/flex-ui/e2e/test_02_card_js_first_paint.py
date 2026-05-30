"""Test 02: card-js first paint.

Verify the first card-js bundle renders within acceptable time.
"""
import time
import pytest
from test_base import TestBase


@pytest.mark.device
class TestCardJsFirstPaint(TestBase):
    def test_card_js_first_paint(self):
        self.click_text("FlexCard")
        assert self.wait_for_text("FlexCard Waterfall Demo", timeout=5)
        start = time.time()
        assert self.wait_for_text("Weather Summary", timeout=5), \
            "first card-js card not visible"
        elapsed_ms = (time.time() - start) * 1000
        assert elapsed_ms < 1000, f"first card-js paint took {elapsed_ms:.0f}ms"
