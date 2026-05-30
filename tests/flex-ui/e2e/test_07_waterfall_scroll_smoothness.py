"""Test 07: Waterfall scroll smoothness.

Sample FLEXUI/CommitPipeline/Apply durations during scroll.
Assert no commit > 32ms (one frame at 30fps).
"""
import re
import pytest
from test_base import TestBase


@pytest.mark.device
@pytest.mark.slow
class TestWaterfallScrollSmoothness(TestBase):
    def test_waterfall_scroll_smoothness(self):
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")

        for _ in range(5):
            self.swipe(0.5, 0.7, 0.5, 0.2, 0.5)

        log = self.grab_log("FLEXUI", lines=2000)
        durations = []
        for line in log.split('\n'):
            m = re.search(r'FLEXUI/CommitPipeline/Apply.*duration_ms=([\d.]+)', line)
            if m:
                durations.append(float(m.group(1)))
        assert len(durations) > 0, "no CommitPipeline durations sampled"
        max_d = max(durations)
        assert max_d <= 32, f"max commit duration {max_d}ms exceeds 32ms"
