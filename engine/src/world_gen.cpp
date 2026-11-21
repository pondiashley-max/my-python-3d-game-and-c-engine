#include "world_gen.hpp"
#include <algorithm>
#include <cmath>

namespace maximus {

void WorldGenerator::reset(const LevelParams& params) {
    params_ = params;
    rng_.seed(params.seed ? params.seed : 23701ULL + static_cast<uint64_t>(params.level_id) * 997ULL);
    next_obstacle_s_ = 34.0f;
    next_scenery_s_ = -24.0f;
    next_road_s_ = -24.0f;
    next_monument_s_ = 72.0f;
    finish_spawned_ = false;
    lane_busy_until_ = {0.0f, 0.0f, 0.0f};
    pattern_index_ = 0;
}

int WorldGenerator::active_zone_at(float s) const {
    if (!params_.is_endless) {
        return clampf(static_cast<float>(params_.level_id), 1.0f, 5.0f);
    }
    // Endless Douala Tour cycles through all 5 districts every 380 meters!
    int idx = (static_cast<int>(std::max(0.0f, s) / 380.0f) % 5) + 1;
    return idx;
}

Obstacle WorldGenerator::make_obstacle(ObstacleKind kind, int lane, float s,
                                       float speed_scale, int& next_id) {
    Obstacle o;
    o.id = next_id++;
    o.kind = kind;
    o.lane = lane;
    o.x = LANE_X[lane];
    o.y = 0.0f;
    o.s = s;
    o.anim_phase = rng_.range(0.0f, TWO_PI);

    switch (kind) {
        case OBS_TAXI_PARKED:
            o.mesh = MESH_TAXI_YELLOW;
            o.half_w = 0.94f;
            o.height = 1.55f;
            o.half_len = 2.28f;
            o.speed_s = 0.0f;
            o.runnable_top = true;
            break;

        case OBS_TAXI_MOVING:
            o.mesh = MESH_TAXI_YELLOW;
            o.half_w = 0.94f;
            o.height = 1.55f;
            o.half_len = 2.28f;
            o.speed_s = -(7.5f + 5.5f * speed_scale);
            o.runnable_top = true;
            break;

        case OBS_BENSKIN_MOVING:
            o.mesh = MESH_BENSKIN;
            o.half_w = 0.56f;
            o.height = 1.48f;
            o.half_len = 1.15f;
            o.speed_s = -(10.0f + 7.0f * speed_scale);
            o.runnable_top = false;
            break;

        case OBS_COASTER_BUS:
            o.mesh = MESH_COASTER_BUS;
            o.half_w = 1.10f;
            o.height = 2.85f;
            o.half_len = 4.08f;
            o.speed_s = 0.0f;
            o.runnable_top = true;
            break;

        case OBS_CARGO_TRUCK:
            o.mesh = MESH_CARGO_TRUCK;
            o.half_w = 1.16f;
            o.height = 3.15f;
            o.half_len = 4.75f;
            o.speed_s = 0.0f;
            o.runnable_top = true;
            // Subtle color variation on container tint
            if (rng_.chance(0.5f)) {
                o.tint_r = 1.0f; o.tint_g = 0.95f; o.tint_b = 0.88f;
            } else {
                o.tint_r = 0.88f; o.tint_g = 0.96f; o.tint_b = 1.0f;
            }
            break;

        case OBS_RAMP:
            o.mesh = MESH_RAMP_WOOD;
            o.half_w = 1.06f;
            o.height = 2.85f;
            o.half_len = 2.30f;
            o.speed_s = 0.0f;
            o.is_ramp = true;
            o.runnable_top = true;
            break;

        case OBS_BARRIER_LOW:
            o.mesh = MESH_BARRIER_LOW;
            o.half_w = 1.06f;
            o.height = 1.04f;
            o.half_len = 0.34f;
            break;

        case OBS_BARRIER_HIGH:
            o.mesh = MESH_BARRIER_HIGH;
            o.half_w = 1.12f;
            o.height = 2.90f;
            o.clearance_y = 1.28f;  // Roll height is 0.82m -> clean slide under!
            o.half_len = 0.32f;
            break;

        case OBS_BARRIER_FULL:
            o.mesh = MESH_BARRIER_FULL;
            o.half_w = 1.10f;
            o.height = 1.66f;
            o.half_len = 0.56f;
            break;

        case OBS_POTHOLE:
            o.mesh = MESH_POTHOLE_TRENCH;
            o.half_w = 1.04f;
            o.height = 0.25f;
            o.half_len = 1.25f;
            o.is_pothole = true;
            break;

        case OBS_PUSHCART:
            o.mesh = MESH_PUSHCART_PLANTAIN;
            o.half_w = 0.88f;
            o.height = 1.38f;
            o.half_len = 1.40f;
            break;
    }
    return o;
}

void WorldGenerator::spawn_coin_line(int lane, float s_start, int count, float spacing, float y,
                                     std::vector<Collectible>& collectibles, int& next_id) {
    for (int i = 0; i < count; ++i) {
        Collectible c;
        c.id = next_id++;
        c.is_powerup = false;
        c.x = LANE_X[lane];
        c.y = y;
        c.s = s_start + i * spacing;
        c.spin = static_cast<float>(i) * 0.35f;
        collectibles.push_back(c);
    }
}

void WorldGenerator::spawn_coin_arc(int lane, float s_center, int count, float span,
                                    float base_y, float peak_h,
                                    std::vector<Collectible>& collectibles, int& next_id) {
    for (int i = 0; i < count; ++i) {
        float t = (count > 1) ? (static_cast<float>(i) / (count - 1)) : 0.5f;
        float s = s_center - span * 0.5f + t * span;
        float arc = std::sin(t * PI);
        Collectible c;
        c.id = next_id++;
        c.is_powerup = false;
        c.x = LANE_X[lane];
        c.y = base_y + arc * peak_h;
        c.s = s;
        c.spin = static_cast<float>(i) * 0.32f;
        collectibles.push_back(c);
    }
}

void WorldGenerator::spawn_coin_diagonal(int lane_from, int lane_to, float s_start,
                                         int count, float spacing, float y,
                                         std::vector<Collectible>& collectibles, int& next_id) {
    for (int i = 0; i < count; ++i) {
        float t = (count > 1) ? (static_cast<float>(i) / (count - 1)) : 0.5f;
        Collectible c;
        c.id = next_id++;
        c.is_powerup = false;
        c.x = lerpf(LANE_X[lane_from], LANE_X[lane_to], t);
        c.y = y;
        c.s = s_start + i * spacing;
        c.spin = static_cast<float>(i) * 0.38f;
        collectibles.push_back(c);
    }
}

void WorldGenerator::maybe_spawn_powerup(int lane, float s, float y,
                                         std::vector<Collectible>& collectibles, int& next_id) {
    if (!rng_.chance(params_.powerup_chance)) return;
    Collectible p;
    p.id = next_id++;
    p.is_powerup = true;
    int r = rng_.range_int(0, 99);
    if (r < 26) p.powerup = PU_MAGNET;
    else if (r < 50) p.powerup = PU_SNEAKERS;
    else if (r < 68) p.powerup = PU_MULTIPLIER;
    else if (r < 85) p.powerup = PU_HOVERBOARD;
    else p.powerup = PU_JETPACK;

    p.x = LANE_X[lane];
    p.y = y;
    p.s = s;
    p.spin = 0.0f;
    collectibles.push_back(p);
}

void WorldGenerator::spawn_scenery_row(float s, int zone, std::vector<SceneryProp>& scenery,
                                       std::vector<NpcEntity>& npcs, int& next_id) {
    bool is_bridge = (zone == 4) || params_.is_bridge;
    bool is_market = (zone == 2) || params_.is_market;
    bool is_deido = (zone == 3) || params_.has_monument;
    bool is_night = (zone == 5) || params_.is_night;

    // Vibrant authentic Douala PNJ clothing & skin palette
    static const Vec3 npc_shirts[8] = {
        {0.92f, 0.30f, 0.16f},  // Terracotta wax shirt
        {0.10f, 0.62f, 0.36f},  // Cameroon green tee
        {0.96f, 0.78f, 0.14f},  // Sunny Douala yellow
        {0.18f, 0.44f, 0.88f},  // Royal blue jersey
        {0.84f, 0.18f, 0.56f},  // Vibrant magenta tunic
        {0.94f, 0.94f, 0.92f},  // Crisp white polo
        {0.14f, 0.68f, 0.72f},  // Coastal turquoise
        {0.94f, 0.52f, 0.12f},  // Warm mango orange
    };
    static const Vec3 npc_pants[4] = {
        {0.16f, 0.24f, 0.42f},  // Denim blue
        {0.16f, 0.16f, 0.18f},  // Charcoal
        {0.44f, 0.34f, 0.22f},  // Khaki
        {0.24f, 0.18f, 0.26f},  // Deep plum
    };
    static const Vec3 npc_skins[3] = {
        {0.34f, 0.20f, 0.13f},
        {0.42f, 0.26f, 0.16f},
        {0.28f, 0.16f, 0.10f},
    };

    auto make_npc = [&](NpcRole role, float x, float y, float s_pos, float spd, float yaw) {
        NpcEntity n;
        n.id = next_id++;
        n.role = role;
        n.x = x;
        n.y = y;
        n.s = s_pos;
        n.speed_s = spd;
        n.yaw = yaw;
        n.walk_cycle = rng_.range(0.0f, TWO_PI);
        n.height_scale = rng_.range(0.88f, 1.08f);
        n.shirt_col = npc_shirts[rng_.range_int(0, 7)];
        n.pants_col = npc_pants[rng_.range_int(0, 3)];
        n.skin_col = npc_skins[rng_.range_int(0, 2)];
        n.shoe_col = rng_.chance(0.5f) ? Vec3(0.95f, 0.95f, 0.92f) : Vec3(0.18f, 0.18f, 0.20f);
        n.hat_col = npc_shirts[rng_.range_int(0, 7)];
        npcs.push_back(n);
    };

    if (is_bridge) {
        // Pont du Wouri: Port of Douala giant red/white gantry cranes overlooking the river!
        if (static_cast<int>(std::floor(s / 12.0f)) % 4 == 0) {
            float side = rng_.chance(0.5f) ? -1.0f : 1.0f;
            SceneryProp crane;
            crane.mesh = MESH_PORT_CRANE;
            crane.x = side * rng_.range(24.0f, 34.0f);
            crane.y = -5.8f;
            crane.s = s;
            crane.yaw = (side < 0.0f) ? 0.0f : PI;
            crane.sx = 1.0f; crane.sy = 1.0f; crane.sz = 1.0f;
            scenery.push_back(crane);
        }
        // PNJ bridge strollers walking along the Pont du Wouri pedestrian walkways!
        for (float side : {-1.0f, 1.0f}) {
            if (rng_.chance(0.32f)) {
                bool forward = rng_.chance(0.5f);
                float spd = forward ? rng_.range(1.3f, 2.2f) : -rng_.range(1.3f, 2.2f);
                make_npc(NPC_BRIDGE_STROLLER, side * 6.05f, 0.36f, s + rng_.range(-4.0f, 4.0f),
                         spd, forward ? 0.0f : PI);
            }
        }
        return;
    }

    // Douala warm architectural color palette (ochre, terracotta, coastal teal, cream, pastel yellow)
    static const Vec3 palette[6] = {
        {0.96f, 0.88f, 0.72f},  // Warm cream plaster
        {0.92f, 0.66f, 0.44f},  // Douala terracotta ochre
        {0.72f, 0.86f, 0.84f},  // Coastal mint-teal
        {0.94f, 0.80f, 0.48f},  // Sunny tropical yellow
        {0.84f, 0.78f, 0.76f},  // Urban concrete stone
        {0.88f, 0.58f, 0.52f},  // Coral stucco
    };

    for (float side : {-1.0f, 1.0f}) {
        // Streetlamps lining both sidewalks every 24m
        if (static_cast<int>(std::floor(s / 12.0f)) % 2 == 0) {
            SceneryProp lamp;
            lamp.mesh = MESH_STREETLAMP;
            lamp.x = side * 6.05f;
            lamp.y = 0.20f;
            lamp.s = s;
            lamp.yaw = (side < 0.0f) ? 0.0f : PI;
            lamp.emissive = is_night ? 1.0f : 0.1f;
            scenery.push_back(lamp);
        }

        bool spawned_billboard = false;
        // Occasional vibrant Douala Roadside Billboards — kept completely clear of palms!
        if (rng_.chance(0.14f) && !is_market) {
            SceneryProp bb;
            bb.mesh = MESH_BILLBOARD;
            bb.x = side * 8.4f;
            bb.y = 0.15f;
            bb.s = s;
            bb.yaw = side * -0.26f;  // Angled cleanly toward the runner!
            bb.emissive = is_night ? 0.55f : 0.08f;
            scenery.push_back(bb);
            spawned_billboard = true;
        }

        // Marché Mboppi: bustling colorful striped market umbrellas + PNJ vendors!
        if (is_market) {
            SceneryProp umb;
            umb.mesh = MESH_MARKET_UMBRELLA;
            umb.x = side * rng_.range(7.4f, 8.3f);
            umb.y = 0.20f;
            umb.s = s + rng_.range(-1.5f, 1.5f);
            umb.yaw = rng_.range(0.0f, TWO_PI);
            float sc = rng_.range(0.92f, 1.15f);
            umb.sx = sc; umb.sy = sc; umb.sz = sc;
            int ucol = rng_.range_int(0, 3);
            if (ucol == 0) { umb.tr = 1.0f; umb.tg = 0.95f; umb.tb = 0.90f; }
            else if (ucol == 1) { umb.tr = 0.82f; umb.tg = 1.0f; umb.tb = 0.85f; }
            else if (ucol == 2) { umb.tr = 0.85f; umb.tg = 0.92f; umb.tb = 1.0f; }
            else { umb.tr = 1.0f; umb.tg = 0.88f; umb.tb = 0.78f; }
            scenery.push_back(umb);

            // Animated Market Vendor PNJ standing right beside stall facing the street!
            if (rng_.chance(0.62f)) {
                make_npc(NPC_VENDOR, side * 6.35f, 0.22f, umb.s + rng_.range(-1.2f, 1.2f),
                         0.0f, (side < 0.0f) ? (-PI * 0.5f) : (PI * 0.5f));
            }
        } else if (!spawned_billboard && rng_.chance(0.55f)) {
            // Iconic Douala Palm Trees lining Akwa / Deido / Bonanjo avenues!
            SceneryProp palm;
            palm.mesh = MESH_PALM_TREE;
            palm.x = side * rng_.range(7.2f, 8.8f);
            palm.y = 0.16f;
            palm.s = s + rng_.range(-2.5f, 2.5f);
            palm.yaw = rng_.range(0.0f, TWO_PI);
            float sc = rng_.range(0.88f, 1.25f);
            palm.sx = sc; palm.sy = sc * rng_.range(0.92f, 1.15f); palm.sz = sc;
            scenery.push_back(palm);
        }

        // Lively Douala sidewalk PNJ pedestrians strolling along both sides of the avenue!
        float walker_chance = is_market ? 0.55f : (is_night ? 0.30f : 0.48f);
        if (rng_.chance(walker_chance)) {
            bool walk_fwd = rng_.chance(0.5f);
            float spd = walk_fwd ? rng_.range(1.4f, 2.4f) : -rng_.range(1.4f, 2.4f);
            float px = side * (is_market ? rng_.range(5.9f, 6.45f) : rng_.range(6.15f, 7.65f));
            make_npc(NPC_WALKER, px, 0.22f, s + rng_.range(-4.5f, 4.5f),
                     spd, walk_fwd ? 0.0f : PI);
        }

        // Leave an open plaza clearing around Rond-Point Deido Monument de la Nouvelle Liberté!
        bool monument_clearing = is_deido && std::fabs(s - next_monument_s_) < 16.0f && side > 0.0f;
        if (monument_clearing) continue;

        // Buildings behind the sidewalk (mix of modern towers & colorful Douala shopfronts)
        SceneryProp bld;
        bool tall = (zone == 5 || zone == 1) ? rng_.chance(0.58f) : rng_.chance(0.35f);
        bld.mesh = tall ? MESH_BUILDING_MODERN : MESH_BUILDING_SHOP;
        bld.x = side * (is_market ? rng_.range(13.8f, 16.0f) : rng_.range(12.8f, 15.2f));
        bld.y = 0.12f;
        bld.s = s;
        bld.yaw = (side < 0.0f) ? (PI * 0.5f) : (-PI * 0.5f);
        bld.sx = rng_.range(0.95f, 1.25f);
        bld.sy = rng_.range(0.85f, (zone == 5 ? 1.55f : 1.30f));
        bld.sz = rng_.range(0.95f, 1.22f);
        Vec3 c = palette[rng_.range_int(0, 5)];
        if (is_night) {
            c = c * 0.65f;
        }
        bld.tr = c.x; bld.tg = c.y; bld.tb = c.z;
        bld.emissive = is_night ? 0.45f : 0.0f;
        scenery.push_back(bld);

        // Port cranes visible in the background skyline of Bonanjo Night!
        if (zone == 5 && side > 0.0f && static_cast<int>(std::floor(s / 12.0f)) % 6 == 0) {
            SceneryProp crane;
            crane.mesh = MESH_PORT_CRANE;
            crane.x = 28.0f;
            crane.y = 0.0f;
            crane.s = s;
            crane.yaw = PI;
            scenery.push_back(crane);
        }
    }

    // Rond-Point Deido Landmark: Monument de la Nouvelle Liberté towering beside the boulevard!
    if (is_deido && s >= next_monument_s_) {
        SceneryProp mon;
        mon.mesh = MESH_MONUMENT_LIBERTE;
        mon.x = 12.2f;
        mon.y = 0.15f;
        mon.s = s;
        mon.yaw = -0.58f;
        mon.sx = 1.25f; mon.sy = 1.25f; mon.sz = 1.25f;
        scenery.push_back(mon);
        next_monument_s_ = s + 125.0f;
    }
}

void WorldGenerator::spawn_obstacle_pattern(float s, float current_speed, int zone,
                                            std::vector<Obstacle>& obstacles,
                                            std::vector<Collectible>& collectibles,
                                            int& next_entity_id) {
    float difficulty = clampf((current_speed - 16.0f) / 22.0f, 0.0f, 1.0f);
    float speed_scale = clampf(difficulty * 0.75f + params_.obstacle_density * 0.35f, 0.0f, 1.25f);

    // Guarantee at least 1 lane is clear or clearly navigable (open / jump / slide / ramp)!
    int safe_lane = rng_.range_int(0, 2);
    // Avoid picking a lane that is still physically busy from a long truck/bus convoy
    for (int tries = 0; tries < 3; ++tries) {
        if (lane_busy_until_[safe_lane] <= s - 2.0f) break;
        safe_lane = (safe_lane + 1) % 3;
    }

    // Pick pattern weighted by Douala district theme and escalating difficulty
    int roll = rng_.range_int(0, 99);
    int pattern = 0;
    if (roll < static_cast<int>(params_.ramp_combo_chance * 100.0f)) {
        pattern = 1;  // Ramp + Bus/Truck Rooftop Run!
    } else if (roll < static_cast<int>((params_.ramp_combo_chance + params_.moving_traffic_chance) * 100.0f)) {
        pattern = 2;  // Oncoming Douala Yellow Taxi & Benskin Rush!
    } else if (zone == 2 && rng_.chance(0.45f)) {
        pattern = 4;  // Marché Mboppi Pushcart & Vendor Hustle
    } else if (zone == 4 && rng_.chance(0.42f)) {
        pattern = 5;  // Pont du Wouri Twin Port Truck Convoy!
    } else {
        int sub = rng_.range_int(0, 3);
        pattern = (sub == 0) ? 0 : (sub == 1) ? 3 : (sub == 2) ? 6 : 0;
    }

    // Also periodically alternate to guarantee rich variety
    if (pattern_index_ % 7 == 2) pattern = 1;
    pattern_index_++;

    float pattern_len = 10.0f;

    if (pattern == 1) {
        // === PATTERN 1: RAMP + ROOFTOP CONVOY RUN ===
        // Place ramp in ramp_lane leading cleanly onto a Coaster Bus or Cargo Truck!
        int ramp_lane = safe_lane;
        bool use_truck = (zone == 4 || zone == 5 || rng_.chance(0.45f));
        ObstacleKind vehicle_kind = use_truck ? OBS_CARGO_TRUCK : OBS_COASTER_BUS;
        float v_half = use_truck ? 4.75f : 4.08f;
        float v_h = use_truck ? 3.15f : 2.85f;

        Obstacle ramp = make_obstacle(OBS_RAMP, ramp_lane, s, speed_scale, next_entity_id);
        ramp.height = v_h;
        obstacles.push_back(ramp);

        float bus1_s = s + ramp.half_len + v_half - 0.15f;
        Obstacle bus1 = make_obstacle(vehicle_kind, ramp_lane, bus1_s, speed_scale, next_entity_id);
        obstacles.push_back(bus1);

        // Optionally chain a 2nd bus/truck ahead with a rooftop leap gap!
        float end_s = bus1_s + v_half;
        if (rng_.chance(0.55f + 0.25f * difficulty)) {
            int next_lane = ramp_lane;
            // At higher difficulty, shift 2nd roof to adjacent lane for a thrilling mid-air roof hop!
            if (difficulty > 0.35f && rng_.chance(0.45f)) {
                next_lane = (ramp_lane == 1) ? (rng_.chance(0.5f) ? 0 : 2) : 1;
            }
            float bus2_s = end_s + 3.4f + v_half;
            Obstacle bus2 = make_obstacle(vehicle_kind, next_lane, bus2_s, speed_scale, next_entity_id);
            obstacles.push_back(bus2);
            spawn_coin_line(next_lane, bus2_s - v_half + 1.0f, 5, 1.7f, v_h + 0.95f,
                            collectibles, next_entity_id);
            maybe_spawn_powerup(next_lane, bus2_s + 1.0f, v_h + 1.15f, collectibles, next_entity_id);
            lane_busy_until_[next_lane] = std::max(lane_busy_until_[next_lane], bus2_s + v_half + 3.0f);
            end_s = bus2_s + v_half;
        } else {
            maybe_spawn_powerup(ramp_lane, bus1_s, v_h + 1.15f, collectibles, next_entity_id);
        }

        // Coin arc climbing the ramp + rooftop coin streak!
        spawn_coin_arc(ramp_lane, s, 4, 4.0f, 0.7f, v_h * 0.65f, collectibles, next_entity_id);
        spawn_coin_line(ramp_lane, bus1_s - v_half + 1.0f, 5, 1.6f, v_h + 0.95f,
                        collectibles, next_entity_id);
        lane_busy_until_[ramp_lane] = std::max(lane_busy_until_[ramp_lane], end_s + 3.0f);

        // Populate 1 other lane with a parked yellow taxi or barrier (leaving 1 ground lane open too!)
        int other_lane = (ramp_lane + 1) % 3;
        if (lane_busy_until_[other_lane] <= s) {
            ObstacleKind k = rng_.chance(0.55f) ? OBS_TAXI_PARKED : OBS_BARRIER_LOW;
            Obstacle ob = make_obstacle(k, other_lane, bus1_s - 1.0f, speed_scale, next_entity_id);
            obstacles.push_back(ob);
            lane_busy_until_[other_lane] = bus1_s + ob.half_len + 2.5f;
        }
        pattern_len = (end_s - s) + 7.0f;
    } else if (pattern == 2) {
        // === PATTERN 2: ONCOMING DOUALA YELLOW TAXI & BENSKIN TRAFFIC ===
        // Because oncoming vehicles drive toward the player, spawn them slightly further ahead
        // so the player has great sightlines to see headlights & dodge!
        int lane_a = (safe_lane + 1) % 3;
        int lane_b = (safe_lane + 2) % 3;

        float spawn_s = s + 18.0f + current_speed * 0.45f;
        ObstacleKind k1 = rng_.chance(0.58f) ? OBS_TAXI_MOVING : OBS_BENSKIN_MOVING;
        obstacles.push_back(make_obstacle(k1, lane_a, spawn_s, speed_scale, next_entity_id));

        // Second oncoming vehicle staggered in lane_b at medium/high difficulty
        if (rng_.chance(0.45f + 0.45f * params_.obstacle_density)) {
            ObstacleKind k2 = (k1 == OBS_TAXI_MOVING) ? OBS_BENSKIN_MOVING : OBS_TAXI_MOVING;
            obstacles.push_back(make_obstacle(k2, lane_b, spawn_s + rng_.range(6.0f, 12.0f),
                                              speed_scale, next_entity_id));
        }

        // Safe lane gets an inviting diagonal or straight FCFA coin guide!
        if (rng_.chance(0.45f)) {
            Obstacle low = make_obstacle(OBS_BARRIER_LOW, safe_lane, s + 2.0f, speed_scale, next_entity_id);
            obstacles.push_back(low);
            spawn_coin_arc(safe_lane, s + 2.0f, 5, 7.0f, 0.9f, 2.1f, collectibles, next_entity_id);
        } else {
            spawn_coin_line(safe_lane, s, 6, 2.2f, 0.95f, collectibles, next_entity_id);
            maybe_spawn_powerup(safe_lane, s + 13.5f, 1.15f, collectibles, next_entity_id);
        }
        pattern_len = 13.0f;
    } else if (pattern == 3) {
        // === PATTERN 3: MUNICIPAL ROADWORKS CHICANE (JUMP / ROLL / DODGE) ===
        int jump_lane = safe_lane;
        int roll_lane = (safe_lane + 1) % 3;
        int block_lane = (safe_lane + 2) % 3;

        if (lane_busy_until_[jump_lane] <= s) {
            obstacles.push_back(make_obstacle(OBS_BARRIER_LOW, jump_lane, s, speed_scale, next_entity_id));
            spawn_coin_arc(jump_lane, s, 5, 6.8f, 0.9f, 2.0f, collectibles, next_entity_id);
            lane_busy_until_[jump_lane] = s + 3.0f;
        }
        if (lane_busy_until_[roll_lane] <= s) {
            obstacles.push_back(make_obstacle(OBS_BARRIER_HIGH, roll_lane, s, speed_scale, next_entity_id));
            spawn_coin_line(roll_lane, s - 3.5f, 5, 1.8f, 0.65f, collectibles, next_entity_id);
            lane_busy_until_[roll_lane] = s + 3.0f;
        }
        if (lane_busy_until_[block_lane] <= s && rng_.chance(0.55f + 0.35f * params_.obstacle_density)) {
            ObstacleKind bk = rng_.chance(0.5f) ? OBS_BARRIER_FULL : OBS_TAXI_PARKED;
            Obstacle ob = make_obstacle(bk, block_lane, s + 1.0f, speed_scale, next_entity_id);
            obstacles.push_back(ob);
            lane_busy_until_[block_lane] = s + ob.half_len + 3.5f;
        }
        pattern_len = 10.5f;
    } else if (pattern == 4) {
        // === PATTERN 4: MARCHE MBOPPI VENDOR PUSHCART HUSTLE ===
        int cart_lane1 = (safe_lane + 1) % 3;
        int cart_lane2 = (safe_lane + 2) % 3;

        if (lane_busy_until_[cart_lane1] <= s) {
            Obstacle c1 = make_obstacle(OBS_PUSHCART, cart_lane1, s, speed_scale, next_entity_id);
            obstacles.push_back(c1);
            lane_busy_until_[cart_lane1] = s + c1.half_len + 3.0f;
        }
        if (lane_busy_until_[cart_lane2] <= s && rng_.chance(0.65f)) {
            ObstacleKind k = rng_.chance(0.55f) ? OBS_BARRIER_HIGH : OBS_PUSHCART;
            Obstacle c2 = make_obstacle(k, cart_lane2, s + 4.5f, speed_scale, next_entity_id);
            obstacles.push_back(c2);
            lane_busy_until_[cart_lane2] = s + 4.5f + c2.half_len + 3.0f;
        }

        // Diagonal coin ribbon weaving cleanly through the bustling market!
        int next_safe = (safe_lane == 1) ? (rng_.chance(0.5f) ? 0 : 2) : 1;
        spawn_coin_diagonal(safe_lane, next_safe, s - 2.0f, 6, 2.0f, 0.95f, collectibles, next_entity_id);
        maybe_spawn_powerup(next_safe, s + 10.5f, 1.15f, collectibles, next_entity_id);
        pattern_len = 12.0f;
    } else if (pattern == 5) {
        // === PATTERN 5: PONT DU WOURI PORT CONTAINER CONVOY ===
        int open_lane = safe_lane;
        int lane_a = (open_lane + 1) % 3;
        int lane_b = (open_lane + 2) % 3;

        if (lane_busy_until_[lane_a] <= s) {
            Obstacle t1 = make_obstacle(OBS_CARGO_TRUCK, lane_a, s + 4.5f, speed_scale, next_entity_id);
            obstacles.push_back(t1);
            lane_busy_until_[lane_a] = s + 4.5f + t1.half_len + 3.5f;
        }
        if (lane_busy_until_[lane_b] <= s && rng_.chance(0.60f + 0.30f * difficulty)) {
            Obstacle t2 = make_obstacle(OBS_COASTER_BUS, lane_b, s + 6.5f, speed_scale, next_entity_id);
            obstacles.push_back(t2);
            lane_busy_until_[lane_b] = s + 6.5f + t2.half_len + 3.5f;
        }

        // Open lane can have a slide-under or low barrier for exhilarating weaving!
        if (rng_.chance(0.50f)) {
            Obstacle high = make_obstacle(OBS_BARRIER_HIGH, open_lane, s + 4.0f, speed_scale, next_entity_id);
            obstacles.push_back(high);
            spawn_coin_line(open_lane, s, 6, 2.0f, 0.65f, collectibles, next_entity_id);
        } else {
            spawn_coin_line(open_lane, s, 6, 2.0f, 0.95f, collectibles, next_entity_id);
        }
        pattern_len = 14.5f;
    } else if (pattern == 6) {
        // === PATTERN 6: DOUALA DRAINAGE TRENCH / POTHOLE JUMP ===
        if (lane_busy_until_[safe_lane] <= s) {
            Obstacle pit = make_obstacle(OBS_POTHOLE, safe_lane, s, speed_scale, next_entity_id);
            obstacles.push_back(pit);
            spawn_coin_arc(safe_lane, s, 5, 7.2f, 0.95f, 2.15f, collectibles, next_entity_id);
            lane_busy_until_[safe_lane] = s + 3.5f;
        }
        int flank = (safe_lane + 1) % 3;
        if (lane_busy_until_[flank] <= s) {
            ObstacleKind k = rng_.chance(0.55f) ? OBS_TAXI_PARKED : OBS_BARRIER_FULL;
            Obstacle ob = make_obstacle(k, flank, s + 1.0f, speed_scale, next_entity_id);
            obstacles.push_back(ob);
            lane_busy_until_[flank] = s + 1.0f + ob.half_len + 3.0f;
        }
        int flank2 = (safe_lane + 2) % 3;
        if (lane_busy_until_[flank2] <= s && rng_.chance(0.45f + 0.35f * difficulty)) {
            Obstacle ob = make_obstacle(OBS_POTHOLE, flank2, s + 2.0f, speed_scale, next_entity_id);
            obstacles.push_back(ob);
            lane_busy_until_[flank2] = s + 5.0f;
        }
        pattern_len = 10.0f;
    } else {
        // === PATTERN 0: CLASSIC DOUALA BOULEVARD MIX ===
        int lane_a = (safe_lane + 1) % 3;
        int lane_b = (safe_lane + 2) % 3;

        if (lane_busy_until_[lane_a] <= s) {
            ObstacleKind ka = rng_.chance(0.5f) ? OBS_TAXI_PARKED : OBS_COASTER_BUS;
            Obstacle oa = make_obstacle(ka, lane_a, s + 2.0f, speed_scale, next_entity_id);
            obstacles.push_back(oa);
            lane_busy_until_[lane_a] = s + 2.0f + oa.half_len + 3.0f;
        }
        if (lane_busy_until_[lane_b] <= s && rng_.chance(0.50f + 0.40f * params_.obstacle_density)) {
            ObstacleKind kb = rng_.chance(0.5f) ? OBS_BARRIER_HIGH : OBS_BARRIER_LOW;
            Obstacle ob = make_obstacle(kb, lane_b, s + 1.0f, speed_scale, next_entity_id);
            obstacles.push_back(ob);
            lane_busy_until_[lane_b] = s + 4.0f;
        }
        spawn_coin_line(safe_lane, s - 1.0f, 5, 2.0f, 0.95f, collectibles, next_entity_id);
        maybe_spawn_powerup(safe_lane, s + 9.5f, 1.15f, collectibles, next_entity_id);
        pattern_len = 11.5f;
    }

    // Advance next_obstacle_s_ with speed-aware safety breathing room that tightens as difficulty rises
    float min_gap = current_speed * lerpf(0.68f, 0.44f, params_.obstacle_density) + 3.5f;
    float extra_gap = rng_.range(1.5f, 5.5f) * (1.15f - 0.5f * difficulty);
    next_obstacle_s_ = s + std::max(pattern_len, min_gap) + extra_gap;
}

void WorldGenerator::update(float player_s, float current_speed,
                            std::vector<Obstacle>& obstacles,
                            std::vector<Collectible>& collectibles,
                            std::vector<SceneryProp>& scenery,
                            std::vector<NpcEntity>& npcs,
                            std::vector<float>& road_segments,
                            std::vector<bool>& road_is_bridge,
                            int& next_entity_id) {
    const float lookahead = 155.0f;
    const float cull_behind = player_s - 32.0f;

    // 1. Spawn road / Pont du Wouri bridge segments ahead
    while (next_road_s_ < player_s + lookahead + ROAD_SEGMENT_LEN) {
        int zone = active_zone_at(next_road_s_);
        bool bridge = (zone == 4) || params_.is_bridge;
        road_segments.push_back(next_road_s_);
        road_is_bridge.push_back(bridge);
        next_road_s_ += ROAD_SEGMENT_LEN;
    }

    // 2. Spawn Douala roadside architecture, palms, umbrellas, monuments, cranes & PNJs
    while (next_scenery_s_ < player_s + lookahead) {
        int zone = active_zone_at(next_scenery_s_);
        spawn_scenery_row(next_scenery_s_, zone, scenery, npcs, next_entity_id);
        next_scenery_s_ += 12.0f;
    }

    // 3. Spawn finish line archway for finite Douala levels
    if (!params_.is_endless && !finish_spawned_ &&
        player_s + lookahead >= params_.target_distance) {
        SceneryProp arch;
        arch.mesh = MESH_FINISH_ARCH;
        arch.x = 0.0f;
        arch.y = 0.0f;
        arch.s = params_.target_distance;
        arch.yaw = 0.0f;
        scenery.push_back(arch);

        // Victory coin runway across center lane leading straight to finish arch!
        spawn_coin_line(1, params_.target_distance - 18.0f, 8, 2.0f, 1.0f, collectibles, next_entity_id);
        finish_spawned_ = true;
    }

    // 4. Spawn guaranteed-solvable obstacle patterns
    while (next_obstacle_s_ < player_s + lookahead) {
        if (!params_.is_endless && next_obstacle_s_ > params_.target_distance - 26.0f) {
            next_obstacle_s_ = params_.target_distance + 1000.0f;
            break;
        }
        int zone = active_zone_at(next_obstacle_s_);
        spawn_obstacle_pattern(next_obstacle_s_, current_speed, zone,
                               obstacles, collectibles, next_entity_id);
    }

    // 5. Cull passed entities behind camera to keep memory & CPU flat
    obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
                                   [cull_behind](const Obstacle& o) {
                                       return o.s + o.half_len < cull_behind;
                                   }),
                    obstacles.end());

    collectibles.erase(std::remove_if(collectibles.begin(), collectibles.end(),
                                      [cull_behind](const Collectible& c) {
                                          return c.collected || c.s < cull_behind;
                                      }),
                       collectibles.end());

    scenery.erase(std::remove_if(scenery.begin(), scenery.end(),
                                 [cull_behind](const SceneryProp& p) {
                                     return p.s < cull_behind - 18.0f;
                                 }),
                  scenery.end());

    npcs.erase(std::remove_if(npcs.begin(), npcs.end(),
                              [cull_behind](const NpcEntity& n) {
                                  return n.s < cull_behind - 14.0f;
                              }),
               npcs.end());

    while (!road_segments.empty() && road_segments.front() + ROAD_SEGMENT_LEN < cull_behind) {
        road_segments.erase(road_segments.begin());
        road_is_bridge.erase(road_is_bridge.begin());
    }
}

}  // namespace maximus
