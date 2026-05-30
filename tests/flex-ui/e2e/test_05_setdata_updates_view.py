"""Test 05: setData updates view.

Tap a card's button; verify the JS handler executed via HiLog.
"""
import pytest
from test_base import TestBase


@pytest.mark.device
class TestSetDataUpdatesView(TestBase):
    def test_setdata_updates_view(self):
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")
        assert self.wait_for_text("Weather Summary")

        self.click_text("Tap")
        log = self.grab_log("FLEXUI", lines=500)
        assert ("FLEXUI/Api/ConsoleLog" in log or
                "FLEXUI/Card/CallMethod" in log), \
            "tap event did not reach JS handler"
