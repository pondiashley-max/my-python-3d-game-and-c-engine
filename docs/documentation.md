# douala run (maximus 3d) - full documentation

complete technical and gameplay documentation for **douala run (maximus 3d)**, a solo 3d street-running game built strictly with c++17 and python.

created by **pondi ashley rey maxim** (conceived since year 1 - 2024).
(c) 2024-2026 pondi ashley rey maxim - all rights reserved.

---

## 1. overview & design philosophy

douala run was designed around two core engineering principles:

1. **zero external game engines**: every 3d mesh, collision volume, physics equation, shadow shader, texture, sound effect, and menu widget is built directly in c++17 and python.
2. **strict separation of real-time simulation and presentation**:
   - **c++17 native module (`_maximus_engine`)** owns all cpu-intensive per-frame work: 120 hz fixed-timestep kinematics, procedural geometry construction, obstacle and pnj generation, collision detection, particle updates, and flat gpu instance buffer serialization.
   - **python layer (`game/`)** owns the opengl 3.3 core rendering pipeline (`moderngl`), procedural texture synthesis (`pillow` + `numpy`), procedural makossa audio synthesis (`numpy` + `pygame.mixer`), ui/menus (`pygame-ce`), and persistent json progression.

```text
+-------------------------------------------------------------------+
|                     python game layer (game/)                     |
|  app.py -> input events, state machine, theme cross-fading        |
|  hud.py -> gta 4 parallax intro, legal gate, menus, about us, hud |
|  renderer.py -> opengl 3.3 shadow map + instanced 3d + hud pass   |
|  audio.py -> procedural makossa groove & dynamic sfx              |
+---------------------------------+---------------------------------+
                                  |
              zero-copy pybind11 memoryview (80 bytes/instance)
                                  |
+---------------------------------v---------------------------------+
|                 native c++17 engine (engine/)                     |
|  game_engine.cpp -> 120 hz physics, 2-bone kinematics, pnj ai     |
|  world_gen.cpp   -> solvable douala street chunks & pnj spawner   |
|  mesh_builder.cpp -> 39 procedural 3d meshes (7,838 triangles)    |
|  math3d.hpp      -> column-major mat4, vec3, aabb, pcg rng        |
+-------------------------------------------------------------------+
```

---

## 2. native c++17 engine (`engine/`)

### 2.1 coordinate system & scale

- track simulation runs in intuitive positive forward space:
  - `x`: lateral position across 3 lanes (`-2.8`, `0.0`, `+2.8` meters; road width `10.4` meters).
  - `y`: vertical height above the asphalt (`0.0` road surface, `0.22` raised sidewalk).
  - `s`: forward distance run in meters (`s >= 0`, increasing monotonically).
- at render serialization time, `to_gl_pos(x, y, s)` maps `(x, y, s) -> (x, y, -s)` into right-handed opengl world space where `-z` is forward, `+x` is screen right, and `+y` is up.

### 2.2 procedural 3d mesh catalog (`mesh_builder.cpp`)

`meshbuilder::build_all_meshes()` generates **39 distinct multi-part meshes** directly in c++ with position, normal, uv, vertex color, and material tag (12 floats / 48 bytes per vertex):

- **roads & landmarks**: douala 3-lane boulevard with painted red-white curbs, pont du wouri steel x-truss bridge deck, subdivided wouri river water plane, modern multi-story buildings with balconies and rooftop water tanks, tropical zinc-roof shopfronts, marche mboppi striped market parasols, coconut palm trees, streetlamps, douala billboards, monument de la nouvelle liberte, port of douala container gantry cranes, and cameroon tricolor finish archway.
- **real-world douala vehicles & obstacles**: yellow taxis (`taxi jaune` with checkered stripe, headlights, license plate, illuminated roof sign), benskin moto-taxis with seated riders in reflective vests, coaster urban transit buses, port container cargo trucks, wooden hazard ramps, jump-over roadblocks, slide-under overhead gantries, concrete jersey barriers with traffic cones, drainage potholes, plantain `pousse-pousse` pushcarts, and volumetric headlight cones.
- **articulated characters & collectibles**: sculpted torso with backpack, rounded head with cap and headphones, tapered upper limbs, tapered lower limbs, running sneakers with white midsoles, glowing hoverboard, twin-nozzle jetpack, embossed metallic gold fcfa coins, 5 power-up models, particles, rain streaks, and contact shadows.

### 2.3 realistic movement physics & 2-bone kinematics (`game_engine.cpp`)

- **fixed-timestep simulation**: internally stepped at `120 hz` (`dt = 1/120s`) with accumulator clamping for consistent jump heights and collision fairness regardless of frame rate.
- **critically-damped spring lane switching**: lateral lane transitions solve `a = k * (target_x - x) - d * vx` (`k = 275`, `d = 32.8`), producing snappy organic acceleration, athletic body banking (`lane_lean_`), and subtle camera turn roll (`cam_roll_`).
- **landing compression & parkour roll**: landing from jumps or bus roofs triggers a damped knee/hip compression spring (`landing_crouch_`). pressing down executes a full 360-degree tucked forward somersault (`roll_phase` `0 -> 1`) or fast-fall slam when airborne.
- **2-bone hierarchical animation**: `emit_character` computes pelvis twist, shoulder counter-rotation, thigh swing, anatomically backward-bending knees, forward-bending elbows, and head stabilization for the runner, traffic patrol chaser, and every pnj pedestrian.
- **rooftop running & collision fairness**: wooden hazard ramps smoothly elevate the runner onto runnable bus, truck, and taxi roofs. side-grazing a vehicle while switching lanes triggers a stumble bounce and brings the municipal traffic patrol chaser right behind you (a second stumble before recovery results in capture).

### 2.4 douala pnj pedestrian & vendor system (`world_gen.cpp`, `game_engine.cpp`)

- **sidewalk walkers (`npc_walker`)**: stroll in both directions along left and right sidewalks with natural walking gait cycles and colorful douala attire.
- **market vendors (`npc_vendor`)**: stand beside striped marche mboppi umbrellas facing the boulevard with animated gesturing.
- **bridge strollers (`npc_bridge_stroller`)**: walk along the elevated pedestrian walkways of pont du wouri.
- **proximity reaction**: when maximus sprints within `11 m`, nearby pnjs smoothly raise an arm to wave as you pass.

### 2.5 zero-copy python bindings (`bindings.cpp`)

each mesh's active instances are packed into contiguous 80-byte structs (`mat4 model` column-major + `vec4 tint_emissive`) and exposed to python via `py::memoryview::from_memory(...)`, letting `moderngl` upload directly to dynamic instance vbos without per-object python loops or heap copies.

---

## 3. python rendering, audio & ui layer (`game/`)

### 3.1 opengl 3.3 core renderer (`renderer.py`)

the renderer runs a 4-stage pipeline per frame:

1. **directional shadow pass**: renders shadow-casting instanced meshes into a `1024x1024` depth texture using front-face culling to minimize shadow acne.
2. **procedural sky dome pass**: renders tropical zenith-to-horizon gradients, glowing sun/moon disc with scattering halo, multi-octave fbm drifting clouds, and night stars (active only in bonanjo night).
3. **main 3d scene pass**:
   - applies a subtle subway-surfers curved horizon in screen space while evaluating shadow lookup in uncurved world space for pixel-accurate shadow alignment.
   - samples a **9-tap (3x3) pcf soft shadow kernel**.
   - shades 8 material types: matte micro-grain, glossy metallic/gold, self-lit emissive, rain-wet specular asphalt, night-lit building windows, floodlit billboards, animated shimmering wouri river waves, and additive translucent headlight cones.
   - applies **aces filmic tone mapping**, height-based grounding ambient occlusion, and smooth tropical distance fog.
4. **hardware 2d hud pass**: composites the alpha-blended `pygame` ui surface onto a fullscreen quad (uploaded at 60 hz in menus/transitions and 30 hz during fast gameplay for peak gpu throughput).

### 3.2 gta 4 animation style & legal flow (`hud.py`, `legal_content.py`, `app.py`)

- **startup sequence**:
  1. `intro` phase 1 (`0.0s - 3.0s`): dark cinema letterbox bars, angled amber-noir parallax frame, slow ken burns drift of `douala run`, and rotating bottom-right gta 4 loading ring.
  2. diagonal amber-noir shutter wipe transition.
  3. `intro` phase 2 (`3.0s - 6.8s`): illustrated gta 4 amber-noir portrait card (`load_gta4_portrait`) of `pondi ashley rey maxim` gliding smoothly beside staggered lowercase credits.
  4. `legal_gate`: mandatory bilingual privacy policy (`politique de confidentialite`) and legal/eula notice (`document juridique`). both checkboxes must be ticked to enable `accept & enter`. declining cleanly exits.
  5. `title`: main menu featuring the creator profile portrait header bar at the top of the game, `about us` / programmer biography button, level selector, garage shop, and controls.
- **strict text policy**: `clean_ui_text` enforces lowercase, accent-free english/french text across all ui surfaces.

### 3.3 procedural textures & audio (`textures.py`, `audio.py`)

- **textures**: generated dynamically at startup with `pillow` and `numpy` - weathered 3-lane douala asphalt with tire-wear tracks and dashed markings, building facades with alpha-encoded night-lit window panes and ac units, and douala billboards.
- **audio**: synthesized purely in `numpy` and played via `pygame.mixer` - a 4-bar 122 bpm makossa/afrobeat-inspired groove (syncopated kick, clave rimshot, shaker, bassline, and balafon melody) plus pentatonic ascending coin chimes, double-beep yellow taxi horns, traffic whistles, jump/roll whooshes, and victory fanfares.

---

## 4. gameplay systems & progression (`levels.py`, `save_manager.py`)

### 4.1 power-ups

- **fcfa coin magnet**: attracts coins across all 3 lanes directly to maximus.
- **turbo sneakers**: increases jump velocity (`13.9 -> 18.4 m/s`) to vault onto bus and cargo truck roofs directly from street level.
- **douala sky jetpack**: launches maximus above the skyline (`y = 6.2 m`) along a high-altitude coin ribbon.
- **2x score multiplier**: doubles distance and coin score gain (stacks with streak multiplier).
- **wouri hoverboard shield**: activated anytime with `space` / `e` / `shift` for 15 seconds; absorbing a crash blasts clear nearby obstacles with 1.8 s invulnerability.

### 4.2 persistence & garage upgrades (`save_manager.py`)

saved locally in `maximus_save.json` (100% offline):
- unlocked campaign districts (`1..5`, plus a `free play / unlock all` toggle on level select).
- 1-to-3 star ratings per district (1 star for finishing, 2 stars for coin target, 3 stars for high coins with zero stumbles).
- virtual fcfa bank used in the garage shop to upgrade power-up durations (levels 1-5), refill hoverboard packs, and unlock 4 runner outfits (`lion indomptable`, `akwa street`, `wouri gold`, `bonanjo night`).

---

## 5. building, testing & troubleshooting

### manual rebuild of the c++17 engine

```bash
python3 build.py --force
```

the compiler command can be overridden via the `cxx` environment variable:

```bash
cxx=g++ python3 build.py --force
```

### running verification tests

```bash
python3 tests/test_engine.py          # verifies all 39 meshes, physics, pnj spawns, difficulty, legal & text rules
python3 tests/test_window_launch.py   # verifies live sdl2/opengl 3.3 flow: intro -> legal gate -> title -> about -> play
python3 tests/capture_screenshots.py  # renders 1280x720 screenshots to screenshots/ and benchmarks gpu fps
```

### troubleshooting

- **missing compiler error**: install xcode command line tools on macos (`xcode-select --install`) or `build-essential` on ubuntu/debian.
- **resetting save or legal acceptance**: delete `maximus_save.json` in the repository root and relaunch `python3 main.py`.
- **audio unavailable on headless systems**: `audio.py` gracefully degrades to silent mode without crashing if no audio device is present.
