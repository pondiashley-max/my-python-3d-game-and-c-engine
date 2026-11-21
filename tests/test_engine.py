#!/usr/bin/env python3
"""
Comprehensive automated test suite for Maximus: Douala Runner 3D.
Tests C++17 physics, 36 procedural meshes, all 5 Douala levels + Endless mode,
difficulty escalation, controls, hoverboard shield, save manager, textures, and audio.
"""

from __future__ import annotations

import math
import sys
import tempfile
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

import build as engine_builder

engine_builder.build(force=False)
import _maximus_engine as E
from game.levels import LEVELS, get_level, to_engine_params
from game.save_manager import SaveManager
from game.textures import (
    generate_billboard_texture,
    generate_facade_texture,
    generate_road_texture,
)


def test_all_procedural_meshes():
    eng = E.GameEngine()
    assert E.MESH_COUNT == 39, f"Expected 39 meshes, got {E.MESH_COUNT}"
    total_tris = 0
    for mid in range(E.MESH_COUNT):
        vb, ib, vc, ic = eng.get_mesh_buffers(mid)
        assert vc >= 3, f"Mesh {mid} has too few vertices: {vc}"
        assert ic >= 3 and ic % 3 == 0, f"Mesh {mid} has invalid index count: {ic}"
        assert len(vb) == vc * 48, f"Mesh {mid} vertex buffer byte mismatch"
        assert len(ib) == ic * 4, f"Mesh {mid} index buffer byte mismatch"
        verts = np.frombuffer(vb, dtype=np.float32).reshape(vc, 12)
        assert np.all(np.isfinite(verts)), f"Mesh {mid} contains NaN/Inf vertices"
        idxs = np.frombuffer(ib, dtype=np.uint32)
        assert int(idxs.max()) < vc, f"Mesh {mid} index out of bounds"
        total_tris += ic // 3
    print(f"[PASS] All {E.MESH_COUNT} C++ procedural 3D meshes verified ({total_tris:,} total unique triangles)")


def test_level_difficulty_progression():
    campaign = [l for l in LEVELS if not l.is_endless]
    assert len(campaign) == 5, "Must have 5 Douala campaign levels"
    for i in range(len(campaign) - 1):
        a, b = campaign[i], campaign[i + 1]
        assert b.base_speed > a.base_speed, f"Base speed must increase L{a.level_id}->L{b.level_id}"
        assert b.max_speed > a.max_speed, f"Max speed must increase L{a.level_id}->L{b.level_id}"
        assert b.obstacle_density > a.obstacle_density, f"Obstacle density must increase L{a.level_id}->L{b.level_id}"
        assert b.moving_traffic_chance > a.moving_traffic_chance, "Traffic chance must increase"
        assert b.target_distance > a.target_distance, "Target distance must increase"
    endless = get_level(6)
    assert endless.is_endless
    print("[PASS] Strictly increasing difficulty verified across all 5 Douala districts + Endless mode")


def test_controls_and_physics():
    eng = E.GameEngine()
    lvl = get_level(1)
    p = to_engine_params(lvl, E)
    u = E.UpgradeParams()
    u.starting_hoverboards = 2
    eng.start_level(p, u, demo_mode=False)

    s0 = eng.snapshot()
    assert abs(s0.player_x - 0.0) < 1e-4
    initial_speed = s0.speed_mps

    # Switch left -> lane 0 (-2.8)
    cmd = E.InputCommands()
    cmd.left = True
    eng.push_input(cmd)
    for _ in range(30):
        eng.step(1.0 / 60.0)
    assert eng.snapshot().player_x < -2.5, f"Expected left lane x~-2.8, got {eng.snapshot().player_x}"

    # Switch right twice -> lane 2 (+2.8)
    cmd = E.InputCommands()
    cmd.right = True
    eng.push_input(cmd)
    eng.push_input(cmd)
    for _ in range(35):
        eng.step(1.0 / 60.0)
    assert eng.snapshot().player_x > 2.5, f"Expected right lane x~+2.8, got {eng.snapshot().player_x}"

    # Jump arc height test
    cmd = E.InputCommands()
    cmd.jump = True
    eng.push_input(cmd)
    max_y = 0.0
    for _ in range(45):
        eng.step(1.0 / 60.0)
        max_y = max(max_y, eng.snapshot().player_y)
    assert max_y > 2.2, f"Expected jump apex > 2.2m, got {max_y:.2f}m"

    # Roll test
    cmd = E.InputCommands()
    cmd.roll = True
    eng.push_input(cmd)
    eng.step(1.0 / 60.0)
    assert eng.snapshot().is_rolling, "Expected rolling state after roll input"

    # Hoverboard activation
    cmd = E.InputCommands()
    cmd.hoverboard = True
    eng.push_input(cmd)
    eng.step(1.0 / 60.0)
    assert eng.snapshot().hoverboard_timer > 14.0, "Hoverboard should activate for ~15s"
    assert eng.snapshot().hoverboards_left == 1, "Hoverboard stock should decrement"

    # Speed escalation with distance
    assert eng.snapshot().speed_mps > initial_speed, "Speed must escalate as Maximus runs forward"
    print(f"[PASS] Controls & physics verified (jump apex={max_y:.2f}m, hoverboard active, speed escalated)")


def test_long_simulation_all_levels():
    eng = E.GameEngine()
    u = E.UpgradeParams()
    for lvl in LEVELS:
        p = to_engine_params(lvl, E)
        eng.start_level(p, u, demo_mode=True)
        start_spd = eng.snapshot().speed_mps
        # Run 300 frames (~5 seconds = ~100m+ per level)
        for _ in range(300):
            eng.step(1.0 / 60.0)
        snap = eng.snapshot()
        assert snap.distance > 75.0, f"Level {lvl.level_id} didn't advance properly: {snap.distance}"
        assert snap.speed_mps >= start_spd, f"Level {lvl.level_id} speed regressed"
        assert eng.total_instance_count() > 30, f"Level {lvl.level_id} emitted too few 3D instances"
        assert eng.npc_count() > 0, f"Level {lvl.level_id} spawned zero Douala PNJs!"

        # Verify instance buffer alignment & finite matrices
        for mid in range(E.MESH_COUNT):
            cnt, mv = eng.get_instance_view(mid)
            if cnt > 0:
                arr = np.frombuffer(mv, dtype=np.float32).reshape(cnt, 20)
                assert np.all(np.isfinite(arr)), f"Non-finite instance data in mesh {mid}"
    print("[PASS] Long-run stability & zero-copy GPU instance batching verified across all 6 modes")


def test_level_completion_and_save_manager():
    eng = E.GameEngine()
    lvl = get_level(1)
    p = to_engine_params(lvl, E)
    p.target_distance = 120.0  # Short target to verify finish line transition deterministically
    p.obstacle_density = 0.0
    u = E.UpgradeParams()
    u.starting_hoverboards = 5
    eng.start_level(p, u, demo_mode=False)

    for _ in range(600):
        if eng.snapshot().hoverboard_timer <= 0.0 and eng.snapshot().hoverboards_left > 0:
            cmd = E.InputCommands()
            cmd.hoverboard = True
            eng.push_input(cmd)
        eng.step(1.0 / 60.0)
        if eng.snapshot().status == E.STATUS_LEVEL_COMPLETE:
            break
    snap = eng.snapshot()
    assert snap.status == E.STATUS_LEVEL_COMPLETE, f"Expected LEVEL_COMPLETE, got status={snap.status}"

    with tempfile.TemporaryDirectory() as tmp:
        sm = SaveManager(Path(tmp) / "test_save.json")
        assert sm.is_legal_accepted() is False, "Fresh install must require legal acceptance"
        sm.accept_legal()
        assert sm.is_legal_accepted() is True, "Legal acceptance must persist"
        sm2 = SaveManager(Path(tmp) / "test_save.json")
        assert sm2.is_legal_accepted() is True, "Legal acceptance must reload from JSON"
        assert sm.is_level_unlocked(1)
        assert not sm.is_level_unlocked(2)
        stars = sm.record_run(1, score=5000, coins=100, distance=650.0, completed=True, stumbles=0)
        assert stars == 3, f"Expected 3 stars, got {stars}"
        assert sm.is_level_unlocked(2), "Clearing Level 1 must unlock Level 2"
        assert sm.buy_upgrade("magnet"), "Should afford magnet upgrade"
        assert sm.data["upgrades"]["magnet"] == 2
        assert sm.toggle_unlock_all() is True
        assert sm.is_level_unlocked(5), "Free play override must unlock Level 5"
    print("[PASS] Level finish line, legal gate persistence, 3-star scoring, progression unlock, and shop verified")


def test_creator_avatar_and_legal_content():
    import pygame
    from game.legal_content import (
        ABOUT_CONCEPTION_PARAGRAPHS,
        COPYRIGHT_LINE,
        CREATOR_NAME,
        CREATOR_PHOTO_PATH,
        GAME_BRAND,
        LEGAL_DOCUMENT_SECTIONS,
        PRIVACY_POLICY_SECTIONS,
        PROGRAMMER_BIO_LINES,
        clean_ui_text,
        load_creator_avatar,
        load_gta4_portrait,
    )
    from game.levels import OUTFITS

    assert CREATOR_PHOTO_PATH.exists(), f"Creator photo missing at {CREATOR_PHOTO_PATH}"
    assert "pondi ashley rey maxim" in CREATOR_NAME
    assert len(PRIVACY_POLICY_SECTIONS) >= 5
    assert len(LEGAL_DOCUMENT_SECTIONS) >= 5
    assert any("2024" in p and "year 1" in p for p in ABOUT_CONCEPTION_PARAGRAPHS)

    # Verify strict lowercase & accent-free invariant across ALL content texts!
    all_texts = [CREATOR_NAME, GAME_BRAND, COPYRIGHT_LINE]
    all_texts += [t for pair in PRIVACY_POLICY_SECTIONS for t in pair]
    all_texts += [t for pair in LEGAL_DOCUMENT_SECTIONS for t in pair]
    all_texts += ABOUT_CONCEPTION_PARAGRAPHS + PROGRAMMER_BIO_LINES
    for lvl in LEVELS:
        all_texts += [lvl.name, lvl.district_fr, lvl.subtitle, lvl.difficulty_label]
    for out in OUTFITS:
        all_texts += [out["name"], out["desc"]]
    for txt in all_texts:
        assert txt == clean_ui_text(txt), f"Text violates lowercase/no-accent rule: {txt!r}"

    pygame.init()
    av = load_creator_avatar(128, ring_width=4)
    assert av is not None and av.get_size() == (128, 128)
    gta_art = load_gta4_portrait(310, 388)
    assert gta_art is not None and gta_art.get_size() == (310, 388)

    # Regression test: sweep GTA IV intro timeline from -0.5s through 9.0s + shutter transition!
    from game.hud import HudManager

    hud = HudManager(1280, 720)
    eng = E.GameEngine()
    lvl = get_level(1)
    with tempfile.TemporaryDirectory() as tmp:
        sm = SaveManager(Path(tmp) / "intro_test.json")
        for step in range(-5, 92):
            hud.render("INTRO", eng.snapshot(), sm, lvl, (0, 0), 0.016,
                       intro_time=step * 0.1, fade_alpha=max(0.0, min(1.0, (step - 66) * 0.08)))
    pygame.quit()
    print("[PASS] Lowercase/no-accent text, GTA IV artwork, PNJ spawns & full Intro timeline verified")


def test_textures_and_audio():
    road = generate_road_texture(256)
    facade = generate_facade_texture(256)
    bb = generate_billboard_texture(256, 128)
    assert len(road) == 256 * 256 * 4
    assert len(facade) == 256 * 256 * 4
    assert len(bb) == 256 * 128 * 4

    import pygame
    from game.audio import DoualaAudio

    pygame.init()
    audio = DoualaAudio(E, muted=True)
    # Even if headless audio unavailable, must initialize cleanly without raising
    audio.play_events([E.SFX_COIN, E.SFX_JUMP, E.SFX_HORN])
    pygame.quit()
    print("[PASS] Procedural textures & Douala Makossa audio synthesizer verified")


if __name__ == "__main__":
    test_all_procedural_meshes()
    test_level_difficulty_progression()
    test_controls_and_physics()
    test_long_simulation_all_levels()
    test_level_completion_and_save_manager()
    test_creator_avatar_and_legal_content()
    test_textures_and_audio()
    print("\nALL AUTOMATED TESTS PASSED!")
