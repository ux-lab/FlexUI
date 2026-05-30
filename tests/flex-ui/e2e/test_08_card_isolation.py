"""Test 08: Card isolation.

Tapping one card must not affect another card's visible state.
"""
import re
import pytest
from test_base import TestBase


@pytest.mark.device
class TestCardIsolation(TestBase):
    def test_card_isolation(self):
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")
        assert self.wait_for_text("Weather Summary")
        assert self.wait_for_text("Daily Digest")

        self.screenshot("isolation_before")
        self.click_text("Tap")
        assert self.find_text("Daily Digest").exists()
        self.screenshot("isolation_after")

        log = self.grab_log("FLEXUI", lines=500)
        scopes = set(re.findall(r'scope=(\d+)', log))
        assert len(scopes) >= 2, f"only {len(scopes)} scopes seen: {scopes}"
