"""
Main Game Application for Douala RUN (Maximus 3D).
Glues the native C++17 engine (`_maximus_engine`), OpenGL 3.3 Core renderer,
procedural Douala Makossa/Afrobeat audio engine, cinematic intro animation,
mandatory Privacy/Legal acceptance gate, About Us / Programmer Biography, and HUD.

Copyright (c) 2024-2026 PONDI ASHLEY REY MAXIM. All Rights Reserved.
"""

from __future__ import annotations

from typing import Optional

import moderngl
import pygame

from .audio import DoualaAudio
from .hud import HudManager, LegalUiState
from .legal_content import CREATOR_NAME, GAME_BRAND
from .levels import ZONE_THEMES, LevelDef, get_level, lerp_theme, to_engine_params
from .renderer import DoualaRenderer
from .save_manager import SaveManager


class MaximusApp:
    def __init__(self, eng_mod, width: int = 1280, height: int = 720) -> None:
        self.eng_mod = eng_mod
        self.width = width
        self.height = height
        self.fullscreen = False

        pygame.init()
        pygame.display.set_caption(f"{GAME_BRAND} - maximus 3d | created by / cree par {CREATOR_NAME}")

        # Request OpenGL 3.3 Core Profile (required on macOS!)
        pygame.display.gl_set_attribute(pygame.GL_CONTEXT_MAJOR_VERSION, 3)
        pygame.display.gl_set_attribute(pygame.GL_CONTEXT_MINOR_VERSION, 3)
        pygame.display.gl_set_attribute(pygame.GL_CONTEXT_PROFILE_MASK, pygame.GL_CONTEXT_PROFILE_CORE)
        pygame.display.gl_set_attribute(pygame.GL_CONTEXT_FORWARD_COMPATIBLE_FLAG, 1)
        pygame.display.gl_set_attribute(pygame.GL_DEPTH_SIZE, 24)
        pygame.display.gl_set_attribute(pygame.GL_DOUBLEBUFFER, 1)

        flags = pygame.OPENGL | pygame.DOUBLEBUF | pygame.RESIZABLE
        self.window = pygame.display.set_mode((self.width, self.height), flags)
        self.ctx = moderngl.create_context()

        self.save = SaveManager()
        self.engine = eng_mod.GameEngine()
        self.engine.set_aspect(self.width / max(1.0, float(self.height)))

        self.renderer = DoualaRenderer(self.ctx, self.engine, eng_mod, self.width, self.height)
        self.hud = HudManager(self.width, self.height)
        self.audio = DoualaAudio(eng_mod, muted=bool(self.save.data.get("sound_muted", False)))

        # Start with the cinematic animated Intro ("Douala RUN" -> "Created by PONDI ASHLEY REY MAXIM")
        self.screen = "INTRO"
        self.intro_time = 0.0
        self.legal_state = LegalUiState(
            accept_privacy=self.save.is_legal_accepted(),
            accept_legal=self.save.is_legal_accepted(),
            review_only=False,
        )

        # Smooth fade transition state
        self._fade_alpha = 0.0
        self._fade_dir = 0           # +1 fading out to black, -1 fading back in
        self._pending_screen: Optional[str] = None

        self.current_level: LevelDef = get_level(1)
        self.active_theme = ZONE_THEMES[1]
        self.last_stars = 0
        self.recorded_run = False
        self.running = True

        self._start_demo_background()
        self.audio.start_music()

    def _transition_to(self, target_screen: str, instant: bool = False) -> None:
        if instant or target_screen == self.screen:
            self.screen = target_screen
            self._fade_alpha = 0.0
            self._fade_dir = 0
            self._pending_screen = None
            return
        self._pending_screen = target_screen
        self._fade_dir = 1

    def _update_transition(self, dt: float) -> None:
        if self._fade_dir == 1:
            self._fade_alpha = min(1.0, self._fade_alpha + dt * 4.5)
            if self._fade_alpha >= 1.0:
                if self._pending_screen is not None:
                    self.screen = self._pending_screen
                    self._pending_screen = None
                self._fade_dir = -1
        elif self._fade_dir == -1:
            self._fade_alpha = max(0.0, self._fade_alpha - dt * 4.2)
            if self._fade_alpha <= 0.0:
                self._fade_dir = 0

    def _finish_intro(self) -> None:
        if self._pending_screen is not None:
            return
        if not self.save.is_legal_accepted():
            self.legal_state.review_only = False
            self.legal_state.active_tab = 0
            self.legal_state.scroll_y = 0
            self._transition_to("LEGAL_GATE")
        else:
            self._transition_to("TITLE")

    def _start_demo_background(self) -> None:
        demo_lvl = get_level(6)
        params = to_engine_params(demo_lvl, self.eng_mod)
        params.base_speed = 18.5
        params.max_speed = 23.0
        params.obstacle_density = 0.45
        upgrades = self.save.to_engine_upgrades(self.eng_mod)
        self.engine.start_level(params, upgrades, demo_mode=True)
        self.current_level = get_level(1)
        self.active_theme = ZONE_THEMES[1]

    def start_level(self, level_id: int) -> None:
        if not self.save.is_legal_accepted():
            self.legal_state.review_only = False
            self._transition_to("LEGAL_GATE")
            return
        self.current_level = get_level(level_id)
        params = to_engine_params(self.current_level, self.eng_mod)
        upgrades = self.save.to_engine_upgrades(self.eng_mod)
        self.engine.start_level(params, upgrades, demo_mode=False)
        self.active_theme = self.current_level.theme
        self.screen = "PLAYING"
        self._fade_alpha = 0.0
        self._fade_dir = 0
        self._pending_screen = None
        self.last_stars = 0
        self.recorded_run = False

    def _toggle_fullscreen(self) -> None:
        self.fullscreen = not self.fullscreen
        flags = pygame.OPENGL | pygame.DOUBLEBUF
        if self.fullscreen:
            flags |= pygame.FULLSCREEN
            info = pygame.display.Info()
            self.width, self.height = info.current_w, info.current_h
        else:
            flags |= pygame.RESIZABLE
            self.width, self.height = 1280, 720
        self.window = pygame.display.set_mode((self.width, self.height), flags)
        self._handle_resize(self.width, self.height)

    def _handle_resize(self, w: int, h: int) -> None:
        self.width = max(320, w)
        self.height = max(240, h)
        self.engine.set_aspect(self.width / float(self.height))
        self.renderer.resize(self.width, self.height)
        self.hud.resize(self.width, self.height)

    def _handle_button(self, action: str, payload: object = None) -> None:
        if action in ("QUIT", "DECLINE_LEGAL"):
            self.running = False
        elif action == "LEGAL_TAB" and isinstance(payload, int):
            self.legal_state.active_tab = payload
            self.legal_state.scroll_y = 0
        elif action == "TOGGLE_PRIVACY":
            self.legal_state.accept_privacy = not self.legal_state.accept_privacy
        elif action == "TOGGLE_LEGAL":
            self.legal_state.accept_legal = not self.legal_state.accept_legal
        elif action == "ACCEPT_LEGAL":
            if self.legal_state.accept_privacy and self.legal_state.accept_legal:
                self.save.accept_legal()
                self.audio.play_events([self.eng_mod.SFX_POWERUP])
                self._transition_to("TITLE")
        elif action == "OPEN_LEGAL":
            self.legal_state.review_only = self.save.is_legal_accepted()
            self.legal_state.active_tab = 0
            self.legal_state.scroll_y = 0
            self._transition_to("LEGAL_GATE")
        elif action == "OPEN_ABOUT":
            self._transition_to("ABOUT_US")
        elif action == "REPLAY_INTRO":
            self.intro_time = 0.0
            self._transition_to("INTRO")
        elif action == "QUICK_PLAY":
            lvl_id = min(5, int(self.save.data.get("unlocked_level", 1)))
            self.start_level(lvl_id)
        elif action == "OPEN_LEVELS":
            self._transition_to("LEVEL_SELECT")
            if self.engine.snapshot().status != self.eng_mod.STATUS_DEMO:
                self._start_demo_background()
        elif action == "OPEN_TITLE":
            if not self.save.is_legal_accepted():
                self._transition_to("LEGAL_GATE")
                return
            self._transition_to("TITLE")
            if self.engine.snapshot().status != self.eng_mod.STATUS_DEMO:
                self._start_demo_background()
        elif action == "OPEN_SHOP":
            self._transition_to("SHOP")
        elif action == "OPEN_CONTROLS":
            self._transition_to("CONTROLS")
        elif action == "START_LEVEL" and isinstance(payload, int):
            self.start_level(payload)
        elif action == "RESTART":
            self.start_level(self.current_level.level_id)
        elif action == "RESUME":
            self.engine.set_paused(False)
            self.screen = "PLAYING"
        elif action == "TOGGLE_UNLOCK_ALL":
            self.save.toggle_unlock_all()
        elif action == "BUY_UPGRADE" and isinstance(payload, str):
            if self.save.buy_upgrade(payload):
                self.audio.play_events([self.eng_mod.SFX_POWERUP])
        elif action == "BUY_HOVERBOARD":
            if self.save.buy_hoverboard_pack():
                self.audio.play_events([self.eng_mod.SFX_HOVERBOARD_ON])
        elif action == "SELECT_OUTFIT" and isinstance(payload, int):
            if self.save.select_or_buy_outfit(payload):
                self.audio.play_events([self.eng_mod.SFX_POWERUP])
                if self.screen == "SHOP":
                    self._start_demo_background()

    def _process_events(self) -> None:
        cmd = self.eng_mod.InputCommands()
        has_input = False

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
            elif event.type == pygame.VIDEORESIZE:
                self._handle_resize(event.w, event.h)
            elif event.type == pygame.MOUSEWHEEL and self.screen == "LEGAL_GATE":
                self.legal_state.scroll_y = max(
                    0, min(self.legal_state.max_scroll, self.legal_state.scroll_y - event.y * 36)
                )
            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                if self.screen == "INTRO":
                    # Advance Phase 1 -> Phase 2 on first click, or finish intro on Phase 2!
                    if self.intro_time < self.hud.INTRO_PHASE1_END:
                        self.intro_time = self.hud.INTRO_PHASE1_END
                    else:
                        self._finish_intro()
                else:
                    hit = self.hud.hit_test(event.pos)
                    if hit is not None:
                        self._handle_button(hit.action, hit.payload)
            elif event.type == pygame.KEYDOWN:
                k = event.key

                # Global hotkeys
                if k == pygame.K_m:
                    muted = not self.audio.muted
                    self.audio.set_muted(muted)
                    self.save.data["sound_muted"] = muted
                    self.save.save()
                    continue
                if k in (pygame.K_f, pygame.K_F11):
                    self._toggle_fullscreen()
                    continue

                if self.screen == "INTRO":
                    if k in (pygame.K_SPACE, pygame.K_RETURN, pygame.K_ESCAPE):
                        if self.intro_time < self.hud.INTRO_PHASE1_END and k != pygame.K_ESCAPE:
                            self.intro_time = self.hud.INTRO_PHASE1_END
                        else:
                            self._finish_intro()
                    continue

                if self.screen == "LEGAL_GATE":
                    if k == pygame.K_DOWN:
                        self.legal_state.scroll_y = min(self.legal_state.max_scroll, self.legal_state.scroll_y + 42)
                    elif k == pygame.K_UP:
                        self.legal_state.scroll_y = max(0, self.legal_state.scroll_y - 42)
                    elif k == pygame.K_LEFT:
                        self.legal_state.active_tab = 0
                        self.legal_state.scroll_y = 0
                    elif k == pygame.K_RIGHT:
                        self.legal_state.active_tab = 1
                        self.legal_state.scroll_y = 0
                    elif k == pygame.K_ESCAPE and self.legal_state.review_only:
                        self._transition_to("TITLE")
                    continue

                if self.screen == "PLAYING":
                    if k in (pygame.K_LEFT, pygame.K_a):
                        cmd.left = True
                        has_input = True
                    elif k in (pygame.K_RIGHT, pygame.K_d):
                        cmd.right = True
                        has_input = True
                    elif k in (pygame.K_UP, pygame.K_w):
                        cmd.jump = True
                        has_input = True
                    elif k in (pygame.K_DOWN, pygame.K_s):
                        cmd.roll = True
                        has_input = True
                    elif k in (pygame.K_SPACE, pygame.K_e, pygame.K_LSHIFT, pygame.K_RSHIFT):
                        cmd.hoverboard = True
                        has_input = True
                    elif k in (pygame.K_ESCAPE, pygame.K_p):
                        self.engine.set_paused(True)
                        self.screen = "PAUSED"
                elif self.screen == "PAUSED":
                    if k in (pygame.K_ESCAPE, pygame.K_p):
                        self.engine.set_paused(False)
                        self.screen = "PLAYING"
                    elif k == pygame.K_r:
                        self.start_level(self.current_level.level_id)
                elif self.screen == "GAME_OVER":
                    if k in (pygame.K_r, pygame.K_RETURN, pygame.K_SPACE):
                        self.start_level(self.current_level.level_id)
                    elif k == pygame.K_ESCAPE:
                        self._handle_button("OPEN_LEVELS")
                elif self.screen == "LEVEL_COMPLETE":
                    if k in (pygame.K_RETURN, pygame.K_SPACE):
                        nxt = min(6, self.current_level.level_id + 1)
                        self.start_level(nxt)
                    elif k == pygame.K_r:
                        self.start_level(self.current_level.level_id)
                    elif k == pygame.K_ESCAPE:
                        self._handle_button("OPEN_LEVELS")
                elif self.screen == "TITLE":
                    if k in (pygame.K_RETURN, pygame.K_SPACE):
                        self._handle_button("QUICK_PLAY")
                    elif k == pygame.K_ESCAPE:
                        self.running = False
                elif self.screen in ("LEVEL_SELECT", "SHOP", "CONTROLS", "ABOUT_US"):
                    if k == pygame.K_ESCAPE:
                        self._handle_button("OPEN_TITLE")

        if has_input:
            self.engine.push_input(cmd)

    def run(self) -> None:
        clock = pygame.time.Clock()
        frame_idx = 0
        while self.running:
            dt = min(0.05, clock.tick(60) / 1000.0)
            self._process_events()
            self._update_transition(dt)

            if self.screen == "INTRO":
                self.intro_time += dt
                if self.intro_time >= self.hud.INTRO_TOTAL_DUR:
                    self._finish_intro()

            # Step native C++17 physics & procedural Douala world!
            self.engine.step(dt)
            snap = self.engine.snapshot()

            # Dispatch SFX events generated by C++ simulation
            events = self.engine.consume_sound_events()
            if events:
                self.audio.play_events(events)

            # Check transitions to Game Over / Level Complete
            prev_screen = self.screen
            if self.screen == "PLAYING":
                if snap.status == self.eng_mod.STATUS_GAME_OVER:
                    self.screen = "GAME_OVER"
                    if not self.recorded_run:
                        self.recorded_run = True
                        self.save.record_run(
                            self.current_level.level_id, snap.score, snap.coins,
                            snap.distance, completed=False, stumbles=snap.stumbles,
                        )
                elif snap.status == self.eng_mod.STATUS_LEVEL_COMPLETE:
                    self.screen = "LEVEL_COMPLETE"
                    if not self.recorded_run:
                        self.recorded_run = True
                        self.last_stars = self.save.record_run(
                            self.current_level.level_id, snap.score, snap.coins,
                            snap.distance, completed=True, stumbles=snap.stumbles,
                        )

            # Smoothly cross-fade atmosphere themes across Douala districts in Endless / Demo mode
            if self.current_level.is_endless or snap.status == self.eng_mod.STATUS_DEMO:
                target_theme = ZONE_THEMES.get(snap.endless_zone, ZONE_THEMES[1])
                self.active_theme = lerp_theme(self.active_theme, target_theme, min(1.0, dt * 1.6))
            else:
                self.active_theme = self.current_level.theme

            # Upload 2D HUD texture at 30 Hz during fast gameplay, 60 Hz during intro/menus/transitions
            update_hud = (
                (self.screen != "PLAYING")
                or (frame_idx % 2 == 0)
                or (prev_screen != self.screen)
                or (self._fade_alpha > 0.01)
            )
            hud_rgba = None
            if update_hud:
                mouse_pos = pygame.mouse.get_pos()
                hud_rgba = self.hud.render(
                    self.screen, snap, self.save, self.current_level,
                    mouse_pos, dt, last_stars=self.last_stars, fps=clock.get_fps(),
                    intro_time=self.intro_time, legal_state=self.legal_state,
                    fade_alpha=self._fade_alpha,
                )
            self.renderer.render_frame(self.engine, self.active_theme, dt, hud_rgba=hud_rgba, draw_hud=True)
            pygame.display.flip()
            frame_idx += 1

        self.audio.stop_music()
        self.renderer.release()
        pygame.quit()
