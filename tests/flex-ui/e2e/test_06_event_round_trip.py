"""Test 06: Event round-trip.

Native click -> C++ Component::OnEvent -> frontend -> JS handler.
"""
import pytest
from test_base import TestBase


@pytest.mark.device
class TestEventRoundTrip(TestBase):
    def test_event_round_trip(self):
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")
        assert self.wait_for_text("Tap")

        self.click_text("Tap")
        log = self.grab_log("FLEXUI", lines=500)
        assert "FLEXUI/Frontend/" in log, "frontend did not see event"
