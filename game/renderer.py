"""
Modern OpenGL 3.3 Core Profile 3D Renderer (ModernGL) for Maximus: Douala Runner 3D.

Features:
- Procedural Sky Dome with tropical gradient, sun/moon corona, and drifting clouds
- Real-time directional Shadow Mapping (1024x1024 depth texture with 4-tap PCF soft shadows)
- Subtle Subway Surfers curved-world horizon shader (shadow-consistent!)
- Hardware instanced rendering fed zero-copy from the C++17 engine
- Multi-material PBR-lite shading (glossy taxi paint, rain-slicked wet asphalt,
  night-lit Douala building windows, emissive headlights/coins, animated Wouri River)
- Hardware alpha-composited 2D HUD quad
"""

from __future__ import annotations

import math
import struct
from typing import List, Optional, Tuple

import moderngl
import numpy as np

from .levels import ThemeConfig
from .textures import (
    generate_billboard_texture,
    generate_facade_texture,
    generate_road_texture,
)

# ---------------------------------------------------------------------------
# GLSL 330 Core Shaders
# ---------------------------------------------------------------------------

SKY_VERT = """
#version 330 core
in vec2 in_pos;
out vec2 v_uv;
void main() {
    v_uv = in_pos * 0.5 + 0.5;
    gl_Position = vec4(in_pos, 0.9999, 1.0);
}
"""

SKY_FRAG = """
#version 330 core
in vec2 v_uv;
out vec4 frag_color;

uniform vec3 u_sky_top;
uniform vec3 u_sky_horizon;
uniform vec3 u_sun_dir;
uniform vec3 u_sun_disc;
uniform float u_time;
uniform float u_aspect;
uniform float u_night;

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; ++i) {
        v += a * noise(p);
        p = p * 2.04 + vec2(17.1, 9.2);
        a *= 0.5;
    }
    return v;
}

void main() {
    // Reconstruct approximate forward-looking view ray for Douala horizon sky
    vec2 ndc = v_uv * 2.0 - 1.0;
    vec3 dir = normalize(vec3(ndc.x * u_aspect * 0.58, ndc.y * 0.58 + 0.06, -1.0));

    float h = clamp(dir.y * 1.45 + 0.14, 0.0, 1.0);
    vec3 sky = mix(u_sky_horizon, u_sky_top, pow(h, 0.72));

    // Sun / Moon disc & warm tropical scattering halo
    vec3 sdir = normalize(u_sun_dir);
    float sun_dot = max(dot(dir, sdir), 0.0);
    float halo = pow(sun_dot, 8.0) * 0.36 + pow(sun_dot, 64.0) * 0.55;
    float disc = smoothstep(0.9965, 0.9988, sun_dot);
    sky += u_sun_disc * (halo * (1.0 - 0.55 * u_night) + disc * 1.35);

    // Tropical drifting clouds above horizon
    if (dir.y > 0.01) {
        vec2 cuv = dir.xz / (dir.y + 0.24) * 1.15 + vec2(u_time * 0.018, -u_time * 0.012);
        float c = fbm(cuv);
        float cloud_mask = smoothstep(0.48, 0.76, c) * smoothstep(0.01, 0.22, dir.y) * (1.0 - smoothstep(0.65, 0.95, dir.y));
        vec3 cloud_col = mix(vec3(1.0, 0.97, 0.94), u_sky_horizon * 1.15, 0.35);
        cloud_col = mix(cloud_col, vec3(0.18, 0.24, 0.38), u_night * 0.85);
        sky = mix(sky, cloud_col, cloud_mask * 0.62);
    }

    // Subtle starry glint on clear upper night sky (Bonanjo Night only, never sunset!)
    if (u_night > 0.78 && dir.y > 0.16) {
        float st = hash21(floor(v_uv * vec2(260.0, 150.0)));
        if (st > 0.9965) {
            sky += vec3(0.75, 0.85, 1.0) * (u_night - 0.78) * 2.2;
        }
    }

    frag_color = vec4(sky, 1.0);
}
"""

SHADOW_VERT = """
#version 330 core
in vec3 in_pos;
in vec4 in_m0;
in vec4 in_m1;
in vec4 in_m2;
in vec4 in_m3;

uniform mat4 u_light_vp;

void main() {
    mat4 model = mat4(in_m0, in_m1, in_m2, in_m3);
    vec4 world = model * vec4(in_pos, 1.0);
    gl_Position = u_light_vp * world;
}
"""

SHADOW_FRAG = """
#version 330 core
void main() {}
"""

SCENE_VERT = """
#version 330 core
in vec3 in_pos;
in vec3 in_norm;
in vec2 in_uv;
in vec3 in_col;
in float in_mat;

in vec4 in_m0;
in vec4 in_m1;
in vec4 in_m2;
in vec4 in_m3;
in vec4 in_tint;

uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat4 u_light_vp;
uniform vec3 u_cam_pos;
uniform vec2 u_curve;

out vec3 v_world_pos;
out vec3 v_normal;
out vec2 v_uv;
out vec3 v_color;
out float v_mat;
out float v_emissive_boost;
out vec4 v_light_space;
out float v_cam_dist;

void main() {
    mat4 model = mat4(in_m0, in_m1, in_m2, in_m3);
    vec4 world = model * vec4(in_pos, 1.0);
    v_world_pos = world.xyz;
    v_light_space = u_light_vp * world;

    // Subtle Subway Surfers curved horizon (only bends screen projection so shadows match 100%!)
    float dz = max(0.0, u_cam_pos.z - world.z);
    vec4 curved = world;
    curved.x += u_curve.x * dz * dz;
    curved.y += u_curve.y * dz * dz;

    mat3 norm_mat = mat3(model);
    v_normal = normalize(norm_mat * in_norm);
    v_uv = in_uv;
    v_color = in_col * in_tint.rgb;
    v_mat = in_mat;
    v_emissive_boost = in_tint.a;
    v_cam_dist = length(world.xyz - u_cam_pos);

    gl_Position = u_proj * u_view * curved;
}
"""

SCENE_FRAG = """
#version 330 core
in vec3 v_world_pos;
in vec3 v_normal;
in vec2 v_uv;
in vec3 v_color;
in float v_mat;
in float v_emissive_boost;
in vec4 v_light_space;
in float v_cam_dist;

uniform sampler2D u_tex_road;
uniform sampler2D u_tex_facade;
uniform sampler2D u_tex_billboard;
uniform sampler2DShadow u_shadow_map;

uniform vec3 u_cam_pos;
uniform vec3 u_sun_dir;
uniform vec3 u_sun_color;
uniform vec3 u_ambient_sky;
uniform vec3 u_ambient_ground;
uniform vec3 u_fog_color;
uniform float u_fog_start;
uniform float u_fog_end;
uniform float u_wetness;
uniform float u_night_windows;
uniform float u_time;

out vec4 frag_color;

float sample_shadow(vec4 lspace, float ndotl) {
    vec3 proj = lspace.xyz / lspace.w;
    proj = proj * 0.5 + 0.5;
    if (proj.x < 0.01 || proj.x > 0.99 || proj.y < 0.01 || proj.y > 0.99 || proj.z > 0.99) {
        return 1.0;
    }
    float bias = max(0.0024 * (1.0 - ndotl), 0.0007);
    float z = proj.z - bias;
    vec2 texel = vec2(1.25 / 1024.0);
    float vis = 0.0;
    // 9-tap 3x3 smooth PCF soft shadow kernel
    for (int ox = -1; ox <= 1; ++ox) {
        for (int oy = -1; oy <= 1; ++oy) {
            vis += texture(u_shadow_map, vec3(proj.xy + vec2(ox, oy) * texel, z));
        }
    }
    return vis / 9.0;
}

vec3 aces_tonemap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(u_sun_dir);
    vec3 V = normalize(u_cam_pos - v_world_pos);
    vec3 H = normalize(L + V);

    vec3 albedo = v_color;
    float spec_power = 24.0;
    float spec_strength = 0.14;
    vec3 emissive = vec3(0.0);

    int mat_id = int(v_mat + 0.5);

    if (mat_id == 7) {
        // Soft translucent headlight beam cone on asphalt!
        float edge_x = 1.0 - abs(v_uv.x * 2.0 - 1.0);
        float fade_z = pow(1.0 - clamp(v_uv.y, 0.0, 1.0), 1.6);
        float beam_a = smoothstep(0.0, 0.45, edge_x) * fade_z * (0.28 + v_emissive_boost * 0.35);
        frag_color = vec4(v_color * 1.35, clamp(beam_a, 0.0, 0.65));
        return;
    }

    if (mat_id == 1) {
        // Glossy automotive paint / metallic gold coins / chrome / glass
        spec_power = 72.0;
        spec_strength = 0.74;
        emissive = albedo * (v_emissive_boost * 0.24);
    } else if (mat_id == 0) {
        // Subtle tactile procedural micro-detail so matte surfaces look realistic & grounded
        float micro = sin(v_world_pos.x * 9.0) * cos(v_world_pos.z * 9.0) * 0.025;
        albedo *= (1.0 + micro);
    } else if (mat_id == 2) {
        // Self-lit emissive (headlights, coins, taxi signs, streetlamps, power-ups)
        emissive = albedo * (0.92 + v_emissive_boost * 0.55);
        spec_power = 48.0;
        spec_strength = 0.45;
    } else if (mat_id == 3) {
        // Douala boulevard asphalt texture with rain-wet reflective puddles!
        vec4 road_s = texture(u_tex_road, v_uv);
        albedo = road_s.rgb * v_color;
        if (u_wetness > 0.05) {
            float ripple = sin(v_world_pos.x * 1.4 + u_time * 2.0) * cos(v_world_pos.z * 0.7 - u_time * 1.5);
            float wet = u_wetness * (0.72 + 0.28 * ripple);
            albedo *= (1.0 - 0.34 * wet);
            spec_power = mix(24.0, 96.0, wet);
            spec_strength = mix(0.14, 0.95, wet);
            // Warm neon/streetlamp reflection shimmer on wet Bonanjo asphalt!
            float lane_glow = exp(-abs(v_world_pos.x) * 0.16) * wet * u_night_windows;
            emissive += vec3(0.18, 0.22, 0.34) * lane_glow;
        }
    } else if (mat_id == 4) {
        // Douala building facade with warm night-lit window panes!
        vec4 fac = texture(u_tex_facade, v_uv);
        albedo = fac.rgb * v_color;
        if (fac.a < 0.45) {
            // Lit window pane at dusk / night!
            vec3 warm_win = vec3(1.0, 0.80, 0.40);
            float glow = clamp(u_night_windows + v_emissive_boost * 0.5, 0.0, 1.0);
            emissive = warm_win * glow * 1.15;
            spec_power = 64.0;
            spec_strength = 0.55;
        } else if (fac.a < 0.85) {
            spec_power = 48.0;
            spec_strength = 0.45;
        }
    } else if (mat_id == 5) {
        // Vibrant Douala street billboard texture
        vec4 bb = texture(u_tex_billboard, v_uv);
        albedo = bb.rgb;
        emissive = bb.rgb * (0.16 + u_night_windows * 0.62 + v_emissive_boost * 0.25);
    } else if (mat_id == 6) {
        // Animated shimmering Wouri River water (deep coastal teal with sunset sun-path glints!)
        float w1 = sin(v_world_pos.x * 0.55 + u_time * 2.4 + v_world_pos.z * 0.28);
        float w2 = cos(v_world_pos.x * 0.30 - u_time * 1.9 + v_world_pos.z * 0.44);
        N = normalize(vec3((w1 + w2) * 0.11, 1.0, (w1 - w2) * 0.11));
        float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.4);
        vec3 deep_river = vec3(0.03, 0.18, 0.30);
        vec3 shallow_wave = vec3(0.08, 0.34, 0.46);
        albedo = mix(deep_river, shallow_wave, 0.45 + 0.35 * w1);
        albedo = mix(albedo, u_sun_color * 0.68, fresnel * 0.45);
        spec_power = 128.0;
        spec_strength = 1.65;
    }

    float ndotl = max(dot(N, L), 0.0);
    float shadow = (mat_id == 2 || mat_id == 6) ? 1.0 : sample_shadow(v_light_space, ndotl);
    shadow = mix(0.28, 1.0, shadow);

    // Subtle height-based grounding ambient occlusion near street/sidewalk contact
    float ground_ao = (mat_id == 3 || mat_id == 6 || mat_id == 2) ? 1.0 : mix(0.78, 1.0, smoothstep(0.0, 0.65, v_world_pos.y));

    // Hemisphere tropical sky/laterite ambient light
    float hemi = N.y * 0.5 + 0.5;
    vec3 ambient = mix(u_ambient_ground, u_ambient_sky, hemi) * ground_ao;

    // Blinn-Phong specular highlight + Fresnel rim light for realistic 3D depth
    float spec = pow(max(dot(N, H), 0.0), spec_power) * spec_strength * shadow;
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0) * (mat_id == 1 ? 0.28 : 0.16);

    vec3 color = albedo * (ambient + u_sun_color * ndotl * shadow)
               + u_sun_color * spec
               + u_ambient_sky * rim
               + emissive;

    // Smooth tropical atmospheric distance fog
    float fog_t = clamp((v_cam_dist - u_fog_start) / max(1.0, (u_fog_end - u_fog_start)), 0.0, 1.0);
    fog_t = fog_t * fog_t * (3.0 - 2.0 * fog_t);
    color = mix(color, u_fog_color, fog_t);

    // ACES Filmic tone mapping + cinematic GTA IV warm contrast grade
    color = aces_tonemap(color * 1.18);
    color = pow(color, vec3(1.0 / 1.08));

    frag_color = vec4(color, 1.0);
}
"""

HUD_VERT = """
#version 330 core
in vec2 in_pos;
out vec2 v_uv;
void main() {
    v_uv = vec2(in_pos.x * 0.5 + 0.5, 0.5 - in_pos.y * 0.5);
    gl_Position = vec4(in_pos, 0.0, 1.0);
}
"""

HUD_FRAG = """
#version 330 core
in vec2 v_uv;
uniform sampler2D u_hud_tex;
out vec4 frag_color;
void main() {
    vec4 c = texture(u_hud_tex, v_uv);
    if (c.a < 0.005) discard;
    frag_color = c;
}
"""


def _look_at_col_major(eye: Tuple[float, float, float],
                       target: Tuple[float, float, float],
                       up: Tuple[float, float, float] = (0.0, 1.0, 0.0)) -> np.ndarray:
    e = np.array(eye, dtype=np.float32)
    t = np.array(target, dtype=np.float32)
    u = np.array(up, dtype=np.float32)
    f = t - e
    f /= max(1e-6, float(np.linalg.norm(f)))
    s = np.cross(f, u)
    s /= max(1e-6, float(np.linalg.norm(s)))
    u2 = np.cross(s, f)
    m = np.eye(4, dtype=np.float32)
    m[0, 0:3] = s
    m[1, 0:3] = u2
    m[2, 0:3] = -f
    m[0, 3] = -float(np.dot(s, e))
    m[1, 3] = -float(np.dot(u2, e))
    m[2, 3] = float(np.dot(f, e))
    return m


def _ortho_col_major(l: float, r: float, b: float, t: float, n: float, f: float) -> np.ndarray:
    m = np.zeros((4, 4), dtype=np.float32)
    m[0, 0] = 2.0 / (r - l)
    m[1, 1] = 2.0 / (t - b)
    m[2, 2] = -2.0 / (f - n)
    m[0, 3] = -(r + l) / (r - l)
    m[1, 3] = -(t + b) / (t - b)
    m[2, 3] = -(f + n) / (f - n)
    m[3, 3] = 1.0
    return m


class MeshBatch:
    def __init__(self, ctx: moderngl.Context, scene_prog: moderngl.Program,
                 shadow_prog: moderngl.Program, vert_bytes: bytes, idx_bytes: bytes,
                 max_instances: int = 512) -> None:
        self.vbo = ctx.buffer(vert_bytes)
        self.ibo = ctx.buffer(idx_bytes)
        self.max_instances = max_instances
        self.inst_vbo = ctx.buffer(reserve=max_instances * 80, dynamic=True)

        self.scene_vao = ctx.vertex_array(
            scene_prog,
            [
                (self.vbo, "3f 3f 2f 3f 1f", "in_pos", "in_norm", "in_uv", "in_col", "in_mat"),
                (self.inst_vbo, "4f 4f 4f 4f 4f /i", "in_m0", "in_m1", "in_m2", "in_m3", "in_tint"),
            ],
            index_buffer=self.ibo,
        )
        self.shadow_vao = ctx.vertex_array(
            shadow_prog,
            [
                (self.vbo, "3f 36x", "in_pos"),
                (self.inst_vbo, "4f 4f 4f 4f 16x /i", "in_m0", "in_m1", "in_m2", "in_m3"),
            ],
            index_buffer=self.ibo,
        )
        self.active_count = 0

    def upload_instances(self, count: int, mem_view) -> None:
        self.active_count = count
        if count <= 0:
            return
        needed = count * 80
        if needed > self.inst_vbo.size:
            self.inst_vbo.orphan(needed * 2)
        self.inst_vbo.write(mem_view)

    def release(self) -> None:
        self.scene_vao.release()
        self.shadow_vao.release()
        self.vbo.release()
        self.ibo.release()
        self.inst_vbo.release()


class DoualaRenderer:
    SHADOW_SIZE = 1024

    def __init__(self, ctx: moderngl.Context, engine, eng_mod, width: int, height: int) -> None:
        self.ctx = ctx
        self.eng_mod = eng_mod
        self.width = width
        self.height = height
        self.time = 0.0

        self.ctx.enable(moderngl.DEPTH_TEST | moderngl.CULL_FACE)
        self.ctx.front_face = "ccw"

        # Compile shaders
        self.sky_prog = ctx.program(vertex_shader=SKY_VERT, fragment_shader=SKY_FRAG)
        self.shadow_prog = ctx.program(vertex_shader=SHADOW_VERT, fragment_shader=SHADOW_FRAG)
        self.scene_prog = ctx.program(vertex_shader=SCENE_VERT, fragment_shader=SCENE_FRAG)
        self.hud_prog = ctx.program(vertex_shader=HUD_VERT, fragment_shader=HUD_FRAG)

        # Fullscreen quad for Sky Dome & 2D HUD overlay
        quad_data = struct.pack("8f", -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0)
        self.quad_vbo = ctx.buffer(quad_data)
        self.sky_vao = ctx.vertex_array(self.sky_prog, [(self.quad_vbo, "2f", "in_pos")])
        self.hud_vao = ctx.vertex_array(self.hud_prog, [(self.quad_vbo, "2f", "in_pos")])

        # Procedural textures
        self.tex_road = self._create_texture(512, 512, generate_road_texture(512), repeat=True)
        self.tex_facade = self._create_texture(512, 512, generate_facade_texture(512), repeat=True)
        self.tex_billboard = self._create_texture(512, 256, generate_billboard_texture(512, 256), repeat=False)

        # Shadow depth texture & FBO
        self.shadow_depth = ctx.depth_texture((self.SHADOW_SIZE, self.SHADOW_SIZE))
        self.shadow_depth.compare_func = "<="
        self.shadow_depth.filter = (moderngl.LINEAR, moderngl.LINEAR)
        self.shadow_depth.repeat_x = False
        self.shadow_depth.repeat_y = False
        self.shadow_fbo = ctx.framebuffer(depth_attachment=self.shadow_depth)

        # Dynamic HUD RGBA texture (updated when pygame UI draws)
        self.hud_tex: Optional[moderngl.Texture] = None
        self._ensure_hud_texture(width, height)

        # Bind static sampler slots
        self.scene_prog["u_tex_road"].value = 0
        self.scene_prog["u_tex_facade"].value = 1
        self.scene_prog["u_tex_billboard"].value = 2
        self.scene_prog["u_shadow_map"].value = 3
        self.hud_prog["u_hud_tex"].value = 4

        # Upload all 36 procedural C++ meshes to GPU VBOs/IBOs
        self.batches: List[MeshBatch] = []
        self.no_shadow_meshes = {
            eng_mod.MESH_WATER_PLANE,
            eng_mod.MESH_HEADLIGHT_BEAM,
            eng_mod.MESH_PARTICLE_CUBE,
            eng_mod.MESH_RAIN_STREAK,
            eng_mod.MESH_SHADOW_BLOB,
        }
        self.translucent_meshes = {eng_mod.MESH_HEADLIGHT_BEAM}
        for mid in range(eng_mod.MESH_COUNT):
            vb, ib, _vc, _ic = engine.get_mesh_buffers(mid)
            self.batches.append(MeshBatch(ctx, self.scene_prog, self.shadow_prog, vb, ib))

    def _create_texture(self, w: int, h: int, rgba_bytes: bytes, repeat: bool) -> moderngl.Texture:
        tex = self.ctx.texture((w, h), 4, rgba_bytes)
        tex.build_mipmaps()
        tex.filter = (moderngl.LINEAR_MIPMAP_LINEAR, moderngl.LINEAR)
        tex.anisotropy = min(8.0, float(self.ctx.info.get("GL_MAX_TEXTURE_MAX_ANISOTROPY", 4.0)))
        tex.repeat_x = repeat
        tex.repeat_y = repeat
        return tex

    def _ensure_hud_texture(self, w: int, h: int) -> None:
        if self.hud_tex is not None and self.hud_tex.size == (w, h):
            return
        if self.hud_tex is not None:
            self.hud_tex.release()
        self.hud_tex = self.ctx.texture((w, h), 4)
        self.hud_tex.filter = (moderngl.LINEAR, moderngl.LINEAR)

    def resize(self, width: int, height: int) -> None:
        self.width = max(1, width)
        self.height = max(1, height)
        self._ensure_hud_texture(self.width, self.height)

    def _compute_light_vp(self, focus: Tuple[float, float, float],
                          sun_dir: Tuple[float, float, float]) -> bytes:
        s = np.array(sun_dir, dtype=np.float32)
        s /= max(1e-6, float(np.linalg.norm(s)))
        # Snap focus along track gently to avoid shadow texel shimmer
        fx, fy, fz = focus[0], 0.0, math.floor(focus[2] * 0.5) * 2.0
        eye = (fx + float(s[0]) * 52.0, fy + float(s[1]) * 52.0, fz + float(s[2]) * 52.0)
        view = _look_at_col_major(eye, (fx, fy, fz))
        proj = _ortho_col_major(-32.0, 32.0, -34.0, 38.0, 4.0, 125.0)
        vp = proj @ view
        # Transpose to OpenGL column-major float32 bytes
        return np.ascontiguousarray(vp.T, dtype=np.float32).tobytes()

    def render_frame(self, engine, theme: ThemeConfig, dt: float,
                     hud_rgba: Optional[bytes] = None,
                     draw_hud: bool = True,
                     target_fbo: Optional[moderngl.Framebuffer] = None) -> None:
        self.time += dt
        snap = engine.snapshot()

        # Upload active C++ instanced matrices (zero-copy memoryviews!)
        for mid, batch in enumerate(self.batches):
            count, mv = engine.get_instance_view(mid)
            batch.upload_instances(count, mv)

        light_vp_bytes = self._compute_light_vp(snap.sun_focus, theme.sun_dir)

        # --- PASS 1: Directional Shadow Map ---
        self.shadow_fbo.use()
        self.shadow_fbo.clear(depth=1.0)
        self.ctx.viewport = (0, 0, self.SHADOW_SIZE, self.SHADOW_SIZE)
        self.ctx.enable(moderngl.DEPTH_TEST)
        self.ctx.disable(moderngl.BLEND)
        self.ctx.cull_face = "front"  # Reduces shadow acne cleanly!

        self.shadow_prog["u_light_vp"].write(light_vp_bytes)
        for mid, batch in enumerate(self.batches):
            if batch.active_count > 0 and mid not in self.no_shadow_meshes:
                batch.shadow_vao.render(instances=batch.active_count)

        # --- PASS 2: Main 3D Scene ---
        fbo = target_fbo if target_fbo is not None else self.ctx.screen
        fbo.use()
        vp_w, vp_h = fbo.size
        self.ctx.viewport = (0, 0, vp_w, vp_h)
        fbo.clear(theme.fog_color[0], theme.fog_color[1], theme.fog_color[2], 1.0, depth=1.0)

        # 2a. Procedural Tropical Sky Dome (no depth write)
        self.ctx.disable(moderngl.DEPTH_TEST)
        self.ctx.disable(moderngl.CULL_FACE)
        self.sky_prog["u_sky_top"].value = theme.sky_top
        self.sky_prog["u_sky_horizon"].value = theme.sky_horizon
        self.sky_prog["u_sun_dir"].value = theme.sun_dir
        self.sky_prog["u_sun_disc"].value = theme.sun_disc_color
        self.sky_prog["u_time"].value = self.time
        self.sky_prog["u_aspect"].value = self.width / max(1.0, float(self.height))
        self.sky_prog["u_night"].value = theme.night_windows
        self.sky_vao.render(moderngl.TRIANGLE_STRIP)

        # 2b. Instanced 3D Douala World
        self.ctx.enable(moderngl.DEPTH_TEST | moderngl.CULL_FACE)
        self.ctx.cull_face = "back"

        self.tex_road.use(0)
        self.tex_facade.use(1)
        self.tex_billboard.use(2)
        self.shadow_depth.use(3)

        self.scene_prog["u_view"].write(snap.view_bytes)
        self.scene_prog["u_proj"].write(snap.proj_bytes)
        self.scene_prog["u_light_vp"].write(light_vp_bytes)
        self.scene_prog["u_cam_pos"].value = snap.cam_pos
        self.scene_prog["u_curve"].value = (theme.curve_x, theme.curve_y)
        self.scene_prog["u_sun_dir"].value = theme.sun_dir
        self.scene_prog["u_sun_color"].value = theme.sun_color
        self.scene_prog["u_ambient_sky"].value = theme.ambient_sky
        self.scene_prog["u_ambient_ground"].value = theme.ambient_ground
        self.scene_prog["u_fog_color"].value = theme.fog_color
        self.scene_prog["u_fog_start"].value = theme.fog_start
        self.scene_prog["u_fog_end"].value = theme.fog_end
        self.scene_prog["u_wetness"].value = theme.wetness
        self.scene_prog["u_night_windows"].value = theme.night_windows
        self.scene_prog["u_time"].value = self.time

        for mid, batch in enumerate(self.batches):
            if batch.active_count > 0 and mid not in self.translucent_meshes:
                batch.scene_vao.render(instances=batch.active_count)

        # 2c. Soft translucent headlight cones (additive/alpha glow without depth write)
        self.ctx.enable(moderngl.BLEND)
        self.ctx.disable(moderngl.CULL_FACE)
        self.ctx.blend_func = (moderngl.SRC_ALPHA, moderngl.ONE)
        self.ctx.depth_mask = False
        for mid in self.translucent_meshes:
            batch = self.batches[mid]
            if batch.active_count > 0:
                batch.scene_vao.render(instances=batch.active_count)
        self.ctx.depth_mask = True
        self.ctx.disable(moderngl.BLEND)

        # --- PASS 3: Alpha-blended 2D HUD / Menu Overlay ---
        if self.hud_tex is not None and (hud_rgba is not None or draw_hud):
            if hud_rgba is not None:
                self.hud_tex.write(hud_rgba)
            self.hud_tex.use(4)
            self.ctx.disable(moderngl.DEPTH_TEST)
            self.ctx.disable(moderngl.CULL_FACE)
            self.ctx.enable(moderngl.BLEND)
            self.ctx.blend_func = (moderngl.SRC_ALPHA, moderngl.ONE_MINUS_SRC_ALPHA)
            self.hud_vao.render(moderngl.TRIANGLE_STRIP)
            self.ctx.disable(moderngl.BLEND)

    def release(self) -> None:
        for b in self.batches:
            b.release()
        self.sky_vao.release()
        self.hud_vao.release()
        self.quad_vbo.release()
        self.tex_road.release()
        self.tex_facade.release()
        self.tex_billboard.release()
        self.shadow_fbo.release()
        self.shadow_depth.release()
        if self.hud_tex is not None:
            self.hud_tex.release()
