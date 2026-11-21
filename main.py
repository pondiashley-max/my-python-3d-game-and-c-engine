#!/usr/bin/env python3
"""
MAXIMUS: DOUALA RUNNER 3D
Full 3D Subway Surfers-style endless & level-based runner set in Douala, Cameroon.
Built strictly with C++17 (native pybind11 engine) and Python (OpenGL 3.3 / ModernGL / Pygame).

Usage:
    python3 main.py
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))


def ensure_native_engine():
    import build as engine_builder

    engine_builder.build(force=False)
    import _maximus_engine as eng_mod

    return eng_mod


def main() -> int:
    eng_mod = ensure_native_engine()
    from game.app import MaximusApp

    app = MaximusApp(eng_mod, width=1280, height=720)
    app.run()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
