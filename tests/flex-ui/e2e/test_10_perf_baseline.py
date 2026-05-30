"""Test 10: Performance baseline capture.

Collect first-paint, setData latency, scroll FPS into a JSON file.
Does not fail unless data is unobtainable — perf is a baseline, not a gate.
"""
import json
import os
import re
import time
import pytest
from test_base import TestBase


@pytest.mark.device
@pytest.mark.slow
class TestPerfBaseline(TestBase):
    def test_capture_baseline(self):
        self.click_text("FlexCard")

        t0 = time.time()
        self.wait_for_text("Weather Summary")
        first_card_paint_ms = (time.time() - t0) * 1000

        t0 = time.time()
        self.wait_for_text("Trending")
        first_a2ui_paint_ms = (time.time() - t0) * 1000

        for _ in range(5):
            self.swipe(0.5, 0.7, 0.5, 0.2, 0.5)
        log = self.grab_log("FLEXUI", lines=2000)
        durations = [float(m.group(1)) for m in
                     re.finditer(r'FLEXUI/CommitPipeline/Apply.*duration_ms=([\d.]+)', log)]
        max_commit_ms = max(durations) if durations else None
        avg_commit_ms = (sum(durations) / len(durations)) if durations else None

        scope_create_ms = []
        for m in re.finditer(r'FLEXUI/Scope/Create.*?duration_ms=([\d.]+)', log):
            scope_create_ms.append(float(m.group(1)))
        avg_scope_ms = (sum(scope_create_ms) / len(scope_create_ms)) if scope_create_ms else None

        baseline = {
            "captured_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "device_info": str(self.d.device_info),
            "first_card_js_paint_ms": first_card_paint_ms,
            "first_a2ui_paint_ms": first_a2ui_paint_ms,
            "max_commit_pipeline_apply_ms": max_commit_ms,
            "avg_commit_pipeline_apply_ms": avg_commit_ms,
            "avg_scope_create_ms": avg_scope_ms,
            "scope_samples": len(scope_create_ms),
        }
        out_dir = os.path.join(os.path.dirname(__file__), "baseline")
        os.makedirs(out_dir, exist_ok=True)
        out_path = os.path.join(out_dir, f"perf_baseline_{time.strftime('%Y-%m-%d')}.json")
        with open(out_path, "w") as f:
            json.dump(baseline, f, indent=2)
        print(f"Baseline written: {out_path}\n{json.dumps(baseline, indent=2)}")
