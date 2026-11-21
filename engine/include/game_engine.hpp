#pragma once

#include "mesh_builder.hpp"
#include "types.hpp"
#include "world_gen.hpp"
#include <array>
#include <string>
#include <vector>

namespace maximus {

struct InputCommands {
    bool left = false;
    bool right = false;
    bool jump = false;
    bool roll = false;
    bool hoverboard = false;
};

class GameEngine {
public:
    GameEngine();

    // Build and return all procedural 3D meshes once at startup.
    const std::array<MeshData, MESH_COUNT>& meshes() const { return meshes_; }

    // Configure & start a Douala level (or demo flythrough mode).
    void start_level(const LevelParams& params, const UpgradeParams& upgrades, bool demo_mode = false);
    void set_paused(bool paused);
    void set_aspect(float aspect) { aspect_ = clampf(aspect, 0.5f, 3.0f); }

    // Feed instantaneous edge-triggered inputs (from keyboard / controller).
    void push_input(const InputCommands& cmd);

    // Advance simulation by dt seconds (internally fixed-stepped for determinism).
    void step(float dt);

    // Snapshot for HUD, camera, and audio events.
    const HudSnapshot& snapshot() const { return snapshot_; }
    std::vector<int> consume_sound_events();

    // Per-mesh instanced GPU transforms built cleanly in C++ each frame.
    const std::vector<InstanceData>& mesh_instances(int mesh_id) const {
        return instances_[mesh_id];
    }

    // Introspection helpers for testing & verification
    int obstacle_count() const { return static_cast<int>(obstacles_.size()); }
    int collectible_count() const { return static_cast<int>(collectibles_.size()); }
    int particle_count() const { return static_cast<int>(particles_.size()); }
    int npc_count() const { return static_cast<int>(npcs_.size()); }
    int total_instance_count() const;

private:
    std::array<MeshData, MESH_COUNT> meshes_;
    LevelParams level_;
    UpgradeParams upgrades_;
    WorldGenerator world_gen_;
    FastRng rng_;

    GameStatus status_ = STATUS_DEMO;
    float aspect_ = 16.0f / 9.0f;
    float sim_time_ = 0.0f;
    float accumulator_ = 0.0f;

    // Player physical state (track coords: x=lateral, y=up, s=forward meters)
    int target_lane_ = 1;
    int prev_lane_ = 1;
    float player_x_ = 0.0f;
    float player_vx_ = 0.0f;
    float player_y_ = 0.0f;
    float player_vy_ = 0.0f;
    float player_s_ = 0.0f;
    float speed_s_ = 18.0f;
    float ground_y_ = 0.0f;
    bool grounded_ = true;
    bool on_roof_ = false;

    // Roll & jump state + realistic secondary biomechanics
    float roll_timer_ = 0.0f;
    float jump_buffer_ = 0.0f;
    float roll_buffer_ = 0.0f;
    int queued_lane_delta_ = 0;
    float lane_lean_ = 0.0f;
    float landing_crouch_ = 0.0f;
    float landing_crouch_vel_ = 0.0f;
    float jump_takeoff_blend_ = 0.0f;
    float run_cycle_ = 0.0f;
    float invuln_timer_ = 0.0f;

    // Power-up timers
    float magnet_timer_ = 0.0f;
    float sneakers_timer_ = 0.0f;
    float jetpack_timer_ = 0.0f;
    float mult_timer_ = 0.0f;
    float hoverboard_timer_ = 0.0f;
    int hoverboards_left_ = 2;

    // Scoring & chaser
    float score_accum_ = 0.0f;
    int bonus_score_ = 0;
    int coins_ = 0;
    int combo_streak_ = 0;
    float combo_decay_ = 0.0f;
    int near_misses_ = 0;
    int stumbles_ = 0;
    float chaser_proximity_ = 0.15f; // 0..1; starts visible at launch then falls back
    float chaser_s_offset_ = 3.2f;
    float chaser_x_ = 0.0f;
    float stumble_recovery_timer_ = 0.0f;

    // Camera dynamics (spring-damped banking & breathing)
    float cam_x_ = 0.0f;
    float cam_y_ = 4.6f;
    float cam_roll_ = 0.0f;
    float cam_shake_ = 0.0f;
    float popup_timer_ = 0.0f;
    std::string popup_text_;
    std::string crash_reason_;
    float last_milestone_s_ = 0.0f;

    // World entities
    int next_entity_id_ = 1;
    std::vector<Obstacle> obstacles_;
    std::vector<Collectible> collectibles_;
    std::vector<SceneryProp> scenery_;
    std::vector<NpcEntity> npcs_;
    std::vector<float> road_segments_;
    std::vector<bool> road_is_bridge_;
    std::vector<Particle> particles_;

    // Sound event queue & render batches
    std::vector<int> sound_events_;
    std::array<std::vector<InstanceData>, MESH_COUNT> instances_;
    HudSnapshot snapshot_;

    void fixed_update(float dt);
    void update_demo_ai(float dt);
    void update_player(float dt);
    void update_npcs(float dt);
    void update_obstacles_and_collisions(float dt);
    void update_collectibles(float dt);
    void update_particles(float dt);
    void update_camera(float dt);
    void build_render_instances();
    void update_snapshot();

    void emit_instance(MeshId id, const Mat4& model, float r = 1.0f, float g = 1.0f, float b = 1.0f,
                       float emissive = 0.0f);
    void emit_character(float x, float y, float s, float yaw_face, float cycle, float stride_amp,
                        float lean, float crouch, float roll_phase, bool rolling, bool jumping,
                        bool on_hoverboard, bool jetpacking, float wave_amt, float height_scale,
                        const Vec3& jersey_col, const Vec3& shorts_col, const Vec3& skin_col,
                        const Vec3& shoe_col, const Vec3& cap_col, bool is_chaser, bool is_npc);

    void trigger_popup(const std::string& text, float duration = 1.35f);
    void trigger_stumble(const std::string& reason);
    void trigger_crash(const std::string& reason);
    void activate_hoverboard();
    void spawn_burst(Vec3 pos, int count, Vec3 color, float speed, float size, float life,
                     float emissive = 1.0f);
    void spawn_jetpack_coins();
    void push_sfx(SoundEvent ev);
};

}  // namespace maximus
