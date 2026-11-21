#pragma once

#include "types.hpp"
#include <array>
#include <vector>

namespace maximus {

constexpr float LANE_WIDTH = 2.8f;
constexpr float LANE_X[3] = {-2.8f, 0.0f, 2.8f};
constexpr float ROAD_SEGMENT_LEN = 24.0f;

class WorldGenerator {
public:
    WorldGenerator() = default;

    void reset(const LevelParams& params);

    // Populate upcoming road segments, scenery, PNJ pedestrians, obstacles, coins, and power-ups
    // ahead of player_s up to player_s + lookahead.
    void update(float player_s, float current_speed,
                std::vector<Obstacle>& obstacles,
                std::vector<Collectible>& collectibles,
                std::vector<SceneryProp>& scenery,
                std::vector<NpcEntity>& npcs,
                std::vector<float>& road_segments,
                std::vector<bool>& road_is_bridge,
                int& next_entity_id);

    int active_zone_at(float s) const;

private:
    LevelParams params_;
    FastRng rng_;
    float next_obstacle_s_ = 36.0f;
    float next_scenery_s_ = -24.0f;
    float next_road_s_ = -24.0f;
    float next_monument_s_ = 140.0f;
    bool finish_spawned_ = false;
    std::array<float, 3> lane_busy_until_ = {0.0f, 0.0f, 0.0f};
    int pattern_index_ = 0;

    void spawn_scenery_row(float s, int zone, std::vector<SceneryProp>& scenery,
                           std::vector<NpcEntity>& npcs, int& next_id);
    void spawn_obstacle_pattern(float s, float current_speed, int zone,
                                std::vector<Obstacle>& obstacles,
                                std::vector<Collectible>& collectibles,
                                int& next_entity_id);
    void spawn_coin_line(int lane, float s_start, int count, float spacing, float y,
                         std::vector<Collectible>& collectibles, int& next_id);
    void spawn_coin_arc(int lane, float s_center, int count, float span, float base_y, float peak_h,
                        std::vector<Collectible>& collectibles, int& next_id);
    void spawn_coin_diagonal(int lane_from, int lane_to, float s_start, int count, float spacing,
                             float y, std::vector<Collectible>& collectibles, int& next_id);
    void maybe_spawn_powerup(int lane, float s, float y,
                             std::vector<Collectible>& collectibles, int& next_id);

    Obstacle make_obstacle(ObstacleKind kind, int lane, float s, float speed_scale, int& next_id);
};

}  // namespace maximus
