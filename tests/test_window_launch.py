#!/usr/bin/env python3
"""
End-to-end live SDL2 + OpenGL 3.3 Core window smoke test for Douala RUN (MaximusApp).
Verifies:
- Animated Intro ("Douala RUN" -> "Created by PONDI ASHLEY REY MAXIM")
- Mandatory Politique de Confidentialite & Document Juridique acceptance gate before entering game
- Title screen, About Us / Programmer Biography, Level Select, live 3D gameplay & clean shutdown
"""

from __future__ import annotations

import sys
import tempfile
from pathlib import Path

import pygame

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

import build as engine_builder

engine_builder.build(force=False)
import _maximus_engine as E
from game.app import MaximusApp
from game.save_manager import SaveManager


def _pump(app: MaximusApp, frames: int = 8, allow_events: bool = False) -> None:
    for _ in range(frames):
        dt = 1.0 / 60.0
        if allow_events:
            app._process_events()
        else:
            pygame.event.pump()
            pygame.event.clear()
        app._update_transition(dt)
        if app.screen == "INTRO":
            app.intro_time += dt
        app.engine.step(dt)
        snap = app.engine.snapshot()
        hud = app.hud.render(
            app.screen, snap, app.save, app.current_level, (640, 360), dt,
            fps=60.0, intro_time=app.intro_time, legal_state=app.legal_state,
            fade_alpha=app._fade_alpha,
        )
        app.renderer.render_frame(app.engine, app.active_theme, dt, hud_rgba=hud)
        pygame.display.flip()


def main() -> int:
    app = MaximusApp(E, width=1280, height=720)
    # Isolate test save state so user's real first-launch legal gate stays intact!
    with tempfile.TemporaryDirectory() as tmp:
        app.save = SaveManager(Path(tmp) / "smoke_save.json")
        app.legal_state.accept_privacy = False
        app.legal_state.accept_legal = False

        # 1. Verify starts on Animated Intro
        assert app.screen == "INTRO", f"Expected INTRO, got {app.screen}"
        _pump(app, 6)
        app.intro_time = app.hud.INTRO_PHASE1_END + 0.8
        _pump(app, 6)

        # 2. Finish intro -> must enter LEGAL_GATE (cannot bypass!)
        app._finish_intro()
        _pump(app, 32)
        assert app.screen == "LEGAL_GATE", f"Expected LEGAL_GATE before game entry, got {app.screen}"

        # Attempting to accept without ticking checkboxes must NOT enter the game
        app._handle_button("ACCEPT_LEGAL")
        _pump(app, 6)
        assert app.screen == "LEGAL_GATE", "Must block game entry until both legal boxes checked"

        # Trying to force-open Title without acceptance must stay on LEGAL_GATE
        app._handle_button("OPEN_TITLE")
        _pump(app, 18)
        assert app.screen == "LEGAL_GATE", "Must not allow skipping legal gate to TITLE"

        # Check both Privacy & Juridique boxes and accept -> transitions to TITLE!
        app._handle_button("TOGGLE_PRIVACY")
        app._handle_button("TOGGLE_LEGAL")
        app._handle_button("ACCEPT_LEGAL")
        _pump(app, 36)
        assert app.screen == "TITLE", f"Expected TITLE after legal acceptance, got {app.screen}"

        # 3. Open About Us screen
        app._handle_button("OPEN_ABOUT")
        _pump(app, 36)
        assert app.screen == "ABOUT_US", f"Expected ABOUT_US, got {app.screen}"

        # 4. Open Level Select -> Start Level 1 -> Play with real keyboard inputs
        app._handle_button("OPEN_LEVELS")
        _pump(app, 36)
        assert app.screen == "LEVEL_SELECT", f"Expected LEVEL_SELECT, got {app.screen}"

        app._handle_button("START_LEVEL", 1)
        assert app.screen == "PLAYING"

        for key in (pygame.K_LEFT, pygame.K_RIGHT, pygame.K_UP, pygame.K_DOWN, pygame.K_SPACE):
            pygame.event.clear()
            pygame.event.post(pygame.event.Event(pygame.KEYDOWN, {"key": key}))
            _pump(app, 6, allow_events=True)

        snap = app.engine.snapshot()
        assert snap.distance > 5.0, f"Expected distance > 5m, got {snap.distance}"
        assert snap.hoverboard_timer > 0.0, "Expected hoverboard active after SPACE key"

    app.audio.stop_music()
    app.renderer.release()
    pygame.quit()
    print(f"[PASS] Full flow verified: INTRO -> LEGAL_GATE -> TITLE -> ABOUT_US -> LEVEL_SELECT -> PLAYING (dist={snap.distance:.1f}m)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
