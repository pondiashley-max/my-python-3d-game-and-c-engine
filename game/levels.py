"""
Douala District Level Definitions & Visual Atmosphere Themes
for Maximus: Douala Runner 3D.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Tuple

Vec3 = Tuple[float, float, float]


@dataclass(frozen=True)
class ThemeConfig:
    sky_top: Vec3
    sky_horizon: Vec3
    sun_dir: Vec3
    sun_color: Vec3
    ambient_sky: Vec3
    ambient_ground: Vec3
    fog_color: Vec3
    fog_start: float
    fog_end: float
    wetness: float          # 0.0 dry asphalt -> 0.85 glossy rain-slicked street
    night_windows: float    # 0.0 daytime -> 1.0 glowing lit windows at night
    sun_disc_color: Vec3
    curve_x: float          # Subtle Subway Surfers horizon curve X
    curve_y: float          # Subtle downward horizon droop


@dataclass(frozen=True)
class LevelDef:
    level_id: int
    name: str
    district_fr: str
    subtitle: str
    difficulty: int         # 1..5 (6 = Endless)
    difficulty_label: str
    target_distance: float
    is_endless: bool
    is_bridge: bool
    is_market: bool
    has_monument: bool
    is_rainy: bool
    is_night: bool
    base_speed: float
    max_speed: float
    speed_accel: float
    obstacle_density: float
    moving_traffic_chance: float
    ramp_combo_chance: float
    powerup_chance: float
    seed: int
    star2_coins: int
    star3_coins: int
    accent_rgb: Tuple[int, int, int]
    theme: ThemeConfig


# Rich tropical lighting themes tailored to each real Douala neighborhood
ZONE_THEMES = {
    # 1. Akwa Boulevard — Bright tropical morning coastal sunshine
    1: ThemeConfig(
        sky_top=(0.14, 0.48, 0.86),
        sky_horizon=(0.76, 0.89, 0.96),
        sun_dir=(0.45, 0.78, -0.42),
        sun_color=(1.05, 0.98, 0.86),
        ambient_sky=(0.44, 0.56, 0.68),
        ambient_ground=(0.32, 0.24, 0.18),
        fog_color=(0.74, 0.86, 0.94),
        fog_start=58.0,
        fog_end=158.0,
        wetness=0.08,
        night_windows=0.0,
        sun_disc_color=(1.0, 0.95, 0.76),
        curve_x=0.00018,
        curve_y=-0.00055,
    ),
    # 2. Marché Mboppi — High midday tropical heat & vivid saturated colors
    2: ThemeConfig(
        sky_top=(0.10, 0.54, 0.92),
        sky_horizon=(0.94, 0.86, 0.70),
        sun_dir=(0.22, 0.94, -0.26),
        sun_color=(1.10, 1.00, 0.82),
        ambient_sky=(0.48, 0.54, 0.56),
        ambient_ground=(0.38, 0.26, 0.16),
        fog_color=(0.90, 0.83, 0.70),
        fog_start=55.0,
        fog_end=150.0,
        wetness=0.05,
        night_windows=0.0,
        sun_disc_color=(1.0, 0.96, 0.82),
        curve_x=-0.00020,
        curve_y=-0.00055,
    ),
    # 3. Rond-Point Deido — Warm late-afternoon golden coastal haze
    3: ThemeConfig(
        sky_top=(0.20, 0.42, 0.76),
        sky_horizon=(0.96, 0.76, 0.52),
        sun_dir=(-0.55, 0.62, -0.56),
        sun_color=(1.08, 0.86, 0.58),
        ambient_sky=(0.42, 0.44, 0.56),
        ambient_ground=(0.36, 0.24, 0.17),
        fog_color=(0.92, 0.74, 0.54),
        fog_start=52.0,
        fog_end=148.0,
        wetness=0.10,
        night_windows=0.15,
        sun_disc_color=(1.0, 0.82, 0.48),
        curve_x=0.00024,
        curve_y=-0.00060,
    ),
    # 4. Pont du Wouri — Dramatic flaming sunset over the Wouri River
    4: ThemeConfig(
        sky_top=(0.18, 0.16, 0.44),
        sky_horizon=(0.98, 0.52, 0.26),
        sun_dir=(0.36, 0.34, -0.87),
        sun_color=(1.15, 0.66, 0.34),
        ambient_sky=(0.38, 0.30, 0.46),
        ambient_ground=(0.32, 0.20, 0.16),
        fog_color=(0.92, 0.52, 0.32),
        fog_start=48.0,
        fog_end=146.0,
        wetness=0.28,
        night_windows=0.55,
        sun_disc_color=(1.0, 0.64, 0.22),
        curve_x=-0.00016,
        curve_y=-0.00050,
    ),
    # 5. Bonanjo Night Rainstorm — Electric tropical rainstorm, neon & headlights
    5: ThemeConfig(
        sky_top=(0.03, 0.05, 0.14),
        sky_horizon=(0.10, 0.17, 0.32),
        sun_dir=(-0.35, 0.65, -0.66),
        sun_color=(0.42, 0.56, 0.85),
        ambient_sky=(0.18, 0.24, 0.38),
        ambient_ground=(0.10, 0.12, 0.18),
        fog_color=(0.08, 0.14, 0.26),
        fog_start=38.0,
        fog_end=132.0,
        wetness=0.90,
        night_windows=1.0,
        sun_disc_color=(0.82, 0.90, 1.0),
        curve_x=0.00026,
        curve_y=-0.00062,
    ),
}


LEVELS = [
    LevelDef(
        level_id=1,
        name="akwa boulevard",
        district_fr="boulevard de la liberte",
        subtitle="morning rush / heure de pointe a akwa, palmiers et taxis jaunes",
        difficulty=1,
        difficulty_label="rookie / debut",
        target_distance=650.0,
        is_endless=False,
        is_bridge=False,
        is_market=False,
        has_monument=False,
        is_rainy=False,
        is_night=False,
        base_speed=17.5,
        max_speed=25.5,
        speed_accel=0.014,
        obstacle_density=0.40,
        moving_traffic_chance=0.22,
        ramp_combo_chance=0.28,
        powerup_chance=0.18,
        seed=23701,
        star2_coins=45,
        star3_coins=90,
        accent_rgb=(24, 180, 96),
        theme=ZONE_THEMES[1],
    ),
    LevelDef(
        level_id=2,
        name="marche mboppi",
        district_fr="grand marche populaire",
        subtitle="dodge pushcarts & benskins / evite pousse-pousse, parasols et benskins",
        difficulty=2,
        difficulty_label="street / rue",
        target_distance=850.0,
        is_endless=False,
        is_bridge=False,
        is_market=True,
        has_monument=False,
        is_rainy=False,
        is_night=False,
        base_speed=20.0,
        max_speed=29.5,
        speed_accel=0.015,
        obstacle_density=0.55,
        moving_traffic_chance=0.30,
        ramp_combo_chance=0.26,
        powerup_chance=0.16,
        seed=23702,
        star2_coins=65,
        star3_coins=125,
        accent_rgb=(244, 166, 26),
        theme=ZONE_THEMES[2],
    ),
    LevelDef(
        level_id=3,
        name="rond-point deido",
        district_fr="monument nouvelle liberte",
        subtitle="roadworks & landmark / chicanes de travaux sous le monument de deido",
        difficulty=3,
        difficulty_label="pro / avance",
        target_distance=1050.0,
        is_endless=False,
        is_bridge=False,
        is_market=False,
        has_monument=True,
        is_rainy=False,
        is_night=False,
        base_speed=22.5,
        max_speed=33.5,
        speed_accel=0.016,
        obstacle_density=0.68,
        moving_traffic_chance=0.35,
        ramp_combo_chance=0.32,
        powerup_chance=0.15,
        seed=23703,
        star2_coins=85,
        star3_coins=155,
        accent_rgb=(235, 92, 38),
        theme=ZONE_THEMES[3],
    ),
    LevelDef(
        level_id=4,
        name="pont du wouri",
        district_fr="traversee du fleuve wouri",
        subtitle="sunset bridge run / course au coucher de soleil sur les camions du port",
        difficulty=4,
        difficulty_label="expert / dur",
        target_distance=1250.0,
        is_endless=False,
        is_bridge=True,
        is_market=False,
        has_monument=False,
        is_rainy=False,
        is_night=False,
        base_speed=24.5,
        max_speed=37.0,
        speed_accel=0.017,
        obstacle_density=0.78,
        moving_traffic_chance=0.38,
        ramp_combo_chance=0.36,
        powerup_chance=0.15,
        seed=23704,
        star2_coins=105,
        star3_coins=190,
        accent_rgb=(224, 62, 74),
        theme=ZONE_THEMES[4],
    ),
    LevelDef(
        level_id=5,
        name="bonanjo rainstorm",
        district_fr="quartier administratif - nuit",
        subtitle="tropical night rain / pluie nocturne, phares et trafic rapide a bonanjo",
        difficulty=5,
        difficulty_label="master / maitre",
        target_distance=1500.0,
        is_endless=False,
        is_bridge=False,
        is_market=False,
        has_monument=False,
        is_rainy=True,
        is_night=True,
        base_speed=26.5,
        max_speed=40.5,
        speed_accel=0.018,
        obstacle_density=0.88,
        moving_traffic_chance=0.42,
        ramp_combo_chance=0.32,
        powerup_chance=0.16,
        seed=23705,
        star2_coins=125,
        star3_coins=225,
        accent_rgb=(88, 152, 255),
        theme=ZONE_THEMES[5],
    ),
    LevelDef(
        level_id=6,
        name="douala endless tour",
        district_fr="tour complet 237 - infini",
        subtitle="non-stop 5 districts / tour infini des 5 quartiers avec vitesse croissante",
        difficulty=6,
        difficulty_label="endless / infini",
        target_distance=999999.0,
        is_endless=True,
        is_bridge=False,
        is_market=False,
        has_monument=True,
        is_rainy=False,
        is_night=False,
        base_speed=19.5,
        max_speed=44.0,
        speed_accel=0.011,
        obstacle_density=0.62,
        moving_traffic_chance=0.33,
        ramp_combo_chance=0.32,
        powerup_chance=0.16,
        seed=23799,
        star2_coins=100,
        star3_coins=250,
        accent_rgb=(250, 206, 26),
        theme=ZONE_THEMES[1],
    ),
]


OUTFITS = [
    {
        "id": 0,
        "name": "lion indomptable",
        "desc": "cameroon green red gold jersey / maillot vert rouge jaune",
        "cost": 0,
        "swatch": (18, 166, 82),
    },
    {
        "id": 1,
        "name": "akwa street",
        "desc": "electric blue streetwear / tenue urbaine bleue akwa",
        "cost": 150,
        "swatch": (38, 114, 240),
    },
    {
        "id": 2,
        "name": "wouri gold",
        "desc": "sunset gold & black kit / tenue or et noir du wouri",
        "cost": 300,
        "swatch": (250, 194, 30),
    },
    {
        "id": 3,
        "name": "bonanjo night",
        "desc": "neon magenta & lime storm jacket / veste neon nuit bonanjo",
        "cost": 450,
        "swatch": (212, 42, 184),
    },
]


def get_level(level_id: int) -> LevelDef:
    for lvl in LEVELS:
        if lvl.level_id == level_id:
            return lvl
    return LEVELS[0]


def lerp_theme(a: ThemeConfig, b: ThemeConfig, t: float) -> ThemeConfig:
    t = max(0.0, min(1.0, t))

    def lv(u: Vec3, v: Vec3) -> Vec3:
        return (
            u[0] + (v[0] - u[0]) * t,
            u[1] + (v[1] - u[1]) * t,
            u[2] + (v[2] - u[2]) * t,
        )

    def lf(x: float, y: float) -> float:
        return x + (y - x) * t

    return ThemeConfig(
        sky_top=lv(a.sky_top, b.sky_top),
        sky_horizon=lv(a.sky_horizon, b.sky_horizon),
        sun_dir=lv(a.sun_dir, b.sun_dir),
        sun_color=lv(a.sun_color, b.sun_color),
        ambient_sky=lv(a.ambient_sky, b.ambient_sky),
        ambient_ground=lv(a.ambient_ground, b.ambient_ground),
        fog_color=lv(a.fog_color, b.fog_color),
        fog_start=lf(a.fog_start, b.fog_start),
        fog_end=lf(a.fog_end, b.fog_end),
        wetness=lf(a.wetness, b.wetness),
        night_windows=lf(a.night_windows, b.night_windows),
        sun_disc_color=lv(a.sun_disc_color, b.sun_disc_color),
        curve_x=lf(a.curve_x, b.curve_x),
        curve_y=lf(a.curve_y, b.curve_y),
    )


def to_engine_params(lvl: LevelDef, eng_mod):
    p = eng_mod.LevelParams()
    p.level_id = lvl.level_id
    p.is_endless = lvl.is_endless
    p.is_bridge = lvl.is_bridge
    p.is_market = lvl.is_market
    p.has_monument = lvl.has_monument
    p.is_rainy = lvl.is_rainy
    p.is_night = lvl.is_night
    p.target_distance = lvl.target_distance
    p.base_speed = lvl.base_speed
    p.max_speed = lvl.max_speed
    p.speed_accel = lvl.speed_accel
    p.obstacle_density = lvl.obstacle_density
    p.moving_traffic_chance = lvl.moving_traffic_chance
    p.ramp_combo_chance = lvl.ramp_combo_chance
    p.powerup_chance = lvl.powerup_chance
    p.seed = lvl.seed
    return p
