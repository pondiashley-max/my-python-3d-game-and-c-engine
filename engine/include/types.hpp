#pragma once

#include "math3d.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace maximus {

// All procedurally generated 3D meshes built in C++ and instanced on GPU.
enum MeshId : int {
    MESH_ROAD_BOULEVARD = 0,
    MESH_BRIDGE_WOURI,
    MESH_WATER_PLANE,

    // Real-world Douala vehicles & obstacles
    MESH_TAXI_YELLOW,
    MESH_BENSKIN,
    MESH_COASTER_BUS,
    MESH_CARGO_TRUCK,
    MESH_RAMP_WOOD,
    MESH_BARRIER_LOW,
    MESH_BARRIER_HIGH,
    MESH_BARRIER_FULL,
    MESH_POTHOLE_TRENCH,
    MESH_PUSHCART_PLANTAIN,

    // Articulated characters & gear (2-bone realistic limbs + sneakers)
    MESH_CHAR_TORSO,
    MESH_CHAR_HEAD,
    MESH_CHAR_LIMB,
    MESH_CHAR_FORELIMB,
    MESH_CHAR_SHOE,
    MESH_HOVERBOARD,
    MESH_JETPACK,
    MESH_HEADLIGHT_BEAM,

    // Collectibles & power-ups
    MESH_COIN,
    MESH_PU_MAGNET,
    MESH_PU_SNEAKERS,
    MESH_PU_JETPACK,
    MESH_PU_MULTIPLIER,
    MESH_PU_HOVERBOARD,

    // Douala landmarks & roadside architecture
    MESH_BUILDING_MODERN,
    MESH_BUILDING_SHOP,
    MESH_MARKET_UMBRELLA,
    MESH_PALM_TREE,
    MESH_STREETLAMP,
    MESH_BILLBOARD,
    MESH_MONUMENT_LIBERTE,
    MESH_PORT_CRANE,
    MESH_FINISH_ARCH,

    // VFX
    MESH_PARTICLE_CUBE,
    MESH_RAIN_STREAK,
    MESH_SHADOW_BLOB,

    MESH_COUNT
};

// Vertex layout: 12 floats per vertex (48 bytes):
// pos(3), normal(3), uv(2), color(3), material(1)
// material encoding:
//   0.0 = standard matte/lit surface
//   1.0 = metallic/glossy specular
//   2.0 = emissive glow (headlights, taxi sign, neon, streetlamps, coins)
//   3.0 = asphalt road texture (atlas slot)
//   4.0 = facade/window texture
//   5.0 = Douala billboard / sign texture
//   6.0 = Wouri river animated water
struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
    float r, g, b;
    float mat;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

// Per-instance GPU data: 20 floats (80 bytes):
// mat4 model (16 floats, column-major) + tint_rgb_emissive (4 floats)
// tint.xyz multiplies vertex color; tint.w boosts emissive glow
struct InstanceData {
    float model[16];
    float tr, tg, tb, emissive;
};

enum ObstacleKind : int {
    OBS_TAXI_PARKED = 0,
    OBS_TAXI_MOVING,
    OBS_BENSKIN_MOVING,
    OBS_COASTER_BUS,
    OBS_CARGO_TRUCK,
    OBS_RAMP,
    OBS_BARRIER_LOW,
    OBS_BARRIER_HIGH,
    OBS_BARRIER_FULL,
    OBS_POTHOLE,
    OBS_PUSHCART
};

enum PowerUpKind : int {
    PU_NONE = 0,
    PU_MAGNET,
    PU_SNEAKERS,
    PU_JETPACK,
    PU_MULTIPLIER,
    PU_HOVERBOARD
};

enum SoundEvent : int {
    SFX_NONE = 0,
    SFX_COIN,
    SFX_JUMP,
    SFX_ROLL,
    SFX_LANE_SWITCH,
    SFX_POWERUP,
    SFX_CRASH,
    SFX_STUMBLE,
    SFX_HOVERBOARD_ON,
    SFX_HOVERBOARD_BREAK,
    SFX_NEAR_MISS,
    SFX_HORN,
    SFX_LEVEL_COMPLETE,
    SFX_MILESTONE
};

enum GameStatus : int {
    STATUS_DEMO = 0,     // Title screen live Douala flythrough / auto-run
    STATUS_PLAYING,
    STATUS_PAUSED,
    STATUS_GAME_OVER,
    STATUS_LEVEL_COMPLETE
};

// Configured per Douala level from Python
struct LevelParams {
    int level_id = 1;            // 1..5 for Douala districts, 6 for Endless Tour
    bool is_endless = false;
    bool is_bridge = false;      // Pont du Wouri mode
    bool is_market = false;      // Marché Mboppi dense market stalls
    bool has_monument = false;   // Rond-Point Deido Monument de la Nouvelle Liberté
    bool is_rainy = false;       // Bonanjo night rainstorm
    bool is_night = false;       // Emissive streetlamps, lit windows, headlight cones

    float target_distance = 650.0f;  // Meters to finish line (ignored if endless)
    float base_speed = 18.0f;        // m/s start speed
    float max_speed = 32.0f;         // m/s max speed
    float speed_accel = 0.018f;      // m/s gained per meter run
    float obstacle_density = 0.55f;  // 0.3 .. 1.0 pattern tightness
    float moving_traffic_chance = 0.25f;
    float ramp_combo_chance = 0.28f;
    float powerup_chance = 0.14f;
    uint64_t seed = 23701ULL;
};

struct UpgradeParams {
    float magnet_duration = 10.0f;
    float sneakers_duration = 10.0f;
    float jetpack_duration = 7.5f;
    float multiplier_duration = 12.0f;
    int starting_hoverboards = 2;
    int outfit_id = 0;  // 0=Cameroon Lion, 1=Akwa Street, 2=Wouri Gold, 3=Bonanjo Night
};

struct Obstacle {
    int id = 0;
    ObstacleKind kind = OBS_BARRIER_LOW;
    MeshId mesh = MESH_BARRIER_LOW;
    int lane = 1;           // 0, 1, 2
    float x = 0.0f;
    float y = 0.0f;         // base height
    float s = 0.0f;         // forward center along track
    float half_w = 1.0f;
    float height = 1.0f;
    float clearance_y = 0.0f; // >0 for overhead arch (slide under if player height < clearance_y)
    float half_len = 1.0f;
    float speed_s = 0.0f;   // negative for oncoming vehicles
    bool runnable_top = false;
    bool is_ramp = false;
    bool is_pothole = false;
    bool triggered_near_miss = false;
    bool triggered_horn = false;
    bool hit_side = false;
    float tint_r = 1.0f;
    float tint_g = 1.0f;
    float tint_b = 1.0f;
    float anim_phase = 0.0f;
};

struct Collectible {
    int id = 0;
    bool is_powerup = false;
    PowerUpKind powerup = PU_NONE;
    float x = 0.0f;
    float y = 1.0f;
    float s = 0.0f;
    bool collected = false;
    bool magnetized = false;
    float spin = 0.0f;
};

struct SceneryProp {
    MeshId mesh = MESH_BUILDING_MODERN;
    float x = 0.0f;
    float y = 0.0f;
    float s = 0.0f;
    float yaw = 0.0f;
    float sx = 1.0f;
    float sy = 1.0f;
    float sz = 1.0f;
    float tr = 1.0f;
    float tg = 1.0f;
    float tb = 1.0f;
    float emissive = 0.0f;
};

enum NpcRole : int {
    NPC_WALKER = 0,       // Sidewalk pedestrian walking smoothly along boulevard
    NPC_VENDOR,           // Street / market vendor gesturing beside stalls
    NPC_BRIDGE_STROLLER   // Pedestrian strolling along Pont du Wouri walkway
};

struct NpcEntity {
    int id = 0;
    NpcRole role = NPC_WALKER;
    float x = 0.0f;
    float y = 0.20f;
    float s = 0.0f;
    float speed_s = 0.0f;   // +/- walking speed along sidewalk
    float yaw = 0.0f;
    float walk_cycle = 0.0f;
    float wave_amount = 0.0f;
    float height_scale = 1.0f;
    Vec3 shirt_col{0.85f, 0.32f, 0.18f};
    Vec3 pants_col{0.18f, 0.24f, 0.38f};
    Vec3 skin_col{0.34f, 0.21f, 0.13f};
    Vec3 shoe_col{0.92f, 0.92f, 0.90f};
    Vec3 hat_col{0.96f, 0.78f, 0.14f};
};

struct Particle {
    Vec3 pos;   // world x, y, track s
    Vec3 vel;
    float size = 0.15f;
    float life = 0.0f;
    float max_life = 1.0f;
    float r = 1.0f, g = 0.85f, b = 0.2f;
    float emissive = 0.8f;
    bool gravity = true;
    bool is_rain = false;
};

struct HudSnapshot {
    int status = STATUS_DEMO;
    int level_id = 1;
    int endless_zone = 1; // 1..5 cycling districts in Endless mode
    float distance = 0.0f;
    float target_distance = 650.0f;
    float progress = 0.0f;
    float speed_mps = 18.0f;
    float speed_kmh = 64.8f;
    int score = 0;
    int coins = 0;
    int multiplier = 1;
    int combo_streak = 0;
    int near_misses = 0;
    int stumbles = 0;
    int hoverboards_left = 2;

    float magnet_timer = 0.0f;
    float magnet_max = 10.0f;
    float sneakers_timer = 0.0f;
    float sneakers_max = 10.0f;
    float jetpack_timer = 0.0f;
    float jetpack_max = 7.5f;
    float mult_timer = 0.0f;
    float mult_max = 12.0f;
    float hoverboard_timer = 0.0f;
    float hoverboard_max = 15.0f;

    float chaser_proximity = 0.0f; // 0 = far/safe, 1 = right behind!
    float camera_shake = 0.0f;
    float player_x = 0.0f;
    float player_y = 0.0f;
    float player_s = 0.0f;
    bool on_roof = false;
    bool is_rolling = false;

    // Camera matrices (16 floats column-major)
    float view_mat[16];
    float proj_mat[16];
    float cam_pos[3];
    float sun_focus[3];

    // Popups
    std::string popup_text;
    float popup_timer = 0.0f;
    std::string crash_reason;
};

}  // namespace maximus
