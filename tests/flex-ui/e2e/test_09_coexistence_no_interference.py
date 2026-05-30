"""Test 09: Coexistence no interference (spec §7.4).

AGenUIContainer (legacy surface) + FlexCard run on adjacent pages,
both responsive, no interference.
"""
import pytest
from test_base import TestBase


@pytest.mark.device
class TestCoexistenceNoInterference(TestBase):
    def test_coexistence_no_interference(self):
        assert self.wait_for_text("AGenUI Demo"), \
            "AGenUI demo did not render at startup"
        self.click_text("FlexCard")
        assert self.wait_for_text("FlexCard Waterfall Demo")
        self.click_text("← Back")
        assert self.wait_for_text("AGenUI Demo")
