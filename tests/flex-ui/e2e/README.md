# FlexUI HarmonyOS E2E tests

pytest + hmnextauto. Adapted from the dview project's TestBase.

## Setup

    python3 -m venv .venv
    source .venv/bin/activate
    pip install -r requirements.txt

## Run

    # All scenarios (requires device connected via HDC)
    pytest tests/flex-ui/e2e -m device

    # Single scenario
    pytest tests/flex-ui/e2e/test_02_card_js_first_paint.py -v
