#include "game_engine.hpp"
#include <algorithm>
#include <cmath>

namespace maximus {

namespace {

constexpr float FIXED_DT = 1.0f / 120.0f;
constexpr float GRAVITY = -38.0f;
constexpr float JUMP_VEL = 13.9f;
constexpr float SNEAKERS_JUMP_VEL = 18.4f;
constexpr float FAST_FALL_VEL = -29.0f;
constexpr float PLAYER_STAND_H = 1.72f;
constexpr float PLAYER_ROLL_H = 0.78f;
constexpr float PLAYER_HALF_W = 0.34f;
constexpr float PLAYER_HALF_LEN = 0.36f;
constexpr float ROLL_DURATION = 0.68f;

inline Vec3 to_gl_pos(float x, float y, float s) {
    return {x, y, -s};
}

}  // namespace

GameEngine::GameEngine() : meshes_(MeshBuilder::build_all_meshes()) {
    LevelParams demo_lvl;
    demo_lvl.level_id = 1;
    demo_lvl.is_endless = true;
    demo_lvl.base_speed = 19.0f;
    demo_lvl.max_speed = 24.0f;
    demo_lvl.obstacle_density = 0.45f;
    demo_lvl.moving_traffic_chance = 0.28f;
    demo_lvl.ramp_combo_chance = 0.32f;
    demo_lvl.seed = 23799ULL;
    UpgradeParams def_upg;
    start_level(demo_lvl, def_upg, true);
}

void GameEngine::start_level(const LevelParams& params, const UpgradeParams& upgrades, bool demo_mode) {
    level_ = params;
    upgrades_ = upgrades;
    status_ = demo_mode ? STATUS_DEMO : STATUS_PLAYING;
    rng_.seed(params.seed ^ 0xA5A55A5AULL);

    sim_time_ = 0.0f;
    accumulator_ = 0.0f;

    target_lane_ = 1;
    prev_lane_ = 1;
    player_x_ = LANE_X[1];
    player_vx_ = 0.0f;
    player_y_ = 0.0f;
    player_vy_ = 0.0f;
    player_s_ = 0.0f;
    speed_s_ = level_.base_speed;
    ground_y_ = 0.0f;
    grounded_ = true;
    on_roof_ = false;

    roll_timer_ = 0.0f;
    jump_buffer_ = 0.0f;
    roll_buffer_ = 0.0f;
    queued_lane_delta_ = 0;
    lane_lean_ = 0.0f;
    landing_crouch_ = 0.0f;
    landing_crouch_vel_ = 0.0f;
    jump_takeoff_blend_ = 0.0f;
    run_cycle_ = 0.0f;
    invuln_timer_ = 0.0f;

    magnet_timer_ = 0.0f;
    sneakers_timer_ = 0.0f;
    jetpack_timer_ = 0.0f;
    mult_timer_ = 0.0f;
    hoverboard_timer_ = 0.0f;
    hoverboards_left_ = std::max(0, upgrades_.starting_hoverboards);

    score_accum_ = 0.0f;
    bonus_score_ = 0;
    coins_ = 0;
    combo_streak_ = 0;
    combo_decay_ = 0.0f;
    near_misses_ = 0;
    stumbles_ = 0;

    chaser_proximity_ = demo_mode ? 0.0f : 0.48f;
    chaser_s_offset_ = 3.2f;
    chaser_x_ = 0.0f;
    stumble_recovery_timer_ = 3.5f;

    cam_x_ = 0.0f;
    cam_y_ = 4.6f;
    cam_roll_ = 0.0f;
    cam_shake_ = 0.0f;
    popup_timer_ = 0.0f;
    popup_text_.clear();
    crash_reason_.clear();
    last_milestone_s_ = 0.0f;

    next_entity_id_ = 1;
    obstacles_.clear();
    collectibles_.clear();
    scenery_.clear();
    npcs_.clear();
    road_segments_.clear();
    road_is_bridge_.clear();
    particles_.clear();
    sound_events_.clear();

    world_gen_.reset(level_);
    world_gen_.update(player_s_, speed_s_, obstacles_, collectibles_, scenery_, npcs_,
                      road_segments_, road_is_bridge_, next_entity_id_);

    if (!demo_mode) {
        trigger_popup("go! douala run / allez!", 1.4f);
    }

    update_camera(0.016f);
    build_render_instances();
    update_snapshot();
}

void GameEngine::set_paused(bool paused) {
    if (paused && status_ == STATUS_PLAYING) {
        status_ = STATUS_PAUSED;
    } else if (!paused && status_ == STATUS_PAUSED) {
        status_ = STATUS_PLAYING;
    }
    update_snapshot();
}

void GameEngine::push_input(const InputCommands& cmd) {
    if (status_ != STATUS_PLAYING) return;

    if (cmd.left) {
        int next = std::max(0, target_lane_ - 1);
        if (next != target_lane_) {
            prev_lane_ = target_lane_;
            target_lane_ = next;
            push_sfx(SFX_LANE_SWITCH);
        } else {
            lane_lean_ = -0.35f;
        }
    }
    if (cmd.right) {
        int next = std::min(2, target_lane_ + 1);
        if (next != target_lane_) {
            prev_lane_ = target_lane_;
            target_lane_ = next;
            push_sfx(SFX_LANE_SWITCH);
        } else {
            lane_lean_ = 0.35f;
        }
    }
    if (cmd.jump) {
        jump_buffer_ = 0.16f;
    }
    if (cmd.roll) {
        roll_buffer_ = 0.16f;
        if (!grounded_ && jetpack_timer_ <= 0.0f) {
            player_vy_ = std::min(player_vy_, FAST_FALL_VEL);
        }
    }
    if (cmd.hoverboard) {
        activate_hoverboard();
    }
}

void GameEngine::activate_hoverboard() {
    if (hoverboard_timer_ > 0.0f || hoverboards_left_ <= 0 || jetpack_timer_ > 0.0f) return;
    hoverboards_left_--;
    hoverboard_timer_ = 15.0f;
    push_sfx(SFX_HOVERBOARD_ON);
    trigger_popup("wouri hoverboard / bouclier!", 1.4f);
    spawn_burst({player_x_, player_y_ + 0.5f, player_s_}, 20, {0.20f, 1.0f, 0.55f}, 6.5f, 0.16f, 0.55f);
}

void GameEngine::step(float dt) {
    dt = clampf(dt, 0.0f, 0.08f);
    if (status_ == STATUS_PAUSED) {
        update_snapshot();
        return;
    }

    accumulator_ += dt;
    int substeps = 0;
    while (accumulator_ >= FIXED_DT && substeps < 8) {
        fixed_update(FIXED_DT);
        accumulator_ -= FIXED_DT;
        substeps++;
    }

    build_render_instances();
    update_snapshot();
}

void GameEngine::fixed_update(float dt) {
    sim_time_ += dt;

    if (cam_shake_ > 0.0f) {
        cam_shake_ = std::max(0.0f, cam_shake_ - dt * 2.8f);
    }
    if (popup_timer_ > 0.0f) {
        popup_timer_ = std::max(0.0f, popup_timer_ - dt);
    }

    if (status_ == STATUS_GAME_OVER || status_ == STATUS_LEVEL_COMPLETE) {
        update_npcs(dt);
        update_particles(dt);
        update_camera(dt);
        return;
    }

    if (status_ == STATUS_DEMO) {
        update_demo_ai(dt);
    }

    if (invuln_timer_ > 0.0f) invuln_timer_ = std::max(0.0f, invuln_timer_ - dt);
    if (magnet_timer_ > 0.0f) magnet_timer_ = std::max(0.0f, magnet_timer_ - dt);
    if (sneakers_timer_ > 0.0f) sneakers_timer_ = std::max(0.0f, sneakers_timer_ - dt);
    if (mult_timer_ > 0.0f) mult_timer_ = std::max(0.0f, mult_timer_ - dt);
    if (hoverboard_timer_ > 0.0f) hoverboard_timer_ = std::max(0.0f, hoverboard_timer_ - dt);

    if (jetpack_timer_ > 0.0f) {
        jetpack_timer_ = std::max(0.0f, jetpack_timer_ - dt);
        if (rng_.chance(0.85f)) {
            Particle p;
            p.pos = {player_x_ + rng_.range(-0.22f, 0.22f), player_y_ + 0.55f, player_s_ - 0.3f};
            p.vel = {rng_.range(-0.6f, 0.6f), rng_.range(-4.0f, -1.5f), -speed_s_ * 0.35f};
            p.size = rng_.range(0.14f, 0.26f);
            p.life = 0.0f;
            p.max_life = 0.32f;
            p.r = 1.0f; p.g = rng_.range(0.45f, 0.85f); p.b = 0.12f;
            p.gravity = false;
            particles_.push_back(p);
        }
    }

    if (combo_decay_ > 0.0f) {
        combo_decay_ -= dt;
        if (combo_decay_ <= 0.0f) combo_streak_ = 0;
    }

    float target_speed = std::min(level_.max_speed, level_.base_speed + player_s_ * level_.speed_accel);
    if (level_.is_endless) {
        target_speed = std::min(44.0f, level_.base_speed + player_s_ * level_.speed_accel);
    }
    if (jetpack_timer_ > 0.0f) {
        target_speed = std::max(target_speed * 1.42f, 34.0f);
    }
    speed_s_ = lerpf(speed_s_, target_speed, clampf(dt * 4.0f, 0.0f, 1.0f));

    float prev_s = player_s_;
    player_s_ += speed_s_ * dt;

    int active_mult = (mult_timer_ > 0.0f ? 2 : 1) * (1 + std::min(4, combo_streak_ / 10));
    score_accum_ += (player_s_ - prev_s) * 2.5f * static_cast<float>(active_mult);

    if (stumble_recovery_timer_ > 0.0f) {
        stumble_recovery_timer_ -= dt;
    } else if (chaser_proximity_ > 0.0f) {
        chaser_proximity_ = std::max(0.0f, chaser_proximity_ - dt * 0.22f);
    }
    float desired_chaser_offset = lerpf(8.5f, 1.75f, chaser_proximity_);
    chaser_s_offset_ = lerpf(chaser_s_offset_, desired_chaser_offset, clampf(dt * 5.0f, 0.0f, 1.0f));
    chaser_x_ = lerpf(chaser_x_, player_x_, clampf(dt * 8.0f, 0.0f, 1.0f));

    if (status_ == STATUS_PLAYING && level_.is_endless && player_s_ - last_milestone_s_ >= 500.0f) {
        last_milestone_s_ += 500.0f;
        bonus_score_ += 500;
        push_sfx(SFX_MILESTONE);
        trigger_popup("500m douala milestone / palier +500", 1.8f);
    }

    world_gen_.update(player_s_, speed_s_, obstacles_, collectibles_, scenery_, npcs_,
                      road_segments_, road_is_bridge_, next_entity_id_);

    update_player(dt);
    update_npcs(dt);
    update_obstacles_and_collisions(dt);
    update_collectibles(dt);
    update_particles(dt);
    update_camera(dt);

    if (status_ == STATUS_PLAYING && !level_.is_endless && player_s_ >= level_.target_distance) {
        status_ = STATUS_LEVEL_COMPLETE;
        bonus_score_ += 1000 + coins_ * 5;
        push_sfx(SFX_LEVEL_COMPLETE);
        trigger_popup("district cleared / quartier termine!", 3.0f);
        spawn_burst({player_x_, player_y_ + 1.5f, player_s_ + 1.0f}, 45, {1.0f, 0.84f, 0.12f}, 11.0f, 0.22f, 1.4f);
        spawn_burst({player_x_, player_y_ + 1.8f, player_s_ + 1.0f}, 35, {0.06f, 0.72f, 0.34f}, 10.0f, 0.20f, 1.3f);
    }
}

void GameEngine::update_demo_ai(float /*dt*/) {
    float best_clear = -1.0f;
    int best_lane = target_lane_;
    bool danger_current = false;

    for (const auto& o : obstacles_) {
        float ds = o.s - player_s_;
        if (ds < -1.0f || ds > 20.0f) continue;
        if (o.lane == target_lane_) {
            if (o.is_ramp) {
                danger_current = false;
                break;
            }
            if ((o.kind == OBS_BARRIER_LOW || o.is_pothole) && ds < 6.5f && ds > 1.2f) {
                jump_buffer_ = 0.1f;
                return;
            }
            if (o.kind == OBS_BARRIER_HIGH && ds < 6.5f && ds > 0.5f) {
                roll_buffer_ = 0.1f;
                return;
            }
            if (o.height > player_y_ + 0.4f) {
                danger_current = true;
            }
        }
    }

    if (danger_current) {
        for (int l = 0; l < 3; ++l) {
            float nearest = 100.0f;
            for (const auto& o : obstacles_) {
                if (o.lane != l) continue;
                float ds = o.s - player_s_;
                if (ds > -2.0f && ds < nearest && !o.is_ramp) nearest = ds;
            }
            if (nearest > best_clear) {
                best_clear = nearest;
                best_lane = l;
            }
        }
        target_lane_ = best_lane;
    }
}

void GameEngine::update_player(float dt) {
    // Critically-damped spring lateral movement for realistic athletic weight & fluidity!
    float target_x = LANE_X[target_lane_];
    float spring_k = 275.0f;
    float spring_d = 32.8f;  // ~critical damping (2 * sqrt(k))
    float ax = spring_k * (target_x - player_x_) - spring_d * player_vx_;
    player_vx_ += ax * dt;
    player_x_ += player_vx_ * dt;
    if (std::fabs(target_x - player_x_) < 0.008f && std::fabs(player_vx_) < 0.08f) {
        player_x_ = target_x;
        player_vx_ = 0.0f;
    }

    // Smooth dynamic athletic body banking into lateral turns
    float desired_lean = clampf(player_vx_ * 0.036f, -0.38f, 0.38f);
    lane_lean_ = lerpf(lane_lean_, desired_lean, clampf(dt * 15.0f, 0.0f, 1.0f));

    // Spring-damped knee/hip landing compression for realistic weight absorption!
    float crouch_acc = -210.0f * landing_crouch_ - 26.0f * landing_crouch_vel_;
    landing_crouch_vel_ += crouch_acc * dt;
    landing_crouch_ = clampf(landing_crouch_ + landing_crouch_vel_ * dt, 0.0f, 0.36f);

    if (jump_buffer_ > 0.0f) jump_buffer_ = std::max(0.0f, jump_buffer_ - dt);
    if (roll_buffer_ > 0.0f) roll_buffer_ = std::max(0.0f, roll_buffer_ - dt);
    if (roll_timer_ > 0.0f) roll_timer_ = std::max(0.0f, roll_timer_ - dt);

    // Determine dynamic ground elevation (street y=0, ramp slope, or runnable vehicle roof!)
    ground_y_ = 0.0f;
    on_roof_ = false;
    for (const auto& o : obstacles_) {
        if (std::fabs(player_x_ - o.x) > o.half_w + 0.35f) continue;
        float s_min = o.s - o.half_len;
        float s_max = o.s + o.half_len;
        if (player_s_ < s_min - 0.35f || player_s_ > s_max + 0.35f) continue;

        if (o.is_ramp) {
            float t = clampf((player_s_ - s_min) / (2.0f * o.half_len), 0.0f, 1.0f);
            float ramp_y = t * o.height;
            if (player_y_ <= ramp_y + 0.75f && player_y_ >= ramp_y - 1.15f) {
                if (ramp_y > ground_y_) {
                    ground_y_ = ramp_y;
                    on_roof_ = true;
                }
            }
        } else if (o.runnable_top) {
            if (player_y_ >= o.height - 0.58f && player_vy_ <= 2.5f) {
                if (o.height > ground_y_) {
                    ground_y_ = o.height;
                    on_roof_ = true;
                }
            }
        }
    }

    if (jetpack_timer_ > 0.0f) {
        float flight_y = 6.2f + std::sin(sim_time_ * 4.5f) * 0.25f;
        player_y_ = lerpf(player_y_, flight_y, clampf(dt * 8.0f, 0.0f, 1.0f));
        player_vy_ = 0.0f;
        grounded_ = false;
        roll_timer_ = 0.0f;
    } else {
        if (jump_buffer_ > 0.0f && grounded_) {
            player_vy_ = (sneakers_timer_ > 0.0f) ? SNEAKERS_JUMP_VEL : JUMP_VEL;
            grounded_ = false;
            roll_timer_ = 0.0f;
            jump_buffer_ = 0.0f;
            landing_crouch_ = 0.0f;
            landing_crouch_vel_ = -0.8f;
            push_sfx(SFX_JUMP);
            spawn_burst({player_x_, player_y_ + 0.1f, player_s_}, 8,
                        (sneakers_timer_ > 0.0f) ? Vec3(0.25f, 1.0f, 0.45f) : Vec3(0.86f, 0.78f, 0.65f),
                        4.2f, 0.12f, 0.35f, 0.4f);
        }

        if (roll_buffer_ > 0.0f && grounded_) {
            roll_timer_ = ROLL_DURATION;
            roll_buffer_ = 0.0f;
            push_sfx(SFX_ROLL);
        }

        player_vy_ += GRAVITY * dt;
        player_y_ += player_vy_ * dt;

        if (player_y_ <= ground_y_) {
            if (!grounded_ && player_vy_ < -6.5f) {
                // Realistic athletic landing impact compression!
                landing_crouch_vel_ = clampf(-player_vy_ * 0.14f, 0.6f, 3.4f);
                Vec3 land_col = level_.is_rainy ? Vec3(0.75f, 0.88f, 1.0f) : Vec3(0.82f, 0.70f, 0.56f);
                spawn_burst({player_x_, ground_y_ + 0.08f, player_s_}, 7, land_col, 3.2f, 0.10f, 0.30f, 0.2f);
            }
            player_y_ = ground_y_;
            player_vy_ = 0.0f;
            grounded_ = true;
        } else {
            if (on_roof_ && player_y_ - ground_y_ < 0.28f && player_vy_ <= 0.0f) {
                player_y_ = ground_y_;
                player_vy_ = 0.0f;
                grounded_ = true;
            } else {
                grounded_ = false;
            }
        }
    }

    // Smooth jump pose blend so air-to-ground animation never pops abruptly
    float target_air = grounded_ ? 0.0f : 1.0f;
    jump_takeoff_blend_ = lerpf(jump_takeoff_blend_, target_air, clampf(dt * 14.0f, 0.0f, 1.0f));

    if (grounded_) {
        run_cycle_ += speed_s_ * dt * 0.74f;
        if (rng_.chance(level_.is_rainy ? 0.35f : 0.10f)) {
            Particle p;
            p.pos = {player_x_ + rng_.range(-0.2f, 0.2f), player_y_ + 0.08f, player_s_ - 0.2f};
            p.vel = {rng_.range(-1.2f, 1.2f), rng_.range(1.2f, 2.8f), -speed_s_ * 0.25f};
            p.size = rng_.range(0.07f, 0.12f);
            p.max_life = 0.28f;
            if (level_.is_rainy) {
                p.r = 0.72f; p.g = 0.86f; p.b = 1.0f; p.emissive = 0.4f;
            } else {
                p.r = 0.78f; p.g = 0.62f; p.b = 0.45f; p.emissive = 0.0f;
            }
            particles_.push_back(p);
        }
    } else {
        // Subtle mid-air leg drift
        run_cycle_ += dt * 3.2f;
    }
}

void GameEngine::update_npcs(float dt) {
    // Animate all Douala PNJ pedestrians & street vendors realistically!
    for (auto& n : npcs_) {
        if (std::fabs(n.speed_s) > 0.05f) {
            n.s += n.speed_s * dt;
            n.walk_cycle += std::fabs(n.speed_s) * dt * 3.6f;
        } else {
            // Street vendor idle breathing / conversational gesture cycle
            n.walk_cycle += dt * 2.4f;
        }

        // Reactive Douala bystanders: turn slightly & wave when Maximus zooms close!
        float ds = n.s - player_s_;
        float dx = std::fabs(n.x - player_x_);
        float target_wave = 0.0f;
        if (n.role == NPC_VENDOR) {
            target_wave = (ds > -3.0f && ds < 14.0f) ? 1.0f : 0.35f;
        } else if (ds > -2.5f && ds < 11.0f && dx < 9.5f) {
            target_wave = 0.85f;
        }
        n.wave_amount = lerpf(n.wave_amount, target_wave, clampf(dt * 6.0f, 0.0f, 1.0f));
    }
}

void GameEngine::update_obstacles_and_collisions(float dt) {
    float player_h = (roll_timer_ > 0.0f) ? PLAYER_ROLL_H : PLAYER_STAND_H;
    float p_min_x = player_x_ - PLAYER_HALF_W;
    float p_max_x = player_x_ + PLAYER_HALF_W;
    float p_min_y = player_y_;
    float p_max_y = player_y_ + player_h;
    float p_min_s = player_s_ - PLAYER_HALF_LEN;
    float p_max_s = player_s_ + PLAYER_HALF_LEN;

    for (auto& o : obstacles_) {
        if (o.speed_s < 0.0f && o.s - player_s_ < 115.0f) {
            o.s += o.speed_s * dt;
            if (!o.triggered_horn && status_ == STATUS_PLAYING &&
                o.s - player_s_ < 26.0f && o.s - player_s_ > 16.0f &&
                std::fabs(o.x - player_x_) < 3.6f) {
                o.triggered_horn = true;
                push_sfx(SFX_HORN);
            }
        }

        if (status_ == STATUS_DEMO || invuln_timer_ > 0.0f || jetpack_timer_ > 0.0f) {
            continue;
        }

        if (!o.triggered_near_miss && (o.kind == OBS_TAXI_MOVING || o.kind == OBS_BENSKIN_MOVING)) {
            if (std::fabs(o.s - player_s_) < 1.2f &&
                std::fabs(o.x - player_x_) > o.half_w + PLAYER_HALF_W &&
                std::fabs(o.x - player_x_) < 3.4f) {
                o.triggered_near_miss = true;
                near_misses_++;
                bonus_score_ += 50;
                combo_streak_ += 3;
                combo_decay_ = 3.5f;
                push_sfx(SFX_NEAR_MISS);
                trigger_popup("near miss / esquive +50!", 1.0f);
            }
        }

        if (o.is_ramp) continue;

        float o_min_x = o.x - o.half_w;
        float o_max_x = o.x + o.half_w;
        float o_min_s = o.s - o.half_len;
        float o_max_s = o.s + o.half_len;

        bool overlap_x = (p_min_x < o_max_x && p_max_x > o_min_x);
        bool overlap_s = (p_min_s < o_max_s && p_max_s > o_min_s);
        if (!overlap_x || !overlap_s) continue;

        if (o.is_pothole) {
            if (player_y_ < 0.35f) {
                trigger_crash("fell into a douala trench / chute dans une tranchee");
                return;
            }
            continue;
        }

        if (o.clearance_y > 0.0f) {
            if (p_max_y <= o.clearance_y + 0.08f) {
                if (!o.triggered_near_miss) {
                    o.triggered_near_miss = true;
                    bonus_score_ += 25;
                }
                continue;
            }
            if (p_min_y >= o.height - 0.15f) continue;
            trigger_crash("clipped overhead sign / choc panneau travaux");
            return;
        }

        if (o.runnable_top && player_y_ >= o.height - 0.56f) {
            continue;
        }

        if (p_min_y >= o.height - 0.18f) {
            continue;
        }

        float depth_into_front = player_s_ - o_min_s;
        bool side_graze = (depth_into_front > 0.75f && std::fabs(player_vx_) > 3.2f && !o.hit_side);
        if (side_graze) {
            o.hit_side = true;
            target_lane_ = prev_lane_;
            trigger_stumble("side graze / frolement");
            return;
        }

        std::string reason = "crashed in douala / collision a douala";
        switch (o.kind) {
            case OBS_TAXI_PARKED:
            case OBS_TAXI_MOVING: reason = "hit a yellow taxi / choc taxi jaune"; break;
            case OBS_BENSKIN_MOVING: reason = "hit a benskin moto / choc moto benskin"; break;
            case OBS_COASTER_BUS: reason = "hit a coaster bus / choc bus coaster"; break;
            case OBS_CARGO_TRUCK: reason = "hit a port cargo truck / choc camion du port"; break;
            case OBS_BARRIER_LOW: reason = "tripped on barricade / choc barriere travaux"; break;
            case OBS_BARRIER_FULL: reason = "hit concrete block / choc bloc beton"; break;
            case OBS_PUSHCART: reason = "hit plantain pushcart / choc pousse-pousse"; break;
            default: break;
        }
        trigger_crash(reason);
        return;
    }
}

void GameEngine::trigger_stumble(const std::string& /*reason*/) {
    if (hoverboard_timer_ > 0.0f) {
        cam_shake_ = 0.25f;
        push_sfx(SFX_STUMBLE);
        return;
    }
    stumbles_++;
    cam_shake_ = 0.55f;
    push_sfx(SFX_STUMBLE);

    if (chaser_proximity_ > 0.58f) {
        trigger_crash("caught by traffic patrol / attrape par la patrouille");
        return;
    }
    chaser_proximity_ = 0.92f;
    stumble_recovery_timer_ = 7.0f;
    trigger_popup("watch out! guard closing in / attention garde!", 1.5f);
    spawn_burst({player_x_, player_y_ + 0.9f, player_s_}, 12, {1.0f, 0.55f, 0.15f}, 5.0f, 0.12f, 0.4f);
}

void GameEngine::trigger_crash(const std::string& reason) {
    if (hoverboard_timer_ > 0.0f) {
        hoverboard_timer_ = 0.0f;
        invuln_timer_ = 1.8f;
        cam_shake_ = 0.85f;
        push_sfx(SFX_HOVERBOARD_BREAK);
        trigger_popup("hoverboard saved you / sauve par bouclier!", 1.5f);
        spawn_burst({player_x_, player_y_ + 0.8f, player_s_ + 1.0f}, 36, {0.20f, 1.0f, 0.65f}, 12.0f, 0.20f, 0.75f);

        float clear_s = player_s_;
        obstacles_.erase(std::remove_if(obstacles_.begin(), obstacles_.end(),
                                        [clear_s](const Obstacle& o) {
                                            return !o.is_ramp && o.s >= clear_s - 4.0f && o.s <= clear_s + 24.0f;
                                        }),
                         obstacles_.end());
        return;
    }

    status_ = STATUS_GAME_OVER;
    crash_reason_ = reason;
    cam_shake_ = 1.15f;
    chaser_proximity_ = 1.0f;
    push_sfx(SFX_CRASH);
    spawn_burst({player_x_, player_y_ + 1.0f, player_s_ + 0.5f}, 40, {1.0f, 0.30f, 0.12f}, 9.5f, 0.18f, 0.95f);
    spawn_burst({player_x_, player_y_ + 1.1f, player_s_ + 0.5f}, 24, {1.0f, 0.85f, 0.15f}, 8.0f, 0.14f, 0.80f);
}

void GameEngine::spawn_jetpack_coins() {
    int lane = target_lane_;
    for (int i = 0; i < 24; ++i) {
        if (i % 7 == 0) lane = rng_.range_int(0, 2);
        Collectible c;
        c.id = next_entity_id_++;
        c.is_powerup = false;
        c.x = LANE_X[lane];
        c.y = 6.2f;
        c.s = player_s_ + 14.0f + i * 3.0f;
        c.spin = i * 0.3f;
        collectibles_.push_back(c);
    }
}

void GameEngine::update_collectibles(float dt) {
    float pickup_r = (magnet_timer_ > 0.0f) ? 1.5f : 1.15f;

    for (auto& c : collectibles_) {
        if (c.collected) continue;
        c.spin += dt * 3.4f;

        float dx = player_x_ - c.x;
        float dy = (player_y_ + 0.95f) - c.y;
        float ds = player_s_ - c.s;
        float dist_sq = dx * dx + dy * dy + ds * ds;

        if (!c.is_powerup && magnet_timer_ > 0.0f && dist_sq < 14.0f * 14.0f && c.s > player_s_ - 2.0f) {
            c.magnetized = true;
        }
        if (c.magnetized) {
            float pull = clampf(dt * 16.0f, 0.0f, 1.0f);
            c.x = lerpf(c.x, player_x_, pull);
            c.y = lerpf(c.y, player_y_ + 0.95f, pull);
            c.s = lerpf(c.s, player_s_, pull * 1.15f);
            dx = player_x_ - c.x;
            dy = (player_y_ + 0.95f) - c.y;
            ds = player_s_ - c.s;
            dist_sq = dx * dx + dy * dy + ds * ds;
        }

        if (dist_sq < pickup_r * pickup_r) {
            c.collected = true;
            if (status_ == STATUS_DEMO) continue;

            if (!c.is_powerup) {
                coins_++;
                combo_streak_++;
                combo_decay_ = 2.6f;
                int mult = (mult_timer_ > 0.0f) ? 2 : 1;
                bonus_score_ += 10 * mult;
                push_sfx(SFX_COIN);
                spawn_burst({c.x, c.y, c.s}, 5, {1.0f, 0.88f, 0.22f}, 4.5f, 0.11f, 0.34f, 1.2f);
            } else {
                push_sfx(SFX_POWERUP);
                bonus_score_ += 100;
                switch (c.powerup) {
                    case PU_MAGNET:
                        magnet_timer_ = upgrades_.magnet_duration;
                        trigger_popup("fcfa coin magnet / aimant!", 1.5f);
                        spawn_burst({c.x, c.y, c.s}, 18, {1.0f, 0.22f, 0.22f}, 7.0f, 0.16f, 0.55f);
                        break;
                    case PU_SNEAKERS:
                        sneakers_timer_ = upgrades_.sneakers_duration;
                        trigger_popup("turbo sneakers / super saut!", 1.5f);
                        spawn_burst({c.x, c.y, c.s}, 18, {0.22f, 1.0f, 0.42f}, 7.0f, 0.16f, 0.55f);
                        break;
                    case PU_JETPACK:
                        jetpack_timer_ = upgrades_.jetpack_duration;
                        trigger_popup("douala sky jetpack / turbo vol!", 1.6f);
                        spawn_jetpack_coins();
                        spawn_burst({c.x, c.y, c.s}, 24, {1.0f, 0.65f, 0.14f}, 8.5f, 0.18f, 0.65f);
                        break;
                    case PU_MULTIPLIER:
                        mult_timer_ = upgrades_.multiplier_duration;
                        trigger_popup("2x multiplier / score double!", 1.5f);
                        spawn_burst({c.x, c.y, c.s}, 18, {0.25f, 0.90f, 1.0f}, 7.0f, 0.16f, 0.55f);
                        break;
                    case PU_HOVERBOARD:
                        hoverboards_left_++;
                        trigger_popup("+1 hoverboard shield / bouclier!", 1.5f);
                        spawn_burst({c.x, c.y, c.s}, 18, {0.20f, 1.0f, 0.65f}, 7.0f, 0.16f, 0.55f);
                        break;
                    default: break;
                }
            }
        }
    }
}

void GameEngine::update_particles(float dt) {
    bool rain_now = level_.is_rainy || (level_.is_endless && world_gen_.active_zone_at(player_s_) == 5);
    if (rain_now && particles_.size() < 340) {
        for (int i = 0; i < 8; ++i) {
            Particle r;
            r.pos = {player_x_ + rng_.range(-14.0f, 14.0f),
                     player_y_ + rng_.range(7.0f, 15.0f),
                     player_s_ + rng_.range(2.5f, 44.0f)};
            r.vel = {-1.8f, -26.0f, -6.0f};
            r.size = 1.0f;
            r.life = 0.0f;
            r.max_life = 0.58f;
            r.r = 0.62f; r.g = 0.78f; r.b = 0.96f;
            r.emissive = 0.45f;
            r.gravity = false;
            r.is_rain = true;
            particles_.push_back(r);
        }
    }

    for (auto& p : particles_) {
        p.life += dt;
        if (p.gravity) p.vel.y += -20.0f * dt;
        p.pos += p.vel * dt;
    }

    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                    [](const Particle& p) {
                                        return p.life >= p.max_life || p.pos.y < -1.0f;
                                    }),
                     particles_.end());
}

void GameEngine::update_camera(float dt) {
    float target_cam_x = player_x_ * 0.64f;
    float foot_bob = (grounded_ && status_ == STATUS_PLAYING)
                         ? std::sin(run_cycle_ * 2.0f) * 0.045f
                         : 0.0f;
    float target_cam_y = 4.55f + player_y_ * 0.68f - landing_crouch_ * 0.35f + foot_bob +
                         (jetpack_timer_ > 0.0f ? 1.4f : 0.0f);

    if (status_ == STATUS_DEMO) {
        // Subtle GTA-IV-like smooth cinematic lateral parallax sweep in demo mode
        target_cam_x += std::sin(sim_time_ * 0.52f) * 1.45f;
        target_cam_y += 0.40f + std::cos(sim_time_ * 0.38f) * 0.28f;
    }

    cam_x_ = lerpf(cam_x_, target_cam_x, clampf(dt * 9.0f, 0.0f, 1.0f));
    cam_y_ = lerpf(cam_y_, target_cam_y, clampf(dt * 8.0f, 0.0f, 1.0f));

    // Smooth camera banking into lane switches for realistic visceral flow!
    float target_roll = clampf(-player_vx_ * 0.0048f, -0.055f, 0.055f);
    cam_roll_ = lerpf(cam_roll_, target_roll, clampf(dt * 10.0f, 0.0f, 1.0f));

    float sx = 0.0f, sy = 0.0f;
    if (cam_shake_ > 0.001f) {
        sx = std::sin(sim_time_ * 55.0f) * cam_shake_ * 0.45f;
        sy = std::cos(sim_time_ * 63.0f) * cam_shake_ * 0.35f;
    }

    float cam_back = (jetpack_timer_ > 0.0f) ? 10.2f : 8.25f;
    Vec3 eye = to_gl_pos(cam_x_ + sx, cam_y_ + sy, player_s_ - cam_back);
    Vec3 look = to_gl_pos(player_x_ * 0.44f, player_y_ + 1.72f, player_s_ + 16.5f);
    Vec3 up = {std::sin(cam_roll_), std::cos(cam_roll_), 0.0f};

    float speed_t = clampf((speed_s_ - 18.0f) / 22.0f, 0.0f, 1.0f);
    float fov = (58.0f + speed_t * 11.0f + (jetpack_timer_ > 0.0f ? 6.0f : 0.0f)) * DEG2RAD;

    Mat4 view = Mat4::look_at(eye, look, up);
    Mat4 proj = Mat4::perspective(fov, aspect_, 0.35f, 190.0f);

    std::copy(std::begin(view.m), std::end(view.m), snapshot_.view_mat);
    std::copy(std::begin(proj.m), std::end(proj.m), snapshot_.proj_mat);
    snapshot_.cam_pos[0] = eye.x;
    snapshot_.cam_pos[1] = eye.y;
    snapshot_.cam_pos[2] = eye.z;

    Vec3 focus = to_gl_pos(0.0f, 0.0f, player_s_ + 22.0f);
    snapshot_.sun_focus[0] = focus.x;
    snapshot_.sun_focus[1] = focus.y;
    snapshot_.sun_focus[2] = focus.z;
}

void GameEngine::emit_instance(MeshId id, const Mat4& model, float r, float g, float b, float emissive) {
    InstanceData d;
    std::copy(std::begin(model.m), std::end(model.m), d.model);
    d.tr = r; d.tg = g; d.tb = b; d.emissive = emissive;
    instances_[id].push_back(d);
}

void GameEngine::emit_character(float x, float y, float s, float yaw_face, float cycle, float stride_amp,
                                float lean, float crouch, float roll_phase, bool rolling, bool jumping,
                                bool on_hoverboard, bool jetpacking, float wave_amt, float height_scale,
                                const Vec3& jersey_col, const Vec3& shorts_col, const Vec3& skin_col,
                                const Vec3& shoe_col, const Vec3& cap_col, bool is_chaser, bool is_npc) {
    // Realistic biomechanical gait: double-frequency vertical stride bob + hip/shoulder counter-twist!
    float s_sin = std::sin(cycle);
    float s_cos = std::cos(cycle);
    float bounce = (rolling || jumping || jetpacking || on_hoverboard || stride_amp < 0.05f)
                       ? 0.0f
                       : std::fabs(s_sin) * 0.095f * stride_amp;

    float torso_yaw = (rolling || on_hoverboard || jetpacking) ? 0.0f : (-s_sin * 0.22f * stride_amp);
    float hip_roll = (rolling || on_hoverboard || jetpacking) ? 0.0f : (s_sin * 0.075f * stride_amp);

    // Smooth 360-degree athletic parkour front-roll somersault when rolling!
    float pitch_forward = jetpacking ? -0.62f
                         : (rolling ? (-roll_phase * TWO_PI)
                                    : (-0.16f * stride_amp - crouch * 0.55f));
    float base_y = y + (on_hoverboard ? 0.22f : 0.0f) - crouch * 0.42f;
    if (rolling) {
        base_y = y - 0.34f + std::sin(roll_phase * PI) * 0.14f;
    }

    // Grounded contact shadow disc
    if (!is_chaser) {
        float ref_g = is_npc ? y : ground_y_;
        float sh_scale = clampf((1.0f - (y - ref_g) * 0.14f) * height_scale, 0.32f, 1.1f);
        emit_instance(MESH_SHADOW_BLOB,
                      Mat4::trs(to_gl_pos(x, ref_g + 0.025f, s), 0.0f, 0.0f, 0.0f,
                                {sh_scale, 1.0f, sh_scale}),
                      1.0f, 1.0f, 1.0f, 0.0f);
    }

    if (on_hoverboard) {
        float hover_bob = std::sin(sim_time_ * 10.0f) * 0.05f;
        emit_instance(MESH_HOVERBOARD,
                      Mat4::trs(to_gl_pos(x, y + 0.08f + hover_bob, s),
                                yaw_face, -0.05f, -lean * 1.4f, {1.0f, 1.0f, 1.0f}),
                      1.0f, 1.0f, 1.0f, 0.65f);
    }

    if (!is_chaser && !is_npc && invuln_timer_ > 0.0f && static_cast<int>(sim_time_ * 18.0f) % 2 == 0) {
        return;
    }

    // Root torso matrix with realistic counter-twist, banking lean, crouch & height scale
    Vec3 torso_p = to_gl_pos(x, base_y + (1.03f * height_scale) + bounce, s);
    Mat4 root_m = Mat4::trs(torso_p, yaw_face + torso_yaw, pitch_forward, -lean + hip_roll,
                            {height_scale, height_scale, height_scale});
    emit_instance(MESH_CHAR_TORSO, root_m, jersey_col.x, jersey_col.y, jersey_col.z, 0.0f);

    if (jetpacking) {
        Mat4 jp_m = root_m * Mat4::translate({0.0f, 0.02f, 0.28f});
        emit_instance(MESH_JETPACK, jp_m, 1.0f, 1.0f, 1.0f, 0.85f);
    }

    // Head with natural stabilization (counter-rotates slightly against torso twist so eyes stay locked forward!)
    float head_yaw = -torso_yaw * 0.65f + (is_npc ? std::sin(cycle * 0.5f) * 0.18f : 0.0f);
    float head_pitch = rolling ? 0.0f : (-pitch_forward * 0.55f);
    Mat4 head_m = root_m * Mat4::translate({0.0f, 0.47f, 0.0f}) *
                  Mat4::rotate_y(head_yaw) * Mat4::rotate_x(head_pitch);
    emit_instance(MESH_CHAR_HEAD, head_m,
                  lerpf(skin_col.x, cap_col.x, 0.12f),
                  lerpf(skin_col.y, cap_col.y, 0.12f),
                  lerpf(skin_col.z, cap_col.z, 0.12f), 0.0f);

    // === 2-BONE ARTICULATED ARMS (Upper Arm + Forward-Bent Elbow Forearm!) ===
    float l_arm_pitch = 0.0f, r_arm_pitch = 0.0f;
    float l_elbow = 0.72f, r_elbow = 0.72f;
    float r_arm_roll = 0.0f;

    if (rolling) {
        l_arm_pitch = 1.15f; r_arm_pitch = 1.15f;
        l_elbow = 1.35f; r_elbow = 1.35f;
    } else if (jetpacking) {
        l_arm_pitch = -0.35f; r_arm_pitch = -0.35f;
        l_elbow = 0.45f; r_elbow = 0.45f;
    } else if (on_hoverboard) {
        l_arm_pitch = 0.25f; r_arm_pitch = -0.25f;
        l_elbow = 0.55f; r_elbow = 0.55f;
    } else {
        // Athletic sprinting/walking arm pump in phase opposition to legs
        float air_k = is_npc ? 0.0f : jump_takeoff_blend_;
        float run_l = -s_sin * 0.92f * stride_amp;
        float run_r =  s_sin * 0.92f * stride_amp;
        l_arm_pitch = lerpf(run_l, 0.95f, air_k);
        r_arm_pitch = lerpf(run_r, -0.55f, air_k);
        // Natural dynamic elbow flexion (tighter ~85 deg on forward drive, looser on backswing!)
        l_elbow = lerpf(0.35f + std::max(0.0f, -s_sin) * 0.75f * stride_amp + 0.35f * stride_amp, 0.95f, air_k);
        r_elbow = lerpf(0.35f + std::max(0.0f,  s_sin) * 0.75f * stride_amp + 0.35f * stride_amp, 0.80f, air_k);
    }

    // PNJ friendly wave / street vendor gesturing!
    if (is_npc && wave_amt > 0.05f) {
        float wave_osc = std::sin(sim_time_ * 9.5f + cycle);
        r_arm_pitch = lerpf(r_arm_pitch, 2.35f, wave_amt);
        r_arm_roll = lerpf(0.0f, 0.38f + wave_osc * 0.32f, wave_amt);
        r_elbow = lerpf(r_elbow, 0.55f + wave_osc * 0.25f, wave_amt);
    }

    // Left Arm (Upper + Forearm)
    Mat4 l_uarm = root_m * Mat4::translate({-0.31f, 0.20f, 0.0f}) *
                  Mat4::rotate_x(l_arm_pitch) * Mat4::rotate_z(-0.08f);
    emit_instance(MESH_CHAR_LIMB, l_uarm * Mat4::scale({0.72f, 0.84f, 0.72f}),
                  jersey_col.x, jersey_col.y, jersey_col.z, 0.0f);
    Mat4 l_farm = l_uarm * Mat4::translate({0.0f, -0.29f, 0.0f}) * Mat4::rotate_x(l_elbow);
    emit_instance(MESH_CHAR_FORELIMB, l_farm * Mat4::scale({0.68f, 0.82f, 0.68f}),
                  skin_col.x, skin_col.y, skin_col.z, 0.0f);

    // Right Arm (Upper + Forearm)
    Mat4 r_uarm = root_m * Mat4::translate({0.31f, 0.20f, 0.0f}) *
                  Mat4::rotate_x(r_arm_pitch) * Mat4::rotate_z(0.08f + r_arm_roll);
    emit_instance(MESH_CHAR_LIMB, r_uarm * Mat4::scale({0.72f, 0.84f, 0.72f}),
                  jersey_col.x, jersey_col.y, jersey_col.z, 0.0f);
    Mat4 r_farm = r_uarm * Mat4::translate({0.0f, -0.29f, 0.0f}) * Mat4::rotate_x(r_elbow);
    emit_instance(MESH_CHAR_FORELIMB, r_farm * Mat4::scale({0.68f, 0.82f, 0.68f}),
                  skin_col.x, skin_col.y, skin_col.z, 0.0f);

    // === 2-BONE ARTICULATED LEGS (Thigh + Anatomically Backward-Bending Knee + Shin + Sneaker!) ===
    float l_thigh = 0.0f, r_thigh = 0.0f;
    float l_knee = -0.15f, r_knee = -0.15f;

    if (rolling) {
        l_thigh = 1.45f; r_thigh = 1.45f;
        l_knee = -1.65f; r_knee = -1.65f;
    } else if (on_hoverboard) {
        l_thigh = 0.24f; r_thigh = -0.24f;
        l_knee = -0.32f; r_knee = -0.28f;
    } else if (jetpacking) {
        l_thigh = -0.18f; r_thigh = -0.32f;
        l_knee = -0.45f; r_knee = -0.62f;
    } else {
        float air_k = is_npc ? 0.0f : jump_takeoff_blend_;
        float crouch_k = is_npc ? 0.0f : crouch;

        // Smooth natural thigh drive & anatomically accurate trailing-phase knee fold!
        float run_lt =  s_sin * 0.98f * stride_amp + crouch_k * 0.95f;
        float run_rt = -s_sin * 0.98f * stride_amp + crouch_k * 0.95f;

        // Knee folds backward (negative X rot!) highest as the leg swings through behind/under the hip
        float run_lk = -(0.14f + std::max(0.0f, -s_cos) * 1.18f * stride_amp + crouch_k * 1.85f);
        float run_rk = -(0.14f + std::max(0.0f,  s_cos) * 1.18f * stride_amp + crouch_k * 1.85f);

        // Athletic mid-air hurdle jump pose (front knee driven high, trailing knee folded!)
        l_thigh = lerpf(run_lt,  0.92f, air_k);
        r_thigh = lerpf(run_rt, -0.42f, air_k);
        l_knee  = lerpf(run_lk, -1.05f, air_k);
        r_knee  = lerpf(run_rk, -0.95f, air_k);
    }

    float shoe_emissive = (!is_chaser && !is_npc && sneakers_timer_ > 0.0f) ? 1.0f : 0.0f;
    Vec3 leg_col = (!is_chaser && !is_npc && sneakers_timer_ > 0.0f) ? Vec3(0.25f, 1.0f, 0.42f) : shorts_col;

    // Left Leg (Thigh -> Knee -> Shin -> Sneaker)
    Mat4 l_hip = root_m * Mat4::translate({-0.145f, -0.28f, 0.0f}) * Mat4::rotate_x(l_thigh);
    emit_instance(MESH_CHAR_LIMB, l_hip * Mat4::scale({0.95f, 1.05f, 0.95f}),
                  leg_col.x, leg_col.y, leg_col.z, shoe_emissive * 0.25f);
    Mat4 l_shin = l_hip * Mat4::translate({0.0f, -0.37f, 0.0f}) * Mat4::rotate_x(l_knee);
    emit_instance(MESH_CHAR_FORELIMB, l_shin * Mat4::scale({0.88f, 1.05f, 0.88f}),
                  is_npc ? leg_col.x : skin_col.x,
                  is_npc ? leg_col.y : skin_col.y,
                  is_npc ? leg_col.z : skin_col.z, shoe_emissive * 0.25f);
    Mat4 l_foot = l_shin * Mat4::translate({0.0f, -0.36f, 0.0f}) * Mat4::rotate_x(-l_knee * 0.25f);
    emit_instance(MESH_CHAR_SHOE, l_foot, shoe_col.x, shoe_col.y, shoe_col.z, shoe_emissive);

    // Right Leg (Thigh -> Knee -> Shin -> Sneaker)
    Mat4 r_hip = root_m * Mat4::translate({0.145f, -0.28f, 0.0f}) * Mat4::rotate_x(r_thigh);
    emit_instance(MESH_CHAR_LIMB, r_hip * Mat4::scale({0.95f, 1.05f, 0.95f}),
                  leg_col.x, leg_col.y, leg_col.z, shoe_emissive * 0.25f);
    Mat4 r_shin = r_hip * Mat4::translate({0.0f, -0.37f, 0.0f}) * Mat4::rotate_x(r_knee);
    emit_instance(MESH_CHAR_FORELIMB, r_shin * Mat4::scale({0.88f, 1.05f, 0.88f}),
                  is_npc ? leg_col.x : skin_col.x,
                  is_npc ? leg_col.y : skin_col.y,
                  is_npc ? leg_col.z : skin_col.z, shoe_emissive * 0.25f);
    Mat4 r_foot = r_shin * Mat4::translate({0.0f, -0.36f, 0.0f}) * Mat4::rotate_x(-r_knee * 0.25f);
    emit_instance(MESH_CHAR_SHOE, r_foot, shoe_col.x, shoe_col.y, shoe_col.z, shoe_emissive);
}

void GameEngine::build_render_instances() {
    for (auto& vec : instances_) vec.clear();

    bool has_any_bridge = false;
    bool night_now = level_.is_night || (level_.is_endless && world_gen_.active_zone_at(player_s_) == 5);
    bool sunset_now = level_.is_bridge || (level_.is_endless && world_gen_.active_zone_at(player_s_) == 4);

    // 1. Road & Pont du Wouri bridge segments
    for (size_t i = 0; i < road_segments_.size(); ++i) {
        float rs = road_segments_[i];
        bool bridge = road_is_bridge_[i];
        if (bridge) has_any_bridge = true;
        emit_instance(bridge ? MESH_BRIDGE_WOURI : MESH_ROAD_BOULEVARD,
                      Mat4::translate(to_gl_pos(0.0f, 0.0f, rs + ROAD_SEGMENT_LEN * 0.5f)),
                      1.0f, 1.0f, 1.0f, night_now ? 0.35f : 0.0f);
    }

    if (has_any_bridge || level_.is_bridge) {
        emit_instance(MESH_WATER_PLANE,
                      Mat4::translate(to_gl_pos(0.0f, -6.2f, player_s_ + 35.0f)),
                      1.0f, 1.0f, 1.0f, 0.0f);
    }

    // 2. Douala roadside architecture, palms, market umbrellas, streetlamps, monuments
    for (const auto& p : scenery_) {
        if (p.s < player_s_ - 22.0f || p.s > player_s_ + 150.0f) continue;
        emit_instance(p.mesh,
                      Mat4::trs(to_gl_pos(p.x, p.y, p.s), p.yaw, 0.0f, 0.0f, {p.sx, p.sy, p.sz}),
                      p.tr, p.tg, p.tb, p.emissive);
    }

    // 3. Real-world Douala obstacles + realistic suspension heave & headlight glow cones!
    for (const auto& o : obstacles_) {
        if (o.s < player_s_ - 16.0f || o.s > player_s_ + 145.0f) continue;
        float bob = 0.0f;
        float roll_w = 0.0f;
        if (o.kind == OBS_BENSKIN_MOVING) {
            bob = std::sin(sim_time_ * 14.0f + o.anim_phase) * 0.035f;
            roll_w = std::sin(sim_time_ * 5.0f + o.anim_phase) * 0.065f;
        } else if (o.kind == OBS_TAXI_MOVING) {
            bob = std::sin(sim_time_ * 11.0f + o.anim_phase) * 0.025f;
        }
        float sy = (o.is_ramp) ? (o.height / 2.85f) : 1.0f;
        emit_instance(o.mesh,
                      Mat4::trs(to_gl_pos(o.x, o.y + bob, o.s), 0.0f, 0.0f, roll_w, {1.0f, sy, 1.0f}),
                      o.tint_r, o.tint_g, o.tint_b, night_now ? 0.55f : 0.15f);

        // Glowing asphalt headlight beams projecting ahead of oncoming taxis & benskins!
        if ((night_now || sunset_now) && (o.kind == OBS_TAXI_MOVING || o.kind == OBS_BENSKIN_MOVING)) {
            float beam_w = (o.kind == OBS_BENSKIN_MOVING) ? 0.55f : 1.0f;
            float beam_em = night_now ? 0.85f : 0.40f;
            emit_instance(MESH_HEADLIGHT_BEAM,
                          Mat4::trs(to_gl_pos(o.x, 0.02f, o.s - o.half_len), 0.0f, 0.0f, 0.0f,
                                    {beam_w, 1.0f, 1.0f}),
                          1.0f, 0.94f, 0.72f, beam_em);
        }
    }

    // 4. Animated Douala PNJ Pedestrians & Street Vendors!
    for (const auto& n : npcs_) {
        if (n.s < player_s_ - 14.0f || n.s > player_s_ + 115.0f) continue;
        float stride = (n.role == NPC_VENDOR) ? 0.0f : 0.54f;
        emit_character(n.x, n.y, n.s, n.yaw, n.walk_cycle, stride,
                       0.0f, 0.0f, 0.0f, false, false, false, false,
                       n.wave_amount, n.height_scale,
                       n.shirt_col, n.pants_col, n.skin_col, n.shoe_col, n.hat_col,
                       false, true);
    }

    // 5. Spinning FCFA Coins & Glowing Power-Ups
    for (const auto& c : collectibles_) {
        if (c.collected || c.s < player_s_ - 8.0f || c.s > player_s_ + 125.0f) continue;
        float hover = std::sin(sim_time_ * 4.5f + c.spin) * 0.12f;
        MeshId mid = MESH_COIN;
        float sc = 1.0f;
        if (c.is_powerup) {
            sc = 1.15f + std::sin(sim_time_ * 6.0f) * 0.08f;
            switch (c.powerup) {
                case PU_MAGNET: mid = MESH_PU_MAGNET; break;
                case PU_SNEAKERS: mid = MESH_PU_SNEAKERS; break;
                case PU_JETPACK: mid = MESH_PU_JETPACK; break;
                case PU_MULTIPLIER: mid = MESH_PU_MULTIPLIER; break;
                case PU_HOVERBOARD: mid = MESH_PU_HOVERBOARD; break;
                default: mid = MESH_COIN; break;
            }
        }
        emit_instance(mid,
                      Mat4::trs(to_gl_pos(c.x, c.y + hover, c.s), c.spin, 0.0f, 0.0f, {sc, sc, sc}),
                      1.0f, 1.0f, 1.0f, 0.85f);
    }

    // 6. Maximus (the Runner!) with realistic 2-bone sprinting, parkour roll & outfits
    Vec3 jersey(0.06f, 0.62f, 0.30f);
    Vec3 shorts(0.84f, 0.16f, 0.16f);
    Vec3 skin(0.36f, 0.22f, 0.14f);
    Vec3 shoes(0.98f, 0.80f, 0.10f);
    Vec3 cap(0.10f, 0.50f, 0.24f);

    if (upgrades_.outfit_id == 1) {
        jersey = {0.14f, 0.44f, 0.94f};
        shorts = {0.16f, 0.18f, 0.24f};
        shoes = {0.98f, 0.46f, 0.10f};
    } else if (upgrades_.outfit_id == 2) {
        jersey = {0.98f, 0.76f, 0.12f};
        shorts = {0.12f, 0.14f, 0.18f};
        shoes = {0.14f, 0.86f, 0.96f};
    } else if (upgrades_.outfit_id == 3) {
        jersey = {0.82f, 0.16f, 0.72f};
        shorts = {0.12f, 0.12f, 0.20f};
        shoes = {0.26f, 0.98f, 0.46f};
    }

    float roll_phase = (roll_timer_ > 0.0f) ? clampf(1.0f - (roll_timer_ / ROLL_DURATION), 0.0f, 1.0f) : 0.0f;
    float sprint_amp = clampf(0.88f + (speed_s_ - 18.0f) * 0.012f, 0.85f, 1.15f);
    emit_character(player_x_, player_y_, player_s_, 0.0f, run_cycle_, sprint_amp,
                   lane_lean_, landing_crouch_, roll_phase, roll_timer_ > 0.0f, !grounded_,
                   hoverboard_timer_ > 0.0f, jetpack_timer_ > 0.0f, 0.0f, 1.0f,
                   jersey, shorts, skin, shoes, cap, false, false);

    // 7. Douala Municipal Traffic Officer Chaser
    if (status_ != STATUS_DEMO && chaser_proximity_ > 0.06f) {
        emit_character(chaser_x_, 0.0f, player_s_ - chaser_s_offset_, 0.0f,
                       run_cycle_ * 1.08f + 1.2f, 1.0f, 0.0f, 0.0f, 0.0f,
                       false, false, false, false, 0.0f, 1.04f,
                       {0.12f, 0.22f, 0.46f}, {0.10f, 0.14f, 0.26f}, skin, {0.12f, 0.12f, 0.14f},
                       {0.96f, 0.80f, 0.15f}, true, false);
    }

    // 8. Particles & Tropical Rain Streaks
    for (const auto& p : particles_) {
        float t = clampf(p.life / p.max_life, 0.0f, 1.0f);
        if (p.is_rain) {
            emit_instance(MESH_RAIN_STREAK,
                          Mat4::trs(to_gl_pos(p.pos.x, p.pos.y, p.pos.z), 0.0f, 0.0f, -0.08f, {1.0f, 1.0f, 1.0f}),
                          p.r, p.g, p.b, p.emissive);
        } else {
            float sc = p.size * (1.0f - t * 0.75f);
            emit_instance(MESH_PARTICLE_CUBE,
                          Mat4::trs(to_gl_pos(p.pos.x, p.pos.y, p.pos.z), p.life * 6.0f, p.life * 4.0f, 0.0f,
                                    {sc, sc, sc}),
                          p.r, p.g, p.b, p.emissive);
        }
    }
}

void GameEngine::update_snapshot() {
    snapshot_.status = static_cast<int>(status_);
    snapshot_.level_id = level_.level_id;
    snapshot_.endless_zone = world_gen_.active_zone_at(player_s_);
    snapshot_.distance = player_s_;
    snapshot_.target_distance = level_.target_distance;
    snapshot_.progress = level_.is_endless
                             ? std::fmod(player_s_, 380.0f) / 380.0f
                             : clampf(player_s_ / std::max(1.0f, level_.target_distance), 0.0f, 1.0f);
    snapshot_.speed_mps = speed_s_;
    snapshot_.speed_kmh = speed_s_ * 3.6f;
    snapshot_.score = static_cast<int>(score_accum_) + bonus_score_;
    snapshot_.coins = coins_;
    snapshot_.multiplier = (mult_timer_ > 0.0f ? 2 : 1) * (1 + std::min(4, combo_streak_ / 10));
    snapshot_.combo_streak = combo_streak_;
    snapshot_.near_misses = near_misses_;
    snapshot_.stumbles = stumbles_;
    snapshot_.hoverboards_left = hoverboards_left_;

    snapshot_.magnet_timer = magnet_timer_;
    snapshot_.magnet_max = upgrades_.magnet_duration;
    snapshot_.sneakers_timer = sneakers_timer_;
    snapshot_.sneakers_max = upgrades_.sneakers_duration;
    snapshot_.jetpack_timer = jetpack_timer_;
    snapshot_.jetpack_max = upgrades_.jetpack_duration;
    snapshot_.mult_timer = mult_timer_;
    snapshot_.mult_max = upgrades_.multiplier_duration;
    snapshot_.hoverboard_timer = hoverboard_timer_;
    snapshot_.hoverboard_max = 15.0f;

    snapshot_.chaser_proximity = chaser_proximity_;
    snapshot_.camera_shake = cam_shake_;
    snapshot_.player_x = player_x_;
    snapshot_.player_y = player_y_;
    snapshot_.player_s = player_s_;
    snapshot_.on_roof = on_roof_;
    snapshot_.is_rolling = (roll_timer_ > 0.0f);

    snapshot_.popup_text = popup_text_;
    snapshot_.popup_timer = popup_timer_;
    snapshot_.crash_reason = crash_reason_;
}

void GameEngine::trigger_popup(const std::string& text, float duration) {
    popup_text_ = text;
    popup_timer_ = duration;
}

void GameEngine::spawn_burst(Vec3 pos, int count, Vec3 color, float speed, float size, float life,
                             float emissive) {
    for (int i = 0; i < count; ++i) {
        if (particles_.size() >= 480) break;
        float a = rng_.range(0.0f, TWO_PI);
        float elev = rng_.range(0.15f, 1.15f);
        float spd = rng_.range(speed * 0.45f, speed);
        Particle p;
        p.pos = pos;
        p.vel = {std::cos(a) * spd, std::sin(elev) * spd, std::sin(a) * spd * 0.6f};
        p.size = rng_.range(size * 0.65f, size * 1.25f);
        p.life = 0.0f;
        p.max_life = rng_.range(life * 0.7f, life * 1.2f);
        p.r = color.x; p.g = color.y; p.b = color.z;
        p.emissive = emissive;
        p.gravity = true;
        particles_.push_back(p);
    }
}

void GameEngine::push_sfx(SoundEvent ev) {
    if (status_ == STATUS_DEMO) return;
    if (sound_events_.size() < 24) {
        sound_events_.push_back(static_cast<int>(ev));
    }
}

std::vector<int> GameEngine::consume_sound_events() {
    std::vector<int> out;
    out.swap(sound_events_);
    return out;
}

int GameEngine::total_instance_count() const {
    int total = 0;
    for (const auto& v : instances_) total += static_cast<int>(v.size());
    return total;
}

}  // namespace maximus
