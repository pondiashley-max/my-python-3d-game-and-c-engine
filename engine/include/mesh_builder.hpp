#pragma once

#include "types.hpp"
#include <array>

namespace maximus {

// Procedural C++ 3D mesh generator for all Douala vehicles, obstacles, landmarks,
// articulated character parts, collectibles, and VFX.
class MeshBuilder {
public:
    static std::array<MeshData, MESH_COUNT> build_all_meshes();

private:
    static void add_box(MeshData& m, Vec3 center, Vec3 size, Vec3 color, float mat = 0.0f,
                        float u_scale = 1.0f, float v_scale = 1.0f);
    static void add_sloped_box(MeshData& m, Vec3 center, Vec3 size, float top_shift_z,
                               float top_scale_x, float top_scale_z, Vec3 color, float mat = 0.0f);
    static void add_cylinder(MeshData& m, Vec3 base_center, float r_bottom, float r_top,
                             float height, int segs, Vec3 color, float mat = 0.0f);
    static void add_wheel(MeshData& m, Vec3 center, float radius, float width, int segs,
                          Vec3 tire_color, Vec3 rim_color);
    static void add_wedge_ramp(MeshData& m, Vec3 center, float width, float height, float length,
                               Vec3 color, Vec3 stripe_color);
    static void add_beam(MeshData& m, Vec3 a, Vec3 b, float thickness, Vec3 color, float mat = 1.0f);
    static void add_quad(MeshData& m, Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3,
                         Vec3 color, float mat = 0.0f, Vec2 uv0 = {0, 0}, Vec2 uv1 = {1, 0},
                         Vec2 uv2 = {1, 1}, Vec2 uv3 = {0, 1});
    static void add_tri(MeshData& m, Vec3 v0, Vec3 v1, Vec3 v2, Vec3 color, float mat = 0.0f);

    static MeshData build_road_boulevard();
    static MeshData build_bridge_wouri();
    static MeshData build_water_plane();
    static MeshData build_taxi_yellow();
    static MeshData build_benskin();
    static MeshData build_coaster_bus();
    static MeshData build_cargo_truck();
    static MeshData build_ramp_wood();
    static MeshData build_barrier_low();
    static MeshData build_barrier_high();
    static MeshData build_barrier_full();
    static MeshData build_pothole_trench();
    static MeshData build_pushcart_plantain();
    static MeshData build_char_torso();
    static MeshData build_char_head();
    static MeshData build_char_limb();
    static MeshData build_char_forelimb();
    static MeshData build_char_shoe();
    static MeshData build_hoverboard();
    static MeshData build_jetpack();
    static MeshData build_headlight_beam();
    static MeshData build_coin();
    static MeshData build_pu_magnet();
    static MeshData build_pu_sneakers();
    static MeshData build_pu_jetpack();
    static MeshData build_pu_multiplier();
    static MeshData build_pu_hoverboard();
    static MeshData build_building_modern();
    static MeshData build_building_shop();
    static MeshData build_market_umbrella();
    static MeshData build_palm_tree();
    static MeshData build_streetlamp();
    static MeshData build_billboard();
    static MeshData build_monument_liberte();
    static MeshData build_port_crane();
    static MeshData build_finish_arch();
    static MeshData build_particle_cube();
    static MeshData build_rain_streak();
    static MeshData build_shadow_blob();
};

}  // namespace maximus
