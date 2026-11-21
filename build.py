#!/usr/bin/env python3
"""
Pure-Python C++17 build driver for Maximus: Douala Runner 3D.
Compiles engine/src/*.cpp into the native pybind11 module `_maximus_engine`.
"""

from __future__ import annotations

import os
import platform
import subprocess
import sys
import sysconfig
from pathlib import Path

ROOT = Path(__file__).resolve().parent
ENGINE_DIR = ROOT / "engine"
INCLUDE_DIR = ENGINE_DIR / "include"
SRC_DIR = ENGINE_DIR / "src"


def get_output_path() -> Path:
    ext_suffix = sysconfig.get_config_var("EXT_SUFFIX") or ".so"
    return ROOT / f"_maximus_engine{ext_suffix}"


def needs_rebuild(out_path: Path) -> bool:
    if not out_path.exists():
        return True
    out_mtime = out_path.stat().st_mtime
    sources = list(SRC_DIR.glob("*.cpp")) + list(INCLUDE_DIR.glob("*.hpp"))
    return any(p.stat().st_mtime > out_mtime for p in sources)


def build(force: bool = False) -> Path:
    out_path = get_output_path()
    if not force and not needs_rebuild(out_path):
        print(f"[build] Native C++ engine up to date: {out_path.name}")
        return out_path

    import pybind11

    py_inc = sysconfig.get_path("include")
    pb_inc = pybind11.get_include()
    sources = sorted(str(p) for p in SRC_DIR.glob("*.cpp"))

    system = platform.system()
    compiler = os.environ.get("CXX") or os.environ.get("cxx") or ("clang++" if system == "Darwin" else "g++")

    cmd = [
        compiler,
        "-O3",
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-fPIC",
        "-shared",
        f"-I{INCLUDE_DIR}",
        f"-I{py_inc}",
        f"-I{pb_inc}",
    ]

    if system == "Darwin":
        cmd.extend(["-undefined", "dynamic_lookup"])

    cmd.extend(sources)
    cmd.extend(["-o", str(out_path)])

    print("[build] Compiling C++17 native engine (_maximus_engine)...")
    print("[build]", " ".join(cmd))
    subprocess.run(cmd, check=True)
    print(f"[build] Built successfully: {out_path}")
    return out_path


if __name__ == "__main__":
    force_flag = "--force" in sys.argv or "-f" in sys.argv
    build(force=force_flag)
