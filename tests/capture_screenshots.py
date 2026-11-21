#!/usr/bin/env python3
"""
Offscreen OpenGL 3.3 Core visual verification script for Douala RUN (Maximus 3D).
Captures real 1280x720 PNG screenshots of the animated Intro phases, Politique de Confidentialite,
Document Juridique, Title screen with creator profile head bar, About Us / Programmer Biography,
and live 3D Douala districts.
"""

from __future__ import annotations

import sys
import tempfile
import time
from pathlib import Path

import moderngl
import pygame
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

import build as engine_builder

engine_builder.build(force=False)
import _maximus_engine as E
from game.hud import HudManager, LegalUiState
from game.levels import get_level, to_engine_params
from game.renderer import DoualaRenderer
from game.save_manager import SaveManager

OUT_DIR = ROOT / "screenshots"
OUT_DIR.mkdir(parents=True, exist_ok=True)


def save_fbo(fbo: moderngl.Framebuffer, w: int, h: int, path: Path) -> None:
    raw = fbo.read(components=3, alignment=1)
    img = Image.frombytes("RGB", (w, h), raw).transpose(Image.FLIP_TOP_BOTTOM)
    img.save(path)


def main() -> int:
    w, h = 1280, 720
    pygame.init()
    ctx = moderngl.create_standalone_context(require=330)
    print(f"[GL] Renderer: {ctx.info.get('GL_RENDERER')} | Version: {ctx.info.get('GL_VERSION')}")

    color_att = ctx.texture((w, h), 4)
    depth_att = ctx.depth_renderbuffer((w, h))
    fbo = ctx.framebuffer(color_attachments=[color_att], depth_attachment=depth_att)

    eng = E.GameEngine()
    eng.set_aspect(w / float(h))
    renderer = DoualaRenderer(ctx, eng, E, w, h)
    hud = HudManager(w, h)

    with tempfile.TemporaryDirectory() as tmp:
        save = SaveManager(Path(tmp) / "shot_save.json")
        upgrades = save.to_engine_upgrades(E)

        def capture(filename: str, screen: str, level_id: int = 1, steps: int = 90,
                    intro_time: float = 0.0, legal_state: LegalUiState | None = None,
                    last_stars: int = 0):
            lvl = get_level(level_id)
            p = to_engine_params(lvl, E)
            eng.start_level(p, upgrades, demo_mode=True)
            for _ in range(steps):
                eng.step(1.0 / 60.0)
            snap = eng.snapshot()
            hud_rgba = hud.render(
                screen, snap, save, lvl, (640, 360), 0.016,
                last_stars=last_stars, fps=60.0,
                intro_time=intro_time, legal_state=legal_state,
            )
            renderer.render_frame(eng, lvl.theme, 0.016, hud_rgba=hud_rgba, target_fbo=fbo)
            ctx.finish()
            save_fbo(fbo, w, h, OUT_DIR / filename)
            print(f"[SHOT] {filename}")

        # Capture GTA IV intro, lowercase no-accent UI, 2-bone fluid motion, PNJs & night headlights!
        capture("20_gta4_intro_phase1.png", "INTRO", 1, steps=60, intro_time=1.45)
        capture("21_gta4_intro_phase2_art.png", "INTRO", 1, steps=75, intro_time=4.60)
        capture("22_legal_lowercase_no_accent.png", "LEGAL_GATE", 1, steps=80,
                legal_state=LegalUiState(active_tab=0, accept_privacy=True, accept_legal=True))
        capture("23_title_lowercase.png", "TITLE", 1, steps=90)
        capture("24_about_us_lowercase.png", "ABOUT_US", 1, steps=95)
        capture("25_akwa_pnj_2bone.png", "PLAYING", 1, steps=110)
        capture("26_mboppi_pnj_vendors.png", "PLAYING", 2, steps=120)
        capture("27_bonanjo_night_headlights.png", "PLAYING", 5, steps=150)

        # Quick FPS check
        lvl5 = get_level(5)
        eng.start_level(to_engine_params(lvl5, E), upgrades, demo_mode=True)
        t0 = time.perf_counter()
        for i in range(60):
            eng.step(1.0 / 60.0)
            snap = eng.snapshot()
            hbytes = hud.render("PLAYING", snap, save, lvl5, (0, 0), 0.016, fps=60.0) if (i % 2 == 0) else None
            renderer.render_frame(eng, lvl5.theme, 0.016, hud_rgba=hbytes, draw_hud=True, target_fbo=fbo)
        ctx.finish()
        elapsed = time.perf_counter() - t0
        print(f"[BENCH] 60 full frames (30Hz HUD upload + 60Hz 3D): {elapsed:.2f}s -> {60.0 / elapsed:.1f} FPS")

    renderer.release()
    fbo.release()
    color_att.release()
    depth_att.release()
    ctx.release()
    pygame.quit()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
