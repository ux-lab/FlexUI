"""FlexUI HarmonyOS E2E TestBase.

Adapted from the dview project's TestBase pattern.
Provides HDC device control, screenshot capture, log grabbing, and
diagnostic helpers for FlexUI-specific test scenarios.
"""
import os
import re
import subprocess
import time


class TestBase:
    """Base class for FlexUI HarmonyOS E2E tests.

    Subclasses must set BUNDLE to the HarmonyOS app bundle name.
    """

    BUNDLE: str = "com.flexui.playground"
    BUNDLE_URI: str = ""

    @property
    def d(self):
        """Lazy hmnextauto Device instance."""
        if not hasattr(self, "_d"):
            from hmnextauto import Device
            self._d = Device()
        return self._d

    # ------------------------------------------------------------------
    # Log helpers
    # ------------------------------------------------------------------

    def grab_log(self, tag: str = "FLEXUI", lines: int = 500) -> str:
        """Pull HiLog lines matching *tag* from device."""
        try:
            result = subprocess.run(
                ["hdc", "shell", f"hilog -t {tag} -l {lines}"],
                capture_output=True, text=True, timeout=10,
            )
            return result.stdout
        except Exception as e:
            return f"<grab_log failed: {e}>"

    def dump_diagnostics(self) -> str:
        """Pull FlexUIEngine DumpDiagnostics output from HiLog."""
        try:
            result = subprocess.run(
                ["hdc", "shell", "hilog -t FLEXUI/Engine/Diagnostics -l 1000"],
                capture_output=True, text=True, timeout=10,
            )
            return result.stdout
        except Exception as e:
            return f"<dump failed: {e}>"

    # ------------------------------------------------------------------
    # Screenshot helpers
    # ------------------------------------------------------------------

    def screenshot(self, name: str) -> str:
        """Capture a device screenshot and save it.

        Returns the path to the saved PNG.
        """
        out_dir = os.path.join(os.path.dirname(__file__), "screenshots")
        os.makedirs(out_dir, exist_ok=True)
        path = os.path.join(out_dir, f"{name}.png")
        self.d.screenshot(path)
        return path

    def assert_screenshot_matches(self, baseline_path: str, candidate_path: str,
                                  tolerance_per_channel: int = 1) -> None:
        """Assert two screenshots are pixel-identical within tolerance."""
        from PIL import Image, ImageChops
        base = Image.open(baseline_path).convert("RGB")
        cand = Image.open(candidate_path).convert("RGB")
        diff = ImageChops.difference(base, cand)
        bbox = diff.getbbox()
        if bbox is None:
            return
        region = diff.crop(bbox)
        for (r, g, b) in region.getdata():
            if max(r, g, b) > tolerance_per_channel:
                raise AssertionError(
                    f"Screenshot diff exceeds tolerance "
                    f"(channel max={max(r, g, b)}, tol={tolerance_per_channel}) at {bbox}"
                )

    # ------------------------------------------------------------------
    # UI interaction helpers
    # ------------------------------------------------------------------

    def click_text(self, text: str, timeout: float = 10) -> bool:
        """Find and click a UI element by its text content."""
        el = self.d(text=text)
        if el.wait_exists(timeout=timeout * 1000):
            el.click()
            return True
        return False

    def wait_for_text(self, text: str, timeout: float = 10) -> bool:
        """Wait for a UI element with the given text to appear."""
        el = self.d(text=text)
        return el.wait_exists(timeout=timeout * 1000)

    def find_text(self, text: str):
        """Return the UI element matching *text* (hmnextauto UiObject)."""
        return self.d(text=text)

    def swipe(self, fx: float, fy: float, tx: float, ty: float,
              duration: float = 0.5) -> None:
        """Swipe from (fx, fy) to (tx, ty) in relative coordinates (0-1)."""
        w, h = self.d.window_size()
        self.d.swipe(int(fx * w), int(fy * h), int(tx * w), int(ty * h),
                     duration=int(duration * 1000))
