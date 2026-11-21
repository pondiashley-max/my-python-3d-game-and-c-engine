"""
interactive 2d hud, gta 4 style parallax intro & transitions, mandatory privacy & legal
acceptance gate, creator profile header, about us / programmer biography, and game menus
for douala run (maximus 3d).

all displayed text is bilingual english/french, strictly lowercase, and accent-free.
(c) 2024-2026 pondi ashley rey maxim - all rights reserved / tous droits reserves.
"""

from __future__ import annotations

import math
import os
from dataclasses import dataclass
from typing import List, Optional, Tuple

import pygame

from .legal_content import (
    ABOUT_CONCEPTION_PARAGRAPHS,
    COPYRIGHT_LINE,
    CREATOR_NAME,
    GAME_BRAND,
    LEGAL_DOCUMENT_SECTIONS,
    PRIVACY_POLICY_SECTIONS,
    PROGRAMMER_BIO_LINES,
    clean_ui_text,
    load_creator_avatar,
    load_gta4_portrait,
)
from .levels import LEVELS, OUTFITS, LevelDef, get_level
from .save_manager import SaveManager

# Color Palette (GTA IV Warm Amber/Noir + Douala Cameroon Emerald/Red/Gold)
COL_GLASS = (10, 14, 24, 228)
COL_GLASS_DARK = (8, 10, 18, 244)
COL_CARD = (16, 22, 36, 236)
COL_CARD_HOVER = (26, 36, 58, 248)
COL_BORDER = (82, 100, 130, 200)
COL_GOLD = (248, 196, 38)
COL_AMBER = (232, 146, 32)
COL_GREEN = (24, 192, 98)
COL_RED = (232, 58, 58)
COL_CYAN = (68, 208, 248)
COL_WHITE = (244, 246, 250)
COL_MUTED = (166, 178, 198)


@dataclass
class ButtonHit:
    rect: pygame.Rect
    action: str
    payload: object = None


@dataclass
class LegalUiState:
    active_tab: int = 0
    scroll_y: int = 0
    max_scroll: int = 0
    accept_privacy: bool = False
    accept_legal: bool = False
    review_only: bool = False


def _clamp_byte(v: float) -> int:
    return max(0, min(255, int(v)))


def _ease_out_cubic(t: float) -> float:
    t = max(0.0, min(1.0, t))
    return 1.0 - (1.0 - t) ** 3


def _draw_rounded(surf: pygame.Surface, rect: pygame.Rect, color, radius: int = 12,
                  border_color=None, border_w: int = 0) -> None:
    pygame.draw.rect(surf, color, rect, border_radius=radius)
    if border_color and border_w > 0:
        pygame.draw.rect(surf, border_color, rect, width=border_w, border_radius=radius)


def _draw_star(surf: pygame.Surface, cx: int, cy: int, r_outer: int, filled: bool) -> None:
    r_inner = r_outer * 0.45
    pts = []
    for i in range(10):
        ang = -math.pi / 2.0 + i * (math.pi / 5.0)
        r = r_outer if i % 2 == 0 else r_inner
        pts.append((cx + r * math.cos(ang), cy + r * math.sin(ang)))
    if filled:
        pygame.draw.polygon(surf, COL_GOLD, pts)
        pygame.draw.polygon(surf, (255, 240, 140), pts, 2)
    else:
        pygame.draw.polygon(surf, (44, 54, 72, 220), pts)
        pygame.draw.polygon(surf, (108, 122, 148), pts, 2)


def _draw_gta4_spinner(surf: pygame.Surface, cx: int, cy: int, radius: int, t: float, alpha: int = 255) -> None:
    """Iconic GTA IV rotating segmented loading ring."""
    a = _clamp_byte(alpha)
    if a <= 2:
        return
    segs = 10
    for i in range(segs):
        ang = t * 5.2 + i * (2.0 * math.pi / segs)
        px = cx + int(math.cos(ang) * radius)
        py = cy + int(math.sin(ang) * radius)
        frac = (i + 1) / float(segs)
        ca = _clamp_byte(a * (0.2 + 0.8 * frac))
        r_dot = max(2, int(2 + 2.2 * frac))
        dot_s = pygame.Surface((r_dot * 2 + 2, r_dot * 2 + 2), pygame.SRCALPHA)
        pygame.draw.circle(dot_s, (248, 196, 38, ca), (r_dot + 1, r_dot + 1), r_dot)
        surf.blit(dot_s, (px - r_dot - 1, py - r_dot - 1))


class HudManager:
    INTRO_PHASE1_END = 3.00   # gta 4 parallax "douala run" card
    INTRO_TOTAL_DUR = 6.80    # gta 4 parallax portrait "created by pondi ashley rey maxim"

    def __init__(self, width: int, height: int) -> None:
        pygame.font.init()
        self.width = width
        self.height = height
        self.surface = pygame.Surface((width, height), pygame.SRCALPHA)
        self.buttons: List[ButtonHit] = []
        self.time = 0.0
        self._init_fonts()

    @staticmethod
    def _load_font(size: int, bold: bool = False, mono: bool = False) -> pygame.font.Font:
        if mono:
            candidates = [
                "/System/Library/Fonts/Menlo.ttc",
                "/System/Library/Fonts/Supplemental/Courier New Bold.ttf",
            ]
        elif bold:
            candidates = [
                "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
                "/System/Library/Fonts/Helvetica.ttc",
            ]
        else:
            candidates = [
                "/System/Library/Fonts/Supplemental/Arial.ttf",
                "/System/Library/Fonts/Helvetica.ttc",
            ]
        for path in candidates:
            if os.path.exists(path):
                try:
                    f = pygame.font.Font(path, size)
                    if bold and path.endswith(".ttc"):
                        f.set_bold(True)
                    return f
                except Exception:
                    continue
        return pygame.font.Font(None, size + 2)

    def _init_fonts(self) -> None:
        self.f_mega = self._load_font(78, bold=True)
        self.f_hero = self._load_font(48, bold=True)
        self.f_title = self._load_font(30, bold=True)
        self.f_h2 = self._load_font(20, bold=True)
        self.f_card_h = self._load_font(15, bold=True)
        self.f_body = self._load_font(15, bold=False)
        self.f_body_b = self._load_font(15, bold=True)
        self.f_small = self._load_font(13, bold=False)
        self.f_small_b = self._load_font(13, bold=True)
        self.f_tiny = self._load_font(11, bold=True)
        self.f_score = self._load_font(28, bold=True, mono=True)
        self.f_mono = self._load_font(16, bold=True, mono=True)

    def resize(self, width: int, height: int) -> None:
        self.width = max(1, width)
        self.height = max(1, height)
        self.surface = pygame.Surface((self.width, self.height), pygame.SRCALPHA)

    def hit_test(self, pos: Tuple[int, int]) -> Optional[ButtonHit]:
        for b in reversed(self.buttons):
            if b.rect.collidepoint(pos):
                return b
        return None

    def _text(self, text: str, font: pygame.font.Font, color, pos: Tuple[int, int],
              center: bool = False, right: bool = False, shadow: bool = True,
              alpha: int = 255) -> pygame.Rect:
        # Strict enforcement: all rendered text is lowercase and accent-free!
        clean = clean_ui_text(text)
        a = _clamp_byte(alpha)
        rendered = font.render(clean, True, color)
        if a < 255:
            rendered.set_alpha(a)
        rect = rendered.get_rect()
        if center:
            rect.center = pos
        elif right:
            rect.topright = pos
        else:
            rect.topleft = pos
        if shadow:
            sh = font.render(clean, True, (0, 0, 0))
            if a < 255:
                sh.set_alpha(a)
            self.surface.blit(sh, (rect.x + 2, rect.y + 2))
        self.surface.blit(rendered, rect)
        return rect

    def _button(self, rect: pygame.Rect, label: str, action: str, mouse_pos: Tuple[int, int],
                payload: object = None, primary: bool = False, accent=COL_GREEN,
                disabled: bool = False, font: Optional[pygame.font.Font] = None) -> None:
        hovered = rect.collidepoint(mouse_pos) and not disabled
        if disabled:
            bg = (30, 36, 48, 200)
            border = (64, 72, 88, 175)
            tcol = (128, 138, 156)
        elif primary:
            c = tuple(min(255, int(v * (1.14 if hovered else 1.0))) for v in accent)
            bg = (*c, 245)
            border = (255, 255, 255, 230) if hovered else (255, 255, 255, 115)
            tcol = (10, 16, 22) if sum(accent) > 420 else COL_WHITE
        else:
            bg = COL_CARD_HOVER if hovered else COL_CARD
            border = COL_GOLD if hovered else COL_BORDER
            tcol = COL_WHITE

        _draw_rounded(self.surface, rect, bg, radius=11, border_color=border, border_w=2)
        self._text(label, font or self.f_body_b, tcol, rect.center, center=True, shadow=not primary)
        if not disabled:
            self.buttons.append(ButtonHit(rect=rect, action=action, payload=payload))

    def _draw_cameroon_ribbon(self, x: int, y: int, w: int, h: int, alpha: int = 255) -> None:
        if w <= 2:
            return
        a = _clamp_byte(alpha)
        t = w // 3
        rib = pygame.Surface((w, h), pygame.SRCALPHA)
        pygame.draw.rect(rib, (14, 158, 76, a), (0, 0, t, h))
        pygame.draw.rect(rib, (218, 36, 42, a), (t, 0, t, h))
        pygame.draw.rect(rib, (252, 208, 24, a), (t * 2, 0, w - t * 2, h))
        self.surface.blit(rib, (x, y))

    def _draw_game_head_bar(self, mouse_pos: Tuple[int, int], show_about_btn: bool = True) -> None:
        """
        creator profile header bar at the head of the game displaying
        pondi ashley rey maxim's circular profile picture, brand, and copyright.
        """
        w = self.width
        bar_h = 56
        bar = pygame.Rect(16, 10, w - 32, bar_h)
        _draw_rounded(self.surface, bar, (10, 14, 24, 228), radius=14,
                      border_color=(248, 196, 38, 165), border_w=2)

        avatar = load_creator_avatar(44, ring_width=2)
        tx = bar.x + 14
        if avatar is not None:
            self.surface.blit(avatar, (bar.x + 10, bar.y + 6))
            tx = bar.x + 64

        self._text(f"{GAME_BRAND} / maximus 3d", self.f_body_b, COL_GOLD, (tx, bar.y + 8))
        self._text(f"created by / cree par {CREATOR_NAME} | all rights reserved", self.f_small, COL_WHITE,
                   (tx, bar.y + 30))

        if show_about_btn:
            about_r = pygame.Rect(bar.right - 294, bar.y + 11, 134, 34)
            legal_r = pygame.Rect(bar.right - 150, bar.y + 11, 140, 34)
            self._button(about_r, "about us / a propos", "OPEN_ABOUT", mouse_pos, font=self.f_tiny)
            self._button(legal_r, "legal / confidentialite", "OPEN_LEGAL", mouse_pos, font=self.f_tiny)

    def render(self, screen: str, snap, save: SaveManager, current_level: LevelDef,
               mouse_pos: Tuple[int, int], dt: float, last_stars: int = 0,
               fps: float = 60.0, intro_time: float = 0.0,
               legal_state: Optional[LegalUiState] = None,
               fade_alpha: float = 0.0) -> bytes:
        self.time += dt
        self.buttons.clear()
        self.surface.fill((0, 0, 0, 0))

        if screen == "INTRO":
            self._draw_intro_gta4(intro_time)
        elif screen == "LEGAL_GATE":
            self._draw_legal_gate(legal_state or LegalUiState(), mouse_pos)
        elif screen == "TITLE":
            self._draw_title(save, mouse_pos)
        elif screen == "ABOUT_US":
            self._draw_about_us(mouse_pos)
        elif screen == "LEVEL_SELECT":
            self._draw_level_select(save, mouse_pos)
        elif screen == "SHOP":
            self._draw_shop(save, mouse_pos)
        elif screen == "CONTROLS":
            self._draw_controls(mouse_pos)
        elif screen == "PLAYING":
            self._draw_playing_hud(snap, current_level, save, fps)
        elif screen == "PAUSED":
            self._draw_playing_hud(snap, current_level, save, fps)
            self._draw_pause_overlay(current_level, mouse_pos)
        elif screen == "GAME_OVER":
            self._draw_playing_hud(snap, current_level, save, fps)
            self._draw_game_over(snap, current_level, save, mouse_pos)
        elif screen == "LEVEL_COMPLETE":
            self._draw_playing_hud(snap, current_level, save, fps)
            self._draw_level_complete(snap, current_level, save, mouse_pos, last_stars)

        # GTA IV style diagonal noir-gold shutter & spinner screen transition!
        if fade_alpha > 0.01:
            self._draw_gta4_transition(fade_alpha)

        return pygame.image.tobytes(self.surface, "RGBA")

    # -----------------------------------------------------------------------
    # 1. GTA IV Style Parallax Intro & Diagonal Shutter Transitions
    # -----------------------------------------------------------------------

    def _draw_gta4_transition(self, fade_alpha: float) -> None:
        w, h = self.width, self.height
        f = max(0.0, min(1.0, fade_alpha))
        veil = pygame.Surface((w, h), pygame.SRCALPHA)
        veil.fill((6, 8, 14, _clamp_byte(f * 238)))
        # Angled GTA IV golden accent slash sweeping diagonally
        sweep = int((f - 0.5) * w * 1.4)
        pts = [
            (w // 2 - 140 + sweep, 0),
            (w // 2 - 90 + sweep, 0),
            (w // 2 - 220 + sweep, h),
            (w // 2 - 270 + sweep, h),
        ]
        pygame.draw.polygon(veil, (248, 196, 38, _clamp_byte(f * 130)), pts)
        self.surface.blit(veil, (0, 0))
        _draw_gta4_spinner(self.surface, w - 42, h - 38, 14, self.time, _clamp_byte(f * 255))

    def _draw_intro_gta4(self, t: float) -> None:
        """
        Iconic GTA IV loading / title animation style:
        - slow 2.5d parallax drift & dark noir letterbox + angled geometric frame splits
        - phase 1: "douala run" drifting in warm amber/noir parallax
        - diagonal shutter slash transition
        - phase 2: gta 4 illustrated portrait panel of pondi ashley rey maxim gliding in parallax
          beside staggered slide-in lowercase bilingual credits + rotating loading spinner
        """
        w, h = self.width, self.height
        p1_end = self.INTRO_PHASE1_END
        total = self.INTRO_TOTAL_DUR
        t_raw = max(0.0, t)
        t = min(total, t_raw)

        # Warm-noir semi-opaque backdrop with GTA IV gritty cinematic contrast
        bg = pygame.Surface((w, h), pygame.SRCALPHA)
        bg.fill((8, 9, 14, 212))
        self.surface.blit(bg, (0, 0))

        # Top & bottom GTA IV thick black cinema letterbox bars with hairline gold trim
        bar_h = 54
        pygame.draw.rect(self.surface, (5, 6, 10, 250), (0, 0, w, bar_h))
        pygame.draw.rect(self.surface, (5, 6, 10, 250), (0, h - bar_h, w, bar_h))
        pygame.draw.line(self.surface, (248, 196, 38, 180), (0, bar_h), (w, bar_h), 2)
        pygame.draw.line(self.surface, (248, 196, 38, 180), (0, h - bar_h), (w, h - bar_h), 2)

        # Iconic GTA IV angled background dark-amber geometric panel slowly drifting in parallax
        drift_bg = int(t_raw * 18.0)
        poly_bg = [
            (int(w * 0.56) - drift_bg, bar_h),
            (int(w * 0.92) - drift_bg, bar_h),
            (int(w * 0.78) - drift_bg, h - bar_h),
            (int(w * 0.42) - drift_bg, h - bar_h),
        ]
        panel_s = pygame.Surface((w, h), pygame.SRCALPHA)
        pygame.draw.polygon(panel_s, (24, 20, 14, 150), poly_bg)
        pygame.draw.lines(panel_s, (248, 196, 38, 90), False, [poly_bg[0], poly_bg[3]], 3)
        self.surface.blit(panel_s, (0, 0))

        if t < p1_end:
            # --- PHASE 1: GTA IV "douala run" slow Ken Burns parallax slide ---
            enter = _ease_out_cubic(t / 0.78)
            exit_p = max(0.0, min(1.0, (t - (p1_end - 0.55)) / 0.55))
            alpha = _clamp_byte(255 * enter * (1.0 - exit_p))

            # Opposite-direction slow parallax drift (classic GTA IV kinetic layering!)
            px_main = int(-34 + t * 22.0)
            px_sub = int(34 - t * 18.0)

            # Dark editorial angled title plate behind "douala run"
            plate = pygame.Surface((w, h), pygame.SRCALPHA)
            plate_pts = [
                (w // 2 - 330 + px_main // 2, h // 2 - 108),
                (w // 2 + 360 + px_main // 2, h // 2 - 108),
                (w // 2 + 310 + px_main // 2, h // 2 + 104),
                (w // 2 - 380 + px_main // 2, h // 2 + 104),
            ]
            pygame.draw.polygon(plate, (6, 8, 14, _clamp_byte(215 * enter * (1.0 - exit_p))), plate_pts)
            pygame.draw.polygon(plate, (248, 196, 38, _clamp_byte(210 * enter * (1.0 - exit_p))), plate_pts, 3)
            self.surface.blit(plate, (0, 0))

            rib_w = int(360 * enter)
            self._draw_cameroon_ribbon(w // 2 - rib_w // 2 + px_main // 2, h // 2 - 84, rib_w, 6, alpha)

            self._text(GAME_BRAND, self.f_mega, COL_GOLD, (w // 2 + px_main, h // 2 - 18),
                       center=True, alpha=alpha)
            self._text("maximus 3d / douala cameroun - 237 edition", self.f_h2, COL_WHITE,
                       (w // 2 + px_sub, h // 2 + 48), center=True, alpha=alpha)
            self._text("c++17 + python opengl / realistic street runner", self.f_small_b, COL_AMBER,
                       (w // 2 + px_sub, h // 2 + 78), center=True, alpha=alpha)

            # Diagonal amber-noir shutter wipe between Phase 1 and Phase 2
            if exit_p > 0.05:
                sh_w = int(w * 1.35 * _ease_out_cubic(exit_p))
                sh_s = pygame.Surface((w, h), pygame.SRCALPHA)
                sh_pts = [(0, bar_h), (sh_w, bar_h), (sh_w - 180, h - bar_h), (-180, h - bar_h)]
                pygame.draw.polygon(sh_s, (10, 10, 16, _clamp_byte(185 * exit_p)), sh_pts)
                pygame.draw.line(sh_s, (248, 196, 38, _clamp_byte(240 * exit_p)),
                                 (sh_w, bar_h), (sh_w - 180, h - bar_h), 5)
                self.surface.blit(sh_s, (0, 0))

            status_txt = "loading douala... / chargement douala..."
        else:
            # --- PHASE 2: GTA IV Character-Art Split ("created by pondi ashley rey maxim") ---
            t2 = max(0.0, t - p1_end)
            dur2 = max(0.1, total - p1_end)
            enter = _ease_out_cubic(t2 / 0.85)
            exit_p = max(0.0, min(1.0, (t2 - (dur2 - 0.65)) / 0.65))
            alpha = _clamp_byte(255 * enter * (1.0 - exit_p))

            # Left foreground: GTA IV illustrated portrait sliding slowly right & up (Ken Burns!)
            art_w, art_h = 310, 388
            art_surf = load_gta4_portrait(art_w, art_h)
            art_x = int(w * 0.13 + (1.0 - enter) * -70 + t2 * 14.0)
            art_y = int((h - art_h) // 2 + (1.0 - enter) * 24 - t2 * 5.0)

            # Offset dark-amber GTA IV graphic drop card behind portrait
            drop = pygame.Surface((art_w + 18, art_h + 18), pygame.SRCALPHA)
            pygame.draw.rect(drop, (248, 196, 38, _clamp_byte(165 * enter * (1.0 - exit_p))),
                             (0, 0, art_w + 18, art_h + 18))
            self.surface.blit(drop, (art_x - 10, art_y + 10))

            if art_surf is not None:
                acopy = art_surf.copy()
                acopy.set_alpha(alpha)
                self.surface.blit(acopy, (art_x, art_y))

            # Right side: staggered GTA IV bold lowercase typography sliding in opposite parallax!
            tx = int(w * 0.44 + (1.0 - enter) * 65 - t2 * 8.0)
            ty = h // 2 - 122

            # Subtle dark graphic backing strip behind credits for ultra-crisp legibility
            c_plate = pygame.Surface((int(w * 0.50), 270), pygame.SRCALPHA)
            pygame.draw.rect(c_plate, (8, 10, 16, _clamp_byte(195 * enter * (1.0 - exit_p))),
                             (0, 0, c_plate.get_width(), 270), border_radius=6)
            pygame.draw.rect(c_plate, (248, 196, 38, _clamp_byte(190 * enter * (1.0 - exit_p))),
                             (0, 0, 5, 270))
            self.surface.blit(c_plate, (tx - 18, ty - 18))

            self._text("created by / cree par", self.f_body_b, COL_CYAN, (tx, ty), alpha=alpha)
            self._text(CREATOR_NAME, self.f_hero, COL_GOLD, (tx, ty + 28), alpha=alpha)

            rib_w = int(340 * enter)
            self._draw_cameroon_ribbon(tx, ty + 90, rib_w, 5, alpha)

            self._text("conceived in solo since year 1 (2024)", self.f_body_b, COL_WHITE,
                       (tx, ty + 114), alpha=alpha)
            self._text("concu en solo par passion de la programmation", self.f_body, (218, 224, 236),
                       (tx, ty + 140), alpha=alpha)
            self._text("douala run / maximus 3d - cameroon", self.f_small_b, COL_AMBER,
                       (tx, ty + 174), alpha=alpha)
            self._text(COPYRIGHT_LINE, self.f_small, COL_MUTED, (tx, ty + 206), alpha=alpha)

            status_txt = "entering douala run... / entree dans le jeu..."

        # Top & bottom GTA IV status chrome
        self._text(f"{GAME_BRAND} / maximus 3d", self.f_small_b, COL_GOLD, (24, 18))
        self._text(COPYRIGHT_LINE, self.f_tiny, COL_MUTED, (w - 24, 20), right=True, shadow=False)

        prog = max(0.0, min(1.0, t / total))
        pygame.draw.rect(self.surface, (36, 42, 56, 210), (24, h - 30, 280, 5), border_radius=2)
        pygame.draw.rect(self.surface, COL_GOLD, (24, h - 30, int(280 * prog), 5), border_radius=2)
        self._text("click or press space / enter to skip - passer", self.f_tiny, COL_MUTED, (24, h - 46), shadow=False)

        # Bottom-right GTA IV spinning loader + status label
        self._text(status_txt, self.f_small_b, COL_WHITE, (w - 64, h - 35), right=True)
        _draw_gta4_spinner(self.surface, w - 34, h - 27, 13, t_raw, 240)

    # -----------------------------------------------------------------------
    # 2. Politique de Confidentialite & Document Juridique Gate
    # -----------------------------------------------------------------------

    def _draw_legal_gate(self, state: LegalUiState, mouse_pos: Tuple[int, int]) -> None:
        w, h = self.width, self.height
        ov = pygame.Surface((w, h), pygame.SRCALPHA)
        ov.fill((6, 10, 18, 218))
        self.surface.blit(ov, (0, 0))

        box_w = min(940, w - 52)
        box_h = min(656, h - 36)
        box = pygame.Rect((w - box_w) // 2, (h - box_h) // 2, box_w, box_h)
        _draw_rounded(self.surface, box, COL_GLASS_DARK, radius=18,
                      border_color=COL_GOLD, border_w=2)
        self._draw_cameroon_ribbon(box.x + 22, box.y + 14, box.w - 44, 5)

        avatar = load_creator_avatar(48, ring_width=2)
        hx = box.x + 24
        if avatar is not None:
            self.surface.blit(avatar, (box.x + 22, box.y + 28))
            hx = box.x + 82

        self._text("politique de confidentialite & document juridique / privacy & legal",
                   self.f_h2, COL_GOLD, (hx, box.y + 30))
        sub = (
            "official legal notice / consultation officielle - douala run"
            if state.review_only
            else "please read & accept before entering / lire et accepter avant d'entrer"
        )
        self._text(sub, self.f_small, COL_WHITE, (hx, box.y + 56))
        self._text("scroll / molette ou fleches up-down", self.f_tiny, COL_CYAN,
                   (box.right - 26, box.y + 58), right=True, shadow=False)

        tab_y = box.y + 88
        tab_w = (box.w - 60) // 2
        self._button(
            pygame.Rect(box.x + 24, tab_y, tab_w, 36),
            "1. politique de confidentialite / privacy policy",
            "LEGAL_TAB", mouse_pos, payload=0,
            primary=(state.active_tab == 0), accent=COL_GOLD, font=self.f_small_b,
        )
        self._button(
            pygame.Rect(box.x + 36 + tab_w, tab_y, tab_w, 36),
            "2. document juridique & licence / legal eula",
            "LEGAL_TAB", mouse_pos, payload=1,
            primary=(state.active_tab == 1), accent=COL_CYAN, font=self.f_small_b,
        )

        doc_h = box.h - (312 if not state.review_only else 224)
        doc_rect = pygame.Rect(box.x + 24, tab_y + 44, box.w - 48, max(160, doc_h))
        _draw_rounded(self.surface, doc_rect, (14, 18, 30, 240), radius=12,
                      border_color=COL_BORDER, border_w=1)

        sections = PRIVACY_POLICY_SECTIONS if state.active_tab == 0 else LEGAL_DOCUMENT_SECTIONS
        inner_w = doc_rect.w - 42
        lines_to_draw: List[Tuple[str, pygame.font.Font, Tuple[int, int, int], int]] = []
        for heading, body in sections:
            lines_to_draw.append((heading, self.f_body_b, COL_GOLD, 6))
            for wrapped in self._wrap_lines(body, self.f_body, inner_w):
                lines_to_draw.append((wrapped, self.f_body, (232, 238, 248), 2))
            lines_to_draw.append(("", self.f_small, COL_WHITE, 10))

        total_content_h = 18
        for txt, fnt, _col, pad in lines_to_draw:
            total_content_h += (fnt.get_linesize() if txt else 0) + pad

        state.max_scroll = max(0, total_content_h - doc_rect.h + 16)
        state.scroll_y = max(0, min(state.max_scroll, state.scroll_y))

        prev_clip = self.surface.get_clip()
        self.surface.set_clip(doc_rect.inflate(-8, -8))
        cy = doc_rect.y + 12 - state.scroll_y
        for txt, fnt, col, pad in lines_to_draw:
            if txt:
                if doc_rect.y - 30 <= cy <= doc_rect.bottom + 10:
                    self._text(txt, fnt, col, (doc_rect.x + 16, cy), shadow=False)
                cy += fnt.get_linesize()
            cy += pad
        self.surface.set_clip(prev_clip)

        sb_x = doc_rect.right - 14
        pygame.draw.rect(self.surface, (30, 40, 60), (sb_x, doc_rect.y + 10, 6, doc_rect.h - 20), border_radius=3)
        if state.max_scroll > 0:
            thumb_h = max(36, int((doc_rect.h - 20) * (doc_rect.h / max(1, total_content_h))))
            frac = state.scroll_y / max(1, state.max_scroll)
            thumb_y = doc_rect.y + 10 + int((doc_rect.h - 20 - thumb_h) * frac)
            pygame.draw.rect(self.surface, COL_GOLD, (sb_x, thumb_y, 6, thumb_h), border_radius=3)

        cur_y = doc_rect.bottom + 12
        if not state.review_only:
            self._draw_checkbox(
                pygame.Rect(box.x + 24, cur_y, box.w - 48, 34),
                state.accept_privacy,
                "i read & accept privacy policy / j'accepte la politique de confidentialite",
                "TOGGLE_PRIVACY", mouse_pos,
            )
            self._draw_checkbox(
                pygame.Rect(box.x + 24, cur_y + 40, box.w - 48, 34),
                state.accept_legal,
                "i accept legal document & terms / j'accepte le document juridique - all rights reserved",
                "TOGGLE_LEGAL", mouse_pos,
            )

            can_enter = state.accept_privacy and state.accept_legal
            btn_y = cur_y + 82
            self._button(
                pygame.Rect(box.x + 24, btn_y, 240, 42),
                "decline & quit / refuser",
                "DECLINE_LEGAL", mouse_pos, primary=True, accent=(150, 40, 46), font=self.f_small_b,
            )
            self._button(
                pygame.Rect(box.x + 280, btn_y, box.w - 304, 42),
                "accept & enter douala run / accepter et entrer" if can_enter else "check both boxes / cochez les 2 cases",
                "ACCEPT_LEGAL", mouse_pos, primary=can_enter, accent=COL_GREEN, disabled=not can_enter,
            )
        else:
            btn_y = cur_y + 12
            self._button(
                pygame.Rect(box.centerx - 140, btn_y, 280, 42),
                "close / fermer et retour menu",
                "OPEN_TITLE", mouse_pos, primary=True, accent=COL_GREEN,
            )

        self._text(COPYRIGHT_LINE, self.f_tiny, COL_MUTED, (box.centerx, box.bottom - 18),
                   center=True, shadow=False)

    def _draw_checkbox(self, rect: pygame.Rect, checked: bool, label: str, action: str,
                       mouse_pos: Tuple[int, int]) -> None:
        hovered = rect.collidepoint(mouse_pos)
        bg = (22, 46, 38, 230) if checked else ((24, 34, 54, 225) if hovered else (16, 24, 40, 215))
        border = COL_GREEN if checked else (COL_GOLD if hovered else COL_BORDER)
        _draw_rounded(self.surface, rect, bg, radius=9, border_color=border, border_w=2 if checked else 1)

        cb = pygame.Rect(rect.x + 10, rect.y + 6, 22, 22)
        _draw_rounded(self.surface, cb, COL_GREEN if checked else (28, 38, 56), radius=5,
                      border_color=COL_WHITE if checked else COL_MUTED, border_w=2)
        if checked:
            pygame.draw.lines(self.surface, (10, 18, 16), False,
                              [(cb.x + 4, cb.y + 11), (cb.x + 9, cb.y + 16), (cb.x + 17, cb.y + 6)], 3)

        self._text(label, self.f_small_b, COL_WHITE if checked else (220, 228, 242),
                   (cb.right + 12, rect.y + 9), shadow=False)
        self.buttons.append(ButtonHit(rect=rect, action=action))

    # -----------------------------------------------------------------------
    # 3. Title Screen (with Creator Profile Picture at the HEAD of the game!)
    # -----------------------------------------------------------------------

    def _draw_title(self, save: SaveManager, mouse_pos: Tuple[int, int]) -> None:
        w, h = self.width, self.height

        self._draw_game_head_bar(mouse_pos, show_about_btn=True)

        panel_w = min(580, w - 60)
        panel_h = 520
        panel = pygame.Rect((w - panel_w) // 2, 82, panel_w, min(panel_h, h - 116))
        _draw_rounded(self.surface, panel, (10, 14, 24, 216), radius=20,
                      border_color=(248, 196, 38, 175), border_w=2)

        self._draw_cameroon_ribbon(panel.x + 24, panel.y + 16, panel.w - 48, 6)

        self._text(GAME_BRAND, self.f_hero, COL_GOLD, (panel.centerx, panel.y + 58), center=True)
        self._text("maximus 3d / douala cameroon (237)", self.f_body_b, COL_WHITE,
                   (panel.centerx, panel.y + 98), center=True)
        self._text(f"created by / cree par {CREATOR_NAME} - c++17 + python", self.f_small, COL_CYAN,
                   (panel.centerx, panel.y + 122), center=True)

        total_stars = sum(save.get_stars(i) for i in range(1, 6))
        info_pill = pygame.Rect(panel.x + 36, panel.y + 144, panel.w - 72, 34)
        _draw_rounded(self.surface, info_pill, (20, 30, 48, 226), radius=10,
                      border_color=(72, 100, 136, 160), border_w=1)
        self._text(
            f"fcfa bank: {save.data['total_coins']}  |  stars / etoiles: {total_stars}/15  |  boards: {save.data['hoverboard_stock']}",
            self.f_small_b, COL_GOLD, info_pill.center, center=True, shadow=False,
        )

        bw = panel.w - 76
        bx = panel.centerx - bw // 2
        by = panel.y + 192
        self._button(pygame.Rect(bx, by, bw, 50), "play douala run / jouer maintenant", "QUICK_PLAY",
                     mouse_pos, primary=True, accent=COL_GREEN)
        self._button(pygame.Rect(bx, by + 60, bw, 44), "level select / choisir quartier (5 + infini)",
                     "OPEN_LEVELS", mouse_pos)

        half_w = (bw - 12) // 2
        self._button(pygame.Rect(bx, by + 114, half_w, 42), "garage & shop / boutique", "OPEN_SHOP", mouse_pos, font=self.f_small_b)
        self._button(pygame.Rect(bx + half_w + 12, by + 114, half_w, 42), "controls / commandes", "OPEN_CONTROLS", mouse_pos, font=self.f_small_b)

        self._button(pygame.Rect(bx, by + 166, half_w, 42), "about us / a propos", "OPEN_ABOUT",
                     mouse_pos, primary=True, accent=COL_GOLD, font=self.f_small_b)
        self._button(pygame.Rect(bx + half_w + 12, by + 166, half_w, 42), "politique & juridique",
                     "OPEN_LEGAL", mouse_pos, font=self.f_small_b)

        self._button(pygame.Rect(bx, by + 218, bw, 38), "quit game / quitter", "QUIT", mouse_pos)

        self._text(COPYRIGHT_LINE, self.f_tiny, COL_MUTED,
                   (panel.centerx, panel.bottom - 18), center=True, shadow=False)

        self._text("arrows / wasd: move, jump, roll / bouger, sauter, rouler | space/e: shield | m: mute | f: fullscreen",
                   self.f_small, (230, 238, 250), (w // 2, h - 16), center=True)

    # -----------------------------------------------------------------------
    # 4. About Us & Programmer Biography (with Profile Picture!)
    # -----------------------------------------------------------------------

    def _draw_about_us(self, mouse_pos: Tuple[int, int]) -> None:
        w, h = self.width, self.height
        ov = pygame.Surface((w, h), pygame.SRCALPHA)
        ov.fill((7, 11, 20, 210))
        self.surface.blit(ov, (0, 0))

        self._draw_game_head_bar(mouse_pos, show_about_btn=False)

        margin = max(36, int(w * 0.045))
        top_y = 82
        avail_h = h - top_y - 76
        gap = 24
        left_w = int((w - 2 * margin - gap) * 0.54)
        right_w = (w - 2 * margin - gap) - left_w

        left = pygame.Rect(margin, top_y, left_w, avail_h)
        _draw_rounded(self.surface, left, COL_GLASS_DARK, radius=18,
                      border_color=COL_GOLD, border_w=2)
        self._draw_cameroon_ribbon(left.x + 18, left.y + 14, left.w - 36, 5)
        self._text("about us / a propos - game conception", self.f_h2, COL_GOLD, (left.x + 22, left.y + 30))
        self._text("conceived since year 1 (2024) in solo passion / concu depuis annee 1 (2024)",
                   self.f_small_b, COL_CYAN, (left.x + 22, left.y + 56))

        cy = left.y + 86
        for para in ABOUT_CONCEPTION_PARAGRAPHS:
            for line in self._wrap_lines(para, self.f_body, left.w - 44):
                self._text(line, self.f_body, COL_WHITE, (left.x + 22, cy), shadow=False)
                cy += self.f_body.get_linesize() + 1
            cy += 10

        milestones = [
            ("year 1 (2024)", "origin / origine", "initial conception of douala run born from solo passion of programming."),
            ("c++17 & python", "solo engine / moteur", "realistic 2-bone kinematics, douala pnj, 39 meshes, opengl 3.3 & makossa audio."),
            ("douala 237", "5 districts / quartiers", "akwa, marche mboppi, rond-point deido, pont du wouri & bonanjo night rain."),
        ]
        ty = max(cy + 4, left.bottom - 208)
        for tag, title, desc in milestones:
            card = pygame.Rect(left.x + 18, ty, left.w - 36, 58)
            if card.bottom > left.bottom - 12:
                break
            _draw_rounded(self.surface, card, (20, 28, 44, 225), radius=10,
                          border_color=(76, 98, 134, 175), border_w=1)
            pill = pygame.Rect(card.x + 10, card.y + 10, 126, 22)
            _draw_rounded(self.surface, pill, (24, 156, 86, 225), radius=6)
            self._text(tag, self.f_tiny, COL_WHITE, pill.center, center=True, shadow=False)
            self._text(title, self.f_small_b, COL_GOLD, (pill.right + 12, card.y + 12), shadow=False)
            self._text(desc, self.f_small, (216, 224, 238), (card.x + 12, card.y + 36), shadow=False)
            ty += 66

        # Right panel: Biography of the Programmer + Profile Picture!
        right = pygame.Rect(left.right + gap, top_y, right_w, avail_h)
        _draw_rounded(self.surface, right, COL_GLASS_DARK, radius=18,
                      border_color=COL_GREEN, border_w=2)
        self._text("biography of the programmer / biographie", self.f_h2, COL_GREEN,
                   (right.centerx, right.y + 26), center=True)

        avatar = load_creator_avatar(140, ring_width=5)
        av_y = right.y + 52
        if avatar is not None:
            self.surface.blit(avatar, (right.centerx - 70, av_y))

        name_y = av_y + 154
        self._text(CREATOR_NAME, self.f_h2, COL_GOLD, (right.centerx, name_y), center=True)
        role_pill = pygame.Rect(right.centerx - 166, name_y + 20, 332, 26)
        _draw_rounded(self.surface, role_pill, (20, 44, 62, 230), radius=8,
                      border_color=COL_CYAN, border_w=1)
        self._text("solo creator & programmer / createur solo - year 1 (2024)", self.f_tiny, COL_CYAN,
                   role_pill.center, center=True, shadow=False)

        by = role_pill.bottom + 14
        for item in PROGRAMMER_BIO_LINES:
            for line in self._wrap_lines(item, self.f_small, right.w - 40):
                if by > right.bottom - 52:
                    break
                self._text(line, self.f_small, COL_WHITE, (right.x + 20, by), shadow=False)
                by += self.f_small.get_linesize() + 1
            by += 7

        seal = pygame.Rect(right.x + 16, right.bottom - 44, right.w - 32, 30)
        _draw_rounded(self.surface, seal, (24, 28, 42, 230), radius=8,
                      border_color=(248, 196, 38, 150), border_w=1)
        self._text("(c) 2024-2026 pondi ashley rey maxim - all rights reserved",
                   self.f_tiny, COL_GOLD, seal.center, center=True, shadow=False)

        self._button(pygame.Rect(margin, h - 58, 185, 42), "back / retour menu", "OPEN_TITLE", mouse_pos, font=self.f_small_b)
        self._button(pygame.Rect(w // 2 - 175, h - 58, 350, 42),
                     "politique de confidentialite & juridique", "OPEN_LEGAL", mouse_pos, font=self.f_small_b)
        self._button(pygame.Rect(w - margin - 225, h - 58, 225, 42), "replay gta 4 intro / rejouer",
                     "REPLAY_INTRO", mouse_pos, primary=True, accent=COL_GOLD, font=self.f_small_b)

    # -----------------------------------------------------------------------
    # 5. Level Select, Shop, Controls, In-Game HUD & Overlays
    # -----------------------------------------------------------------------

    def _draw_level_select(self, save: SaveManager, mouse_pos: Tuple[int, int]) -> None:
        w, h = self.width, self.height
        overlay = pygame.Surface((w, h), pygame.SRCALPHA)
        overlay.fill((8, 12, 22, 190))
        self.surface.blit(overlay, (0, 0))

        self._text("select douala district / choisir quartier", self.f_title, COL_GOLD, (w // 2, 34), center=True)
        self._text("progress across douala as speed & traffic intensify / vitesse et trafic croissants",
                   self.f_body, COL_WHITE, (w // 2, 66), center=True)

        cols, rows = 3, 2
        margin_x = max(32, int(w * 0.05))
        gap_x, gap_y = 20, 18
        top_y = 96
        avail_h = h - top_y - 84
        card_w = (w - 2 * margin_x - (cols - 1) * gap_x) // cols
        card_h = min(248, (avail_h - gap_y) // rows)

        for idx, lvl in enumerate(LEVELS):
            r = idx // cols
            c = idx % cols
            rect = pygame.Rect(margin_x + c * (card_w + gap_x),
                               top_y + r * (card_h + gap_y), card_w, card_h)
            unlocked = save.is_level_unlocked(lvl.level_id)
            hovered = rect.collidepoint(mouse_pos) and unlocked

            bg = COL_CARD_HOVER if hovered else (COL_CARD if unlocked else (18, 22, 32, 214))
            border = lvl.accent_rgb if hovered else ((*lvl.accent_rgb, 185) if unlocked else (60, 68, 82, 165))
            _draw_rounded(self.surface, rect, bg, radius=16, border_color=border, border_w=2)

            head = pygame.Rect(rect.x + 2, rect.y + 2, rect.w - 4, 38)
            pygame.draw.rect(self.surface, (*lvl.accent_rgb, 220 if unlocked else 108),
                             head, border_top_left_radius=14, border_top_right_radius=14)
            num_txt = "inf" if lvl.is_endless else f"l{lvl.level_id}"
            self._text(f"{num_txt} / {lvl.name}", self.f_card_h, COL_WHITE,
                       (rect.x + 12, rect.y + 11), shadow=True)

            pill = pygame.Rect(rect.right - 106, rect.y + 8, 96, 24)
            _draw_rounded(self.surface, pill, (12, 18, 28, 224), radius=8)
            self._text(lvl.difficulty_label, self.f_tiny, COL_GOLD, pill.center, center=True, shadow=False)

            if unlocked:
                self.buttons.append(ButtonHit(rect=rect, action="START_LEVEL", payload=lvl.level_id))

            self._text(lvl.district_fr, self.f_small_b, COL_CYAN, (rect.x + 14, rect.y + 48))
            self._draw_wrapped(lvl.subtitle, self.f_small, COL_MUTED, rect.x + 14, rect.y + 68, rect.w - 28)

            bar_y = rect.y + 118
            self._text("difficulty / niv:", self.f_small_b, COL_MUTED, (rect.x + 14, bar_y - 2))
            bars = 5 if lvl.is_endless else lvl.difficulty
            for b_i in range(5):
                bx = rect.x + 122 + b_i * 22
                bcol = lvl.accent_rgb if b_i < bars else (48, 58, 76)
                pygame.draw.rect(self.surface, bcol, (bx, bar_y, 17, 10), border_radius=3)

            dist_str = "endless / infini" if lvl.is_endless else f"{int(lvl.target_distance)}m"
            spd_str = f"{int(lvl.base_speed * 3.6)}-{int(lvl.max_speed * 3.6)} km/h"
            self._text(f"goal / but: {dist_str}  |  speed: {spd_str}", self.f_small, COL_WHITE,
                       (rect.x + 14, bar_y + 20))

            if not lvl.is_endless:
                stars = save.get_stars(lvl.level_id)
                for s_i in range(3):
                    _draw_star(self.surface, rect.x + 26 + s_i * 26, rect.bottom - 52, 10, s_i < stars)
                best = save.get_best_score(lvl.level_id)
                self._text(f"best / record: {best:,}", self.f_small_b, COL_GOLD,
                           (rect.right - 14, rect.bottom - 60), right=True)
            else:
                best_d = save.data.get("best_endless_dist", 0.0)
                best_s = save.get_best_score(6)
                self._text(f"best dist: {int(best_d)}m | score: {best_s:,}", self.f_small_b,
                           COL_GOLD, (rect.x + 14, rect.bottom - 60))

            btn_rect = pygame.Rect(rect.x + 14, rect.bottom - 38, rect.w - 28, 28)
            if unlocked:
                self._button(btn_rect, "run district / lancer course", "START_LEVEL", mouse_pos,
                             payload=lvl.level_id, primary=True, accent=lvl.accent_rgb, font=self.f_small_b)
            else:
                self._button(btn_rect, f"clear level {lvl.level_id - 1} / terminer niv {lvl.level_id - 1}", "LOCKED",
                             mouse_pos, disabled=True, font=self.f_tiny)

        self._button(pygame.Rect(margin_x, h - 60, 180, 42), "back / retour menu", "OPEN_TITLE", mouse_pos, font=self.f_small_b)
        unlock_on = save.data.get("all_unlocked_override", False)
        self._button(
            pygame.Rect(w // 2 - 175, h - 60, 350, 42),
            f"free play (unlock all / tout debloquer): {'on' if unlock_on else 'off'}",
            "TOGGLE_UNLOCK_ALL", mouse_pos, primary=unlock_on, accent=COL_GOLD, font=self.f_small_b,
        )
        self._button(pygame.Rect(w - margin_x - 190, h - 60, 190, 42), "garage & shop / boutique", "OPEN_SHOP", mouse_pos, font=self.f_small_b)

    def _draw_shop(self, save: SaveManager, mouse_pos: Tuple[int, int]) -> None:
        w, h = self.width, self.height
        overlay = pygame.Surface((w, h), pygame.SRCALPHA)
        overlay.fill((8, 12, 22, 200))
        self.surface.blit(overlay, (0, 0))

        self._text("douala garage & upgrades / boutique", self.f_title, COL_GOLD, (w // 2, 36), center=True)
        self._text(f"fcfa wallet / solde: {save.data['total_coins']:,} fcfa  |  hoverboards: {save.data['hoverboard_stock']}",
                   self.f_h2, COL_GREEN, (w // 2, 72), center=True)

        margin = max(40, int(w * 0.06))
        col_w = (w - 2 * margin - 28) // 2

        left_panel = pygame.Rect(margin, 106, col_w, h - 186)
        _draw_rounded(self.surface, left_panel, COL_GLASS, radius=16, border_color=COL_BORDER, border_w=2)
        self._text("power-up upgrades / ameliorations", self.f_h2, COL_WHITE, (left_panel.x + 20, left_panel.y + 14))

        up_items = [
            ("magnet", "fcfa coin magnet / aimant", "longer magnetic coin pull / aimant plus long"),
            ("sneakers", "turbo sneakers / baskets", "longer rooftop jump boost / super saut plus long"),
            ("jetpack", "douala sky jetpack / vol", "longer aerial coin flight / vol aerien plus long"),
            ("multiplier", "2x multiplier / score x2", "extended double score / score double plus long"),
        ]
        y = left_panel.y + 52
        for key, title, desc in up_items:
            lvl = int(save.data["upgrades"].get(key, 1))
            cost = save.upgrade_cost(key)
            row = pygame.Rect(left_panel.x + 16, y, left_panel.w - 32, 74)
            _draw_rounded(self.surface, row, (22, 30, 48, 220), radius=12, border_color=(64, 82, 112, 165), border_w=1)
            self._text(f"{title} (lv {lvl}/5)", self.f_body_b, COL_WHITE, (row.x + 14, row.y + 8))
            self._text(desc, self.f_small, COL_MUTED, (row.x + 14, row.y + 30))
            for i in range(5):
                c = COL_GOLD if i < lvl else (54, 64, 84)
                pygame.draw.rect(self.surface, c, (row.x + 14 + i * 28, row.y + 54, 22, 8), border_radius=3)

            btn_r = pygame.Rect(row.right - 134, row.y + 18, 120, 38)
            if lvl >= 5:
                self._button(btn_r, "maxed / max", "NOOP", mouse_pos, disabled=True, font=self.f_small_b)
            else:
                can = save.data["total_coins"] >= cost
                self._button(btn_r, f"{cost} fcfa", "BUY_UPGRADE", mouse_pos,
                             payload=key, primary=can, accent=COL_GOLD, disabled=not can, font=self.f_small_b)
            y += 84

        hb_row = pygame.Rect(left_panel.x + 16, y + 6, left_panel.w - 32, 66)
        _draw_rounded(self.surface, hb_row, (16, 44, 38, 228), radius=12, border_color=COL_GREEN, border_w=1)
        self._text("wouri hoverboard pack (+2 shields / boucliers)", self.f_small_b, COL_GREEN, (hb_row.x + 14, hb_row.y + 10))
        self._text("press space or e mid-run to survive crash / survivre choc", self.f_tiny, COL_WHITE, (hb_row.x + 14, hb_row.y + 36))
        can_hb = save.data["total_coins"] >= 60
        self._button(pygame.Rect(hb_row.right - 134, hb_row.y + 14, 120, 38), "60 fcfa",
                     "BUY_HOVERBOARD", mouse_pos, primary=can_hb, accent=COL_GREEN, disabled=not can_hb, font=self.f_small_b)

        right_panel = pygame.Rect(margin + col_w + 28, 106, col_w, h - 186)
        _draw_rounded(self.surface, right_panel, COL_GLASS, radius=16, border_color=COL_BORDER, border_w=2)
        self._text("maximus outfits / tenues", self.f_h2, COL_WHITE, (right_panel.x + 20, right_panel.y + 14))

        oy = right_panel.y + 52
        unlocked_outfits = save.data.get("unlocked_outfits", [0])
        active_outfit = save.data.get("outfit_id", 0)
        for out in OUTFITS:
            oid = out["id"]
            owned = oid in unlocked_outfits
            equipped = oid == active_outfit
            row = pygame.Rect(right_panel.x + 16, oy, right_panel.w - 32, 86)
            _draw_rounded(self.surface, row, (22, 30, 48, 220), radius=12,
                          border_color=COL_GOLD if equipped else (64, 82, 112, 165), border_w=2 if equipped else 1)
            sw = pygame.Rect(row.x + 14, row.y + 16, 52, 52)
            _draw_rounded(self.surface, sw, out["swatch"], radius=10, border_color=COL_WHITE, border_w=2)
            self._text(out["name"], self.f_body_b, COL_WHITE, (row.x + 80, row.y + 16))
            self._draw_wrapped(out["desc"], self.f_small, COL_MUTED, row.x + 80, row.y + 40, row.w - 224)

            btn_r = pygame.Rect(row.right - 134, row.y + 24, 120, 38)
            if equipped:
                self._button(btn_r, "equipped / actif", "NOOP", mouse_pos, disabled=True, font=self.f_tiny)
            elif owned:
                self._button(btn_r, "equip / porter", "SELECT_OUTFIT", mouse_pos,
                             payload=oid, primary=True, accent=COL_CYAN, font=self.f_small_b)
            else:
                can = save.data["total_coins"] >= out["cost"]
                self._button(btn_r, f"{out['cost']} fcfa", "SELECT_OUTFIT", mouse_pos,
                             payload=oid, primary=can, accent=COL_GOLD, disabled=not can, font=self.f_small_b)
            oy += 98

        self._button(pygame.Rect(margin, h - 60, 185, 42), "back / retour menu", "OPEN_TITLE", mouse_pos, font=self.f_small_b)
        self._button(pygame.Rect(w - margin - 220, h - 60, 220, 42), "select level / niveaux", "OPEN_LEVELS",
                     mouse_pos, primary=True, accent=COL_GREEN, font=self.f_small_b)

    def _draw_controls(self, mouse_pos: Tuple[int, int]) -> None:
        w, h = self.width, self.height
        overlay = pygame.Surface((w, h), pygame.SRCALPHA)
        overlay.fill((8, 12, 22, 200))
        self.surface.blit(overlay, (0, 0))

        box = pygame.Rect((w - 720) // 2, (h - 460) // 2, 720, 460)
        _draw_rounded(self.surface, box, COL_GLASS_DARK, radius=20, border_color=COL_GOLD, border_w=2)
        self._text("how to play / comment jouer - douala run", self.f_h2, COL_GOLD,
                   (box.centerx, box.y + 34), center=True)

        rows = [
            ("left / right  or  a / d", "switch lanes / changer de voie sur le boulevard"),
            ("up  or  w", "jump over obstacles / sauter obstacles et benskins"),
            ("down  or  s", "parkour roll / roulade sous panneaux ou descente rapide"),
            ("space  or  e", "hoverboard shield / bouclier anti-collision"),
            ("p  or  esc", "pause / mettre en pause"),
            ("m  /  f", "mute audio / couper son  |  fullscreen / plein ecran"),
        ]
        y = box.y + 78
        for k, desc in rows:
            kpill = pygame.Rect(box.x + 28, y, 210, 36)
            _draw_rounded(self.surface, kpill, (28, 40, 64, 234), radius=8, border_color=COL_CYAN, border_w=1)
            self._text(k, self.f_small_b, COL_CYAN, kpill.center, center=True, shadow=False)
            self._text(desc, self.f_body, COL_WHITE, (box.x + 252, y + 9))
            y += 48

        tips = "pro tip / astuce: run up hazard ramps onto bus & truck roofs / grimpe sur les toits via les rampes!"
        self._text(tips, self.f_small_b, COL_GOLD, (box.centerx, box.bottom - 76), center=True)
        self._button(pygame.Rect(box.centerx - 120, box.bottom - 54, 240, 40), "back / retour menu",
                     "OPEN_TITLE", mouse_pos, primary=True, accent=COL_GREEN)

    def _draw_playing_hud(self, snap, level: LevelDef, save: SaveManager, fps: float) -> None:
        w, h = self.width, self.height

        # Top-center creator badge at the head of the game
        head_pill = pygame.Rect(w // 2 - 148, 12, 296, 38)
        _draw_rounded(self.surface, head_pill, (10, 14, 24, 200), radius=19,
                      border_color=(248, 196, 38, 145), border_w=1)
        mini_av = load_creator_avatar(30, ring_width=2)
        if mini_av is not None:
            self.surface.blit(mini_av, (head_pill.x + 6, head_pill.y + 4))
        self._text(f"{GAME_BRAND}", self.f_small_b, COL_GOLD, (head_pill.x + 44, head_pill.y + 4))
        self._text(f"by / par {CREATOR_NAME}", self.f_tiny, COL_WHITE, (head_pill.x + 44, head_pill.y + 20), shadow=False)

        # Top-left: District, Progress Bar & Speedometer
        tl = pygame.Rect(18, 16, 336, 96)
        _draw_rounded(self.surface, tl, COL_GLASS, radius=14, border_color=(*level.accent_rgb, 200), border_w=2)

        if level.is_endless:
            zone_lvl = get_level(snap.endless_zone)
            title_txt = f"endless / {zone_lvl.name}"
        else:
            title_txt = f"lvl {level.level_id} / {level.name}"
        self._text(title_txt, self.f_small_b, COL_GOLD, (tl.x + 14, tl.y + 10))

        bar_bg = pygame.Rect(tl.x + 14, tl.y + 34, tl.w - 28, 16)
        _draw_rounded(self.surface, bar_bg, (32, 42, 62, 230), radius=8)
        fill_w = max(8, int(bar_bg.w * max(0.0, min(1.0, snap.progress))))
        _draw_rounded(self.surface, pygame.Rect(bar_bg.x, bar_bg.y, fill_w, bar_bg.h),
                      level.accent_rgb, radius=8)
        if level.is_endless:
            prog_txt = f"{int(snap.distance)}m (next / suivant)"
        else:
            prog_txt = f"{int(snap.distance)}m / {int(snap.target_distance)}m"
        self._text(prog_txt, self.f_small_b, COL_WHITE, (tl.x + 14, tl.y + 56))
        self._text(f"{int(snap.speed_kmh)} km/h", self.f_mono, COL_CYAN,
                   (tl.right - 14, tl.y + 54), right=True)
        if snap.on_roof:
            self._text("rooftop run / course sur toit!", self.f_small_b, COL_GREEN, (tl.x + 14, tl.y + 76))
        else:
            self._text(f"{int(fps)} fps | [p] pause | [m] {'muted' if save.data.get('sound_muted') else 'audio'}",
                       self.f_small, COL_MUTED, (tl.x + 14, tl.y + 76))

        # Top-right: Score, Multiplier & FCFA Coins
        tr = pygame.Rect(w - 292, 16, 274, 96)
        _draw_rounded(self.surface, tr, COL_GLASS, radius=14, border_color=(248, 196, 38, 175), border_w=2)
        self._text(f"{snap.score:07d}", self.f_score, COL_WHITE, (tr.right - 16, tr.y + 10), right=True)

        mpill = pygame.Rect(tr.x + 14, tr.y + 14, 58, 28)
        _draw_rounded(self.surface, mpill, COL_GOLD if snap.multiplier > 1 else (48, 60, 82), radius=8)
        self._text(f"x{snap.multiplier}", self.f_body_b,
                   (12, 16, 24) if snap.multiplier > 1 else COL_WHITE, mpill.center, center=True, shadow=False)

        pygame.draw.circle(self.surface, COL_GOLD, (tr.x + 28, tr.y + 68), 11)
        pygame.draw.circle(self.surface, (255, 242, 160), (tr.x + 28, tr.y + 68), 7)
        self._text(f"{snap.coins} fcfa", self.f_h2, COL_GOLD, (tr.x + 48, tr.y + 55))
        if snap.combo_streak >= 5:
            self._text(f"streak / serie {snap.combo_streak}", self.f_small_b, COL_GREEN,
                       (tr.right - 16, tr.y + 62), right=True)

        # Bottom-left: Hoverboard Shield & Active Power-Ups
        hb_rect = pygame.Rect(18, h - 66, 248, 48)
        hb_active = snap.hoverboard_timer > 0.0
        _draw_rounded(self.surface, hb_rect,
                      (16, 68, 48, 228) if hb_active else COL_GLASS,
                      radius=12, border_color=COL_GREEN if (hb_active or snap.hoverboards_left > 0) else COL_BORDER,
                      border_w=2)
        if hb_active:
            frac = max(0.0, min(1.0, snap.hoverboard_timer / max(0.1, snap.hoverboard_max)))
            pygame.draw.rect(self.surface, COL_GREEN,
                             (hb_rect.x + 8, hb_rect.bottom - 10, int((hb_rect.w - 16) * frac), 5), border_radius=3)
            self._text(f"hoverboard active ({snap.hoverboard_timer:.1f}s)", self.f_small_b,
                       COL_WHITE, (hb_rect.x + 12, hb_rect.y + 10))
        else:
            self._text(f"hoverboards / boucliers: {snap.hoverboards_left}", self.f_small_b,
                       COL_GREEN if snap.hoverboards_left > 0 else COL_MUTED, (hb_rect.x + 12, hb_rect.y + 7))
            self._text("press [space] / [e] for shield", self.f_tiny, COL_MUTED, (hb_rect.x + 12, hb_rect.y + 28))

        active_pus = [
            ("coin magnet / aimant", snap.magnet_timer, snap.magnet_max, COL_RED),
            ("turbo sneakers / saut", snap.sneakers_timer, snap.sneakers_max, COL_GREEN),
            ("sky jetpack / vol", snap.jetpack_timer, snap.jetpack_max, COL_GOLD),
            ("2x multiplier / x2", snap.mult_timer, snap.mult_max, COL_CYAN),
        ]
        py_cursor = hb_rect.y - 44
        for name, t_left, t_max, col in active_pus:
            if t_left <= 0.01:
                continue
            pr = pygame.Rect(18, py_cursor, 248, 36)
            _draw_rounded(self.surface, pr, COL_GLASS, radius=10, border_color=col, border_w=2)
            frac = max(0.0, min(1.0, t_left / max(0.1, t_max)))
            pygame.draw.rect(self.surface, col, (pr.x + 10, pr.bottom - 9, int((pr.w - 20) * frac), 4), border_radius=2)
            self._text(f"{name} {t_left:.1f}s", self.f_small_b, COL_WHITE, (pr.x + 12, pr.y + 6))
            py_cursor -= 42

        # Center popup
        if snap.popup_timer > 0.01 and snap.popup_text:
            alpha_scale = max(0.0, min(1.0, snap.popup_timer * 2.5))
            bob = int(math.sin(self.time * 12.0) * 4)
            pill = pygame.Rect(w // 2 - 220, 66 + bob, 440, 44)
            _draw_rounded(self.surface, pill, (12, 18, 30, _clamp_byte(218 * alpha_scale)),
                          radius=14, border_color=COL_GOLD, border_w=2)
            self._text(snap.popup_text, self.f_h2, COL_GOLD, pill.center, center=True)

        # Bottom-center chaser danger alert
        if snap.chaser_proximity > 0.56 and snap.status == 1:
            pulse = 0.55 + 0.45 * math.sin(self.time * 14.0)
            warn = pygame.Rect(w // 2 - 210, h - 54, 420, 36)
            _draw_rounded(self.surface, warn, (_clamp_byte(180 * pulse), 22, 28, 224),
                          radius=10, border_color=COL_GOLD, border_w=2)
            self._text("traffic patrol behind you / patrouille derriere!", self.f_small_b, COL_WHITE, warn.center, center=True)

    def _draw_pause_overlay(self, level: LevelDef, mouse_pos: Tuple[int, int]) -> None:
        w, h = self.width, self.height
        ov = pygame.Surface((w, h), pygame.SRCALPHA)
        ov.fill((8, 12, 22, 168))
        self.surface.blit(ov, (0, 0))

        box = pygame.Rect((w - 390) // 2, (h - 320) // 2, 390, 320)
        _draw_rounded(self.surface, box, COL_GLASS_DARK, radius=18, border_color=COL_GOLD, border_w=2)
        self._text("paused / pause", self.f_title, COL_GOLD, (box.centerx, box.y + 42), center=True)
        self._text(level.name, self.f_body, COL_MUTED, (box.centerx, box.y + 76), center=True)

        bw = 304
        bx = box.centerx - bw // 2
        self._button(pygame.Rect(bx, box.y + 108, bw, 46), "resume / reprendre [p / esc]", "RESUME",
                     mouse_pos, primary=True, accent=COL_GREEN)
        self._button(pygame.Rect(bx, box.y + 164, bw, 44), "restart / recommencer [r]", "RESTART", mouse_pos)
        self._button(pygame.Rect(bx, box.y + 218, bw, 44), "level select / niveaux", "OPEN_LEVELS", mouse_pos)
        self._button(pygame.Rect(bx, box.y + 270, bw, 36), "main menu / accueil", "OPEN_TITLE", mouse_pos)

    def _draw_game_over(self, snap, level: LevelDef, save: SaveManager, mouse_pos: Tuple[int, int]) -> None:
        w, h = self.width, self.height
        ov = pygame.Surface((w, h), pygame.SRCALPHA)
        ov.fill((16, 6, 10, 168))
        self.surface.blit(ov, (0, 0))

        box = pygame.Rect((w - 480) // 2, (h - 404) // 2, 480, 404)
        _draw_rounded(self.surface, box, COL_GLASS_DARK, radius=20, border_color=COL_RED, border_w=2)
        self._text("wipeout in douala / fin de course!", self.f_h2, COL_RED, (box.centerx, box.y + 44), center=True)
        self._text(snap.crash_reason or "crashed / collision sur le boulevard", self.f_small_b, COL_WHITE,
                   (box.centerx, box.y + 80), center=True)

        stats = pygame.Rect(box.x + 26, box.y + 112, box.w - 52, 134)
        _draw_rounded(self.surface, stats, (20, 28, 44, 224), radius=12, border_color=COL_BORDER, border_w=1)
        dist_goal = "endless" if level.is_endless else f"{int(level.target_distance)}m"
        lines = [
            ("district / quartier:", level.name),
            ("distance:", f"{int(snap.distance)}m / {dist_goal}"),
            ("final score:", f"{snap.score:,} (best: {save.get_best_score(level.level_id):,})"),
            ("fcfa banked / gagnes:", f"+{snap.coins} fcfa | near misses: {snap.near_misses}"),
        ]
        sy = stats.y + 14
        for k, v in lines:
            self._text(k, self.f_small_b, COL_MUTED, (stats.x + 16, sy))
            self._text(v, self.f_small_b, COL_GOLD if "fcfa" in k or "score" in k else COL_WHITE,
                       (stats.right - 16, sy), right=True)
            sy += 28

        bw = box.w - 64
        bx = box.centerx - bw // 2
        self._button(pygame.Rect(bx, box.y + 264, bw, 48), "try again / reessayer [r / enter]", "RESTART",
                     mouse_pos, primary=True, accent=COL_GREEN)
        self._button(pygame.Rect(bx, box.y + 322, (bw - 12) // 2, 42), "levels / niveaux", "OPEN_LEVELS", mouse_pos, font=self.f_small_b)
        self._button(pygame.Rect(bx + (bw + 12) // 2, box.y + 322, (bw - 12) // 2, 42),
                     "shop / boutique", "OPEN_SHOP", mouse_pos, font=self.f_small_b)

    def _draw_level_complete(self, snap, level: LevelDef, save: SaveManager,
                             mouse_pos: Tuple[int, int], stars: int) -> None:
        w, h = self.width, self.height
        ov = pygame.Surface((w, h), pygame.SRCALPHA)
        ov.fill((6, 18, 12, 168))
        self.surface.blit(ov, (0, 0))

        box = pygame.Rect((w - 510) // 2, (h - 445) // 2, 510, 445)
        _draw_rounded(self.surface, box, COL_GLASS_DARK, radius=20, border_color=COL_GOLD, border_w=3)
        self._draw_cameroon_ribbon(box.x + 24, box.y + 16, box.w - 48, 6)

        self._text("district cleared / quartier termine!", self.f_title, COL_GOLD, (box.centerx, box.y + 52), center=True)
        self._text(f"{level.name} - {level.district_fr}", self.f_body_b, COL_GREEN,
                   (box.centerx, box.y + 88), center=True)

        for i in range(3):
            _draw_star(self.surface, box.centerx + (i - 1) * 64, box.y + 136, 22, i < stars)

        stats = pygame.Rect(box.x + 28, box.y + 174, box.w - 56, 126)
        _draw_rounded(self.surface, stats, (18, 30, 46, 224), radius=12, border_color=COL_BORDER, border_w=1)
        lines = [
            ("final score:", f"{snap.score:,}"),
            ("fcfa collected / gagnes:", f"+{snap.coins} fcfa (wallet: {save.data['total_coins']:,})"),
            ("near misses / stumbles:", f"{snap.near_misses} esquives | {snap.stumbles} chocs"),
            ("3-star target / objectif:", f"{level.star3_coins} fcfa & 0 stumbles"),
        ]
        sy = stats.y + 14
        for k, v in lines:
            self._text(k, self.f_small_b, COL_MUTED, (stats.x + 16, sy))
            self._text(v, self.f_small_b, COL_GOLD if "fcfa" in k or "score" in k else COL_WHITE,
                       (stats.right - 16, sy), right=True)
            sy += 27

        bw = box.w - 64
        bx = box.centerx - bw // 2
        next_id = level.level_id + 1 if level.level_id < 6 else 6
        next_label = f"next / suivant: {get_level(next_id).name} [enter]" if level.level_id < 5 else "play endless douala tour / infini!"
        self._button(pygame.Rect(bx, box.y + 314, bw, 48), next_label, "START_LEVEL",
                     mouse_pos, payload=next_id, primary=True, accent=COL_GREEN)
        self._button(pygame.Rect(bx, box.y + 372, (bw - 12) // 2, 42), "replay / rejouer", "RESTART", mouse_pos, font=self.f_small_b)
        self._button(pygame.Rect(bx + (bw + 12) // 2, box.y + 372, (bw - 12) // 2, 42),
                     "levels / niveaux", "OPEN_LEVELS", mouse_pos, font=self.f_small_b)

    @staticmethod
    def _wrap_lines(text: str, font: pygame.font.Font, max_w: int) -> List[str]:
        words = clean_ui_text(text).split(" ")
        lines: List[str] = []
        cur = ""
        for word in words:
            test = f"{cur} {word}".strip()
            if font.size(test)[0] <= max_w:
                cur = test
            else:
                if cur:
                    lines.append(cur)
                cur = word
        if cur:
            lines.append(cur)
        return lines

    def _draw_wrapped(self, text: str, font: pygame.font.Font, color, x: int, y: int, max_w: int) -> None:
        cur_y = y
        for line in self._wrap_lines(text, font, max_w):
            self._text(line, font, color, (x, cur_y), shadow=False)
            cur_y += font.get_linesize()
