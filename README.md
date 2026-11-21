# douala run - maximus 3d

a full 3d endless and level-based street runner set in douala, cameroon, engineered from scratch using only c++17 and python.

created in solo by **pondi ashley rey maxim** since year 1 (2024) under pure passion for programming.
(c) 2024-2026 pondi ashley rey maxim - all rights reserved.

---

## project resume

- **genre**: 3d third-person endless and campaign runner (subway surfers style) set across real neighborhoods of douala.
- **languages**: native c++17 engine (`pybind11`) + python 3 opengl 3.3 core (`moderngl`, `pygame-ce`, `numpy`, `pillow`). zero external game engines.
- **core engine (c++17)**: fixed 120 hz physics simulation, critically-damped spring lane switching, landing knee compression, 360-degree parkour roll, 2-bone articulated character kinematics, 39 procedural 3d meshes (7,838 unique triangles), guaranteed-solvable obstacle pattern generation, animated douala pnj pedestrians and market vendors, rooftop bus/truck ramp running, traffic patrol chaser ai, 3d particles and rain, and zero-copy per-frame gpu instance packing.
- **graphics & audio (python)**: instanced opengl 3.3 core renderer with 9-tap pcf soft directional shadows, aces filmic tone mapping, procedural tropical sky dome with drifting clouds, subtle curved-world horizon, wet rain-slicked asphalt reflections, night-lit windows, glowing vehicle headlight cones, animated shimmering wouri river water, and a procedural makossa/afrobeat-inspired audio synthesizer.
- **presentation & legal flow**: gta 4 style slow parallax intro (`douala run` -> diagonal amber-noir shutter -> illustrated creator portrait of `pondi ashley rey maxim`), mandatory scrollable privacy policy and legal acceptance gate before entering the game, creator profile portrait header, interactive level selection, garage upgrades and outfits shop, and an `about us` / programmer biography screen.

---

## douala districts & difficulty progression

| level | district | setting | goal | speed range | difficulty |
| --- | --- | --- | --- | --- | --- |
| 1 | akwa boulevard | morning rush, palm trees, yellow taxis, commercial blocks | 650 m | 63-91 km/h | rookie |
| 2 | marche mboppi | midday bustle, striped market parasols, plantain pushcarts, benskins, street vendors | 850 m | 72-106 km/h | street |
| 3 | rond-point deido | afternoon heat, roadworks chicanes, monument de la nouvelle liberte | 1050 m | 81-120 km/h | pro |
| 4 | pont du wouri | golden sunset steel truss bridge over shimmering wouri river, port cranes, container trucks | 1250 m | 88-133 km/h | expert |
| 5 | bonanjo rainstorm | tropical night downpour, wet reflective roads, glowing windows, headlight cones, heavy traffic | 1500 m | 95-145 km/h | master |
| inf | douala endless tour | non-stop seamless loop through all 5 districts with continuously escalating speed | endless | 70-158 km/h | endless |

difficulty increases both across districts (faster base and top speed, tighter obstacle density, higher oncoming traffic probability, longer targets) and dynamically within each run as distance increases.

---

## quick start

### prerequisites

- python 3.10+ (tested on python 3.12)
- c++17 compiler (`clang++` on macos, `g++` on linux, or msvc on windows)
- opengl 3.3+ compatible gpu

### install dependencies

```bash
pip install -r requirements.txt
```

### build & run

```bash
python3 main.py
```

`main.py` automatically runs `build.py` to compile the native c++17 extension (`_maximus_engine`) on first launch or whenever any `.cpp`/`.hpp` file changes. to force a manual rebuild:

```bash
python3 build.py --force
```

---

## controls

| key | action |
| --- | --- |
| `left` / `right` or `a` / `d` | switch lanes across the 3-lane douala boulevard |
| `up` or `w` | jump over barricades, trenches, pushcarts, and benskins |
| `down` or `s` | 360-degree parkour roll under overhead signs (or fast-fall in mid-air) |
| `space`, `e`, or `shift` | activate wouri hoverboard shield (absorbs one fatal crash) |
| `p` or `esc` | pause / back |
| `m` | mute / unmute procedural makossa audio |
| `f` or `f11` | toggle fullscreen |

---

## repository layout

```text
.
|-- main.py                     # launcher (auto-builds c++17 engine and starts game)
|-- build.py                    # pure-python c++17 compiler driver using pybind11
|-- requirements.txt            # python dependencies
|-- docs/
|   `-- documentation.md        # full technical and gameplay documentation
|-- assets/
|   `-- creator_profile.jpg     # creator portrait integrated in intro, head bar, and biography
|-- engine/
|   |-- include/                # c++17 headers (math3d, types, mesh_builder, world_gen, game_engine)
|   `-- src/                    # c++17 sources (procedural meshes, world gen, physics/pnj, pybind11 bindings)
|-- game/
|   |-- app.py                  # main sdl2/opengl game loop and state machine
|   |-- renderer.py             # opengl 3.3 core instanced renderer, shadow maps, shaders
|   |-- hud.py                  # gta 4 intro, legal gate, menus, about us, and in-game hud
|   |-- legal_content.py        # bilingual privacy policy, legal eula, biography, avatar loaders
|   |-- levels.py               # douala district definitions, lighting themes, and outfits
|   |-- textures.py             # procedural asphalt, building facade, and billboard textures
|   |-- audio.py                # procedural makossa/afrobeat synth groove and sfx
|   `-- save_manager.py         # local json save persistence (levels, stars, fcfa wallet, upgrades)
`-- tests/
    |-- test_engine.py          # automated unit tests for c++ meshes, physics, pnj, legal, text rules
    |-- test_window_launch.py   # end-to-end live sdl2 + opengl 3.3 window flow verification
    `-- capture_screenshots.py  # offscreen opengl 3.3 visual render and fps benchmark tool
```

---

## verification & testing

run the automated test suites:

```bash
python3 tests/test_engine.py
python3 tests/test_window_launch.py
python3 tests/capture_screenshots.py
```

for full engine internals, rendering pipeline details, physics formulas, and customization guides, see [docs/documentation.md](docs/documentation.md).

---

## author & license

- **creator**: pondi ashley rey maxim
- **conception**: solo project started in year 1 (2024) out of passion for programming
- **copyright**: (c) 2024-2026 pondi ashley rey maxim - all rights reserved
