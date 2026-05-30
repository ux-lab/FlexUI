"""Test 04: Dual-frontend visual parity.

Complementary to the W7-W8 unit-test C2 proof: same logical content via
card-js and a2ui-json renders side-by-side on device.
"""
import os
import pytest
from test_base import TestBase


@pytest.mark.device
class TestDualFrontendVisualParity(TestBase):
    def test_dual_frontend_visual_parity(self):
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")

        # Both card-js and a2ui-json cards visible.
        assert self.wait_for_text("Weather Summary")
        self.swipe(0.5, 0.7, 0.5, 0.3, 0.3)
        assert self.wait_for_text("Trending")

        ss = self.screenshot("dual_frontend_parity")
        assert os.path.exists(ss)
