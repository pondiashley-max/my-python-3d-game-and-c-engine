#include "mesh_builder.hpp"
#include <cmath>

namespace maximus {

namespace {

inline void push_vert(MeshData& m, Vec3 p, Vec3 n, Vec2 uv, Vec3 col, float mat) {
    m.vertices.push_back({p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y, col.x, col.y, col.z, mat});
}

}  // namespace

void MeshBuilder::add_quad(MeshData& m, Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3,
                           Vec3 color, float mat, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3) {
    Vec3 n = cross(v1 - v0, v2 - v0).normalized();
    uint32_t base = static_cast<uint32_t>(m.vertices.size());
    push_vert(m, v0, n, uv0, color, mat);
    push_vert(m, v1, n, uv1, color, mat);
    push_vert(m, v2, n, uv2, color, mat);
    push_vert(m, v3, n, uv3, color, mat);
    m.indices.push_back(base + 0);
    m.indices.push_back(base + 1);
    m.indices.push_back(base + 2);
    m.indices.push_back(base + 0);
    m.indices.push_back(base + 2);
    m.indices.push_back(base + 3);
}

void MeshBuilder::add_tri(MeshData& m, Vec3 v0, Vec3 v1, Vec3 v2, Vec3 color, float mat) {
    Vec3 n = cross(v1 - v0, v2 - v0).normalized();
    uint32_t base = static_cast<uint32_t>(m.vertices.size());
    push_vert(m, v0, n, {0.0f, 0.0f}, color, mat);
    push_vert(m, v1, n, {1.0f, 0.0f}, color, mat);
    push_vert(m, v2, n, {0.5f, 1.0f}, color, mat);
    m.indices.push_back(base + 0);
    m.indices.push_back(base + 1);
    m.indices.push_back(base + 2);
}

void MeshBuilder::add_box(MeshData& m, Vec3 c, Vec3 s, Vec3 col, float mat,
                          float u_scale, float v_scale) {
    float hx = s.x * 0.5f, hy = s.y * 0.5f, hz = s.z * 0.5f;
    Vec3 p000(c.x - hx, c.y - hy, c.z - hz);
    Vec3 p100(c.x + hx, c.y - hy, c.z - hz);
    Vec3 p110(c.x + hx, c.y + hy, c.z - hz);
    Vec3 p010(c.x - hx, c.y + hy, c.z - hz);
    Vec3 p001(c.x - hx, c.y - hy, c.z + hz);
    Vec3 p101(c.x + hx, c.y - hy, c.z + hz);
    Vec3 p111(c.x + hx, c.y + hy, c.z + hz);
    Vec3 p011(c.x - hx, c.y + hy, c.z + hz);

    Vec2 u0(0, 0), u1(u_scale, 0), u2(u_scale, v_scale), u3(0, v_scale);
    // +Z front (facing camera)
    add_quad(m, p001, p101, p111, p011, col, mat, u0, u1, u2, u3);
    // -Z back
    add_quad(m, p100, p000, p010, p110, col, mat, u0, u1, u2, u3);
    // +X right
    add_quad(m, p101, p100, p110, p111, col, mat, u0, u1, u2, u3);
    // -X left
    add_quad(m, p000, p001, p011, p010, col, mat, u0, u1, u2, u3);
    // +Y top
    add_quad(m, p011, p111, p110, p010, col, mat, u0, u1, u2, u3);
    // -Y bottom
    add_quad(m, p000, p100, p101, p001, col, mat, u0, u1, u2, u3);
}

void MeshBuilder::add_sloped_box(MeshData& m, Vec3 c, Vec3 s, float top_shift_z,
                                 float top_scale_x, float top_scale_z, Vec3 col, float mat) {
    float hx = s.x * 0.5f, hy = s.y * 0.5f, hz = s.z * 0.5f;
    float thx = hx * top_scale_x, thz = hz * top_scale_z;

    Vec3 b0(c.x - hx, c.y - hy, c.z - hz);
    Vec3 b1(c.x + hx, c.y - hy, c.z - hz);
    Vec3 b2(c.x + hx, c.y - hy, c.z + hz);
    Vec3 b3(c.x - hx, c.y - hy, c.z + hz);

    Vec3 t0(c.x - thx, c.y + hy, c.z - thz + top_shift_z);
    Vec3 t1(c.x + thx, c.y + hy, c.z - thz + top_shift_z);
    Vec3 t2(c.x + thx, c.y + hy, c.z + thz + top_shift_z);
    Vec3 t3(c.x - thx, c.y + hy, c.z + thz + top_shift_z);

    add_quad(m, b3, b2, t2, t3, col, mat);
    add_quad(m, b1, b0, t0, t1, col, mat);
    add_quad(m, b2, b1, t1, t2, col, mat);
    add_quad(m, b0, b3, t3, t0, col, mat);
    add_quad(m, t3, t2, t1, t0, col, mat);
    add_quad(m, b0, b1, b2, b3, col, mat);
}

void MeshBuilder::add_cylinder(MeshData& m, Vec3 base, float rb, float rt,
                               float h, int segs, Vec3 col, float mat) {
    for (int i = 0; i < segs; ++i) {
        float a0 = (static_cast<float>(i) / segs) * TWO_PI;
        float a1 = (static_cast<float>(i + 1) / segs) * TWO_PI;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        Vec3 b0(base.x + rb * c0, base.y, base.z + rb * s0);
        Vec3 b1(base.x + rb * c1, base.y, base.z + rb * s1);
        Vec3 t1(base.x + rt * c1, base.y + h, base.z + rt * s1);
        Vec3 t0(base.x + rt * c0, base.y + h, base.z + rt * s0);

        // Winding outward
        add_quad(m, b1, b0, t0, t1, col, mat);
        if (rt > 1e-4f) {
            add_tri(m, {base.x, base.y + h, base.z}, t1, t0, col, mat);
        }
        if (rb > 1e-4f) {
            add_tri(m, {base.x, base.y, base.z}, b0, b1, col, mat);
        }
    }
}

void MeshBuilder::add_wheel(MeshData& m, Vec3 center, float radius, float width, int segs,
                            Vec3 tire_col, Vec3 rim_col) {
    float hw = width * 0.5f;
    float rim_r = radius * 0.56f;
    for (int i = 0; i < segs; ++i) {
        float a0 = (static_cast<float>(i) / segs) * TWO_PI;
        float a1 = (static_cast<float>(i + 1) / segs) * TWO_PI;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        Vec3 l0(center.x - hw, center.y + radius * s0, center.z + radius * c0);
        Vec3 l1(center.x - hw, center.y + radius * s1, center.z + radius * c1);
        Vec3 r0(center.x + hw, center.y + radius * s0, center.z + radius * c0);
        Vec3 r1(center.x + hw, center.y + radius * s1, center.z + radius * c1);

        // Tread
        add_quad(m, l0, r0, r1, l1, tire_col, 0.0f);
        // Sidewalls
        Vec3 lc(center.x - hw, center.y, center.z);
        Vec3 rc(center.x + hw, center.y, center.z);
        add_tri(m, lc, l1, l0, tire_col, 0.0f);
        add_tri(m, rc, r0, r1, tire_col, 0.0f);

        // Alloy rim caps slightly proud of sidewall
        Vec3 rl0(center.x - hw - 0.015f, center.y + rim_r * s0, center.z + rim_r * c0);
        Vec3 rl1(center.x - hw - 0.015f, center.y + rim_r * s1, center.z + rim_r * c1);
        Vec3 rr0(center.x + hw + 0.015f, center.y + rim_r * s0, center.z + rim_r * c0);
        Vec3 rr1(center.x + hw + 0.015f, center.y + rim_r * s1, center.z + rim_r * c1);
        add_tri(m, {center.x - hw - 0.015f, center.y, center.z}, rl1, rl0, rim_col, 1.0f);
        add_tri(m, {center.x + hw + 0.015f, center.y, center.z}, rr0, rr1, rim_col, 1.0f);
    }
}

void MeshBuilder::add_wedge_ramp(MeshData& m, Vec3 c, float w, float h, float len,
                                 Vec3 col, Vec3 stripe_col) {
    float hw = w * 0.5f, hl = len * 0.5f;
    // Low edge at +Z (facing approaching runner), high edge at -Z (joining bus/truck roof)
    Vec3 fl(c.x - hw, c.y, c.z + hl);
    Vec3 fr(c.x + hw, c.y, c.z + hl);
    Vec3 bl(c.x - hw, c.y, c.z - hl);
    Vec3 br(c.x + hw, c.y, c.z - hl);
    Vec3 tl(c.x - hw, c.y + h, c.z - hl);
    Vec3 tr(c.x + hw, c.y + h, c.z - hl);

    // Main sloped running deck
    add_quad(m, fl, fr, tr, tl, col, 0.0f);
    // Back vertical support
    add_quad(m, br, bl, tl, tr, stripe_col, 0.0f);
    // Left & right triangular cheeks
    add_tri(m, bl, fl, tl, stripe_col, 0.0f);
    add_tri(m, fr, br, tr, stripe_col, 0.0f);

    // Side safety rails & yellow chevron slats on deck
    int slats = 6;
    for (int i = 0; i < slats; ++i) {
        float t0 = (static_cast<float>(i) + 0.15f) / slats;
        float t1 = (static_cast<float>(i) + 0.55f) / slats;
        Vec3 a0 = lerp(fl, tl, t0) + Vec3(0.12f, 0.03f, 0.0f);
        Vec3 a1 = lerp(fr, tr, t0) + Vec3(-0.12f, 0.03f, 0.0f);
        Vec3 b1 = lerp(fr, tr, t1) + Vec3(-0.12f, 0.03f, 0.0f);
        Vec3 b0 = lerp(fl, tl, t1) + Vec3(0.12f, 0.03f, 0.0f);
        add_quad(m, a0, a1, b1, b0, (i % 2 == 0) ? stripe_col : Vec3(0.12f, 0.12f, 0.14f), 0.0f);
    }
}

void MeshBuilder::add_beam(MeshData& m, Vec3 a, Vec3 b, float thickness, Vec3 col, float mat) {
    Vec3 dir = (b - a).normalized();
    Vec3 up_ref = (std::fabs(dir.y) > 0.92f) ? Vec3(1.0f, 0.0f, 0.0f) : Vec3(0.0f, 1.0f, 0.0f);
    Vec3 u = cross(dir, up_ref).normalized() * (thickness * 0.5f);
    Vec3 v = cross(u.normalized(), dir).normalized() * (thickness * 0.5f);

    Vec3 a0 = a - u - v, a1 = a + u - v, a2 = a + u + v, a3 = a - u + v;
    Vec3 b0 = b - u - v, b1 = b + u - v, b2 = b + u + v, b3 = b - u + v;

    add_quad(m, a0, a1, b1, b0, col, mat);
    add_quad(m, a1, a2, b2, b1, col, mat);
    add_quad(m, a2, a3, b3, b2, col, mat);
    add_quad(m, a3, a0, b0, b3, col, mat);
}

MeshData MeshBuilder::build_road_boulevard() {
    MeshData m;
    float L = 24.0f;
    float half_l = L * 0.5f;
    float road_hw = 5.2f;

    // Main Douala asphalt carriageway (material 3.0 uses procedural lane markings texture)
    add_quad(m,
             {-road_hw, 0.0f, half_l}, {road_hw, 0.0f, half_l},
             {road_hw, 0.0f, -half_l}, {-road_hw, 0.0f, -half_l},
             {0.92f, 0.92f, 0.95f}, 3.0f,
             {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 4.0f}, {0.0f, 4.0f});

    // Raised Douala red-and-white painted curbs on both sides!
    int curb_segs = 8;
    float seg_l = L / curb_segs;
    for (int i = 0; i < curb_segs; ++i) {
        float zc = -half_l + (i + 0.5f) * seg_l;
        Vec3 curb_col = (i % 2 == 0) ? Vec3(0.86f, 0.22f, 0.18f) : Vec3(0.93f, 0.92f, 0.88f);
        add_box(m, {-road_hw - 0.22f, 0.13f, zc}, {0.44f, 0.26f, seg_l - 0.02f}, curb_col, 0.0f);
        add_box(m, {road_hw + 0.22f, 0.13f, zc}, {0.44f, 0.26f, seg_l - 0.02f}, curb_col, 0.0f);
    }

    // Concrete sidewalks + tropical red laterite earth verges beyond
    Vec3 sidewalk(0.68f, 0.64f, 0.59f);
    Vec3 laterite(0.56f, 0.30f, 0.18f);
    add_box(m, {-road_hw - 2.65f, 0.10f, 0.0f}, {4.4f, 0.22f, L}, sidewalk, 0.0f);
    add_box(m, {road_hw + 2.65f, 0.10f, 0.0f}, {4.4f, 0.22f, L}, sidewalk, 0.0f);
    add_box(m, {-road_hw - 12.0f, 0.02f, 0.0f}, {14.5f, 0.14f, L}, laterite, 0.0f);
    add_box(m, {road_hw + 12.0f, 0.02f, 0.0f}, {14.5f, 0.14f, L}, laterite, 0.0f);

    return m;
}

MeshData MeshBuilder::build_bridge_wouri() {
    MeshData m;
    float L = 24.0f;
    float half_l = L * 0.5f;
    float road_hw = 5.2f;

    // Pont du Wouri asphalt road deck
    add_quad(m,
             {-road_hw, 0.0f, half_l}, {road_hw, 0.0f, half_l},
             {road_hw, 0.0f, -half_l}, {-road_hw, 0.0f, -half_l},
             {0.88f, 0.88f, 0.92f}, 3.0f,
             {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 4.0f}, {0.0f, 4.0f});

    // Concrete bridge walkways & heavy under-deck girder
    Vec3 concrete(0.65f, 0.66f, 0.68f);
    Vec3 steel(0.24f, 0.36f, 0.48f);
    Vec3 accent(0.88f, 0.65f, 0.16f);

    add_box(m, {0.0f, -0.75f, 0.0f}, {14.6f, 1.45f, L}, concrete, 0.0f);
    add_box(m, {-road_hw - 1.1f, 0.18f, 0.0f}, {2.2f, 0.36f, L}, concrete, 0.0f);
    add_box(m, {road_hw + 1.1f, 0.18f, 0.0f}, {2.2f, 0.36f, L}, concrete, 0.0f);

    // Bridge concrete piers extending down into the Wouri River
    add_box(m, {-4.8f, -6.5f, 0.0f}, {1.8f, 11.5f, 2.4f}, concrete * 0.82f, 0.0f);
    add_box(m, {4.8f, -6.5f, 0.0f}, {1.8f, 11.5f, 2.4f}, concrete * 0.82f, 0.0f);

    // Pont du Wouri steel side rails & dramatic overhead truss arches!
    for (float side : {-1.0f, 1.0f}) {
        float rx = side * (road_hw + 1.9f);
        // Horizontal upper & mid guardrails
        add_box(m, {rx, 1.15f, 0.0f}, {0.22f, 0.18f, L}, steel, 1.0f);
        add_box(m, {rx, 0.62f, 0.0f}, {0.16f, 0.14f, L}, accent, 0.0f);
        // Top longitudinal truss beam
        add_box(m, {rx, 6.8f, 0.0f}, {0.42f, 0.42f, L}, steel, 1.0f);
        // Vertical stanchions
        for (float z : {-8.0f, 0.0f, 8.0f}) {
            add_box(m, {rx, 3.5f, z}, {0.38f, 6.8f, 0.38f}, steel, 1.0f);
        }
        // Crisp solid diagonal steel X-bracing beams connecting stanchions!
        add_beam(m, {rx, 1.15f, -8.0f}, {rx, 6.75f, 0.0f}, 0.24f, steel * 1.12f, 1.0f);
        add_beam(m, {rx, 6.75f, -8.0f}, {rx, 1.15f, 0.0f}, 0.24f, steel * 1.05f, 1.0f);
        add_beam(m, {rx, 1.15f, 0.0f},  {rx, 6.75f, 8.0f}, 0.24f, steel * 1.12f, 1.0f);
        add_beam(m, {rx, 6.75f, 0.0f},  {rx, 1.15f, 8.0f}, 0.24f, steel * 1.05f, 1.0f);
        add_beam(m, {rx, 1.15f, 8.0f},  {rx, 6.75f, 12.0f}, 0.24f, steel * 1.10f, 1.0f);
        add_beam(m, {rx, 6.75f, -12.0f},{rx, 1.15f, -8.0f}, 0.24f, steel * 1.10f, 1.0f);
    }
    // Overhead steel cross-girders spanning across the boulevard
    for (float z : {-8.0f, 8.0f}) {
        add_box(m, {0.0f, 6.85f, z}, {14.6f, 0.48f, 0.48f}, steel, 1.0f);
        // Warm overhead bridge lamp fixture
        add_box(m, {0.0f, 6.45f, z}, {1.6f, 0.18f, 0.32f}, {1.0f, 0.88f, 0.55f}, 2.0f);
    }
    return m;
}

MeshData MeshBuilder::build_water_plane() {
    MeshData m;
    // Subdivided shimmering Wouri River surface (follows horizon curvature cleanly!)
    const int nx = 14;
    const int nz = 22;
    const float x_min = -110.0f, x_max = 110.0f;
    const float z_min = -160.0f, z_max = 60.0f;
    Vec3 col(0.08f, 0.40f, 0.56f);
    for (int iz = 0; iz < nz; ++iz) {
        float tz0 = static_cast<float>(iz) / nz;
        float tz1 = static_cast<float>(iz + 1) / nz;
        float z0 = lerpf(z_max, z_min, tz0);
        float z1 = lerpf(z_max, z_min, tz1);
        for (int ix = 0; ix < nx; ++ix) {
            float tx0 = static_cast<float>(ix) / nx;
            float tx1 = static_cast<float>(ix + 1) / nx;
            float x0 = lerpf(x_min, x_max, tx0);
            float x1 = lerpf(x_min, x_max, tx1);
            add_quad(m,
                     {x0, 0.0f, z0}, {x1, 0.0f, z0},
                     {x1, 0.0f, z1}, {x0, 0.0f, z1},
                     col, 6.0f,
                     {tx0 * 16.0f, tz0 * 20.0f}, {tx1 * 16.0f, tz0 * 20.0f},
                     {tx1 * 16.0f, tz1 * 20.0f}, {tx0 * 16.0f, tz1 * 20.0f});
        }
    }
    return m;
}

MeshData MeshBuilder::build_taxi_yellow() {
    MeshData m;
    // Authentic Douala "Taxi Jaune" (bright iconic taxi yellow + checkered trim + roof sign!)
    Vec3 yellow(0.98f, 0.78f, 0.06f);
    Vec3 dark_yellow(0.84f, 0.63f, 0.03f);
    Vec3 glass(0.14f, 0.22f, 0.30f);
    Vec3 bumper(0.16f, 0.16f, 0.18f);
    Vec3 tire(0.10f, 0.10f, 0.11f);
    Vec3 rim(0.78f, 0.80f, 0.84f);
    Vec3 headlight(1.0f, 0.96f, 0.78f);
    Vec3 taillight(0.96f, 0.12f, 0.10f);
    Vec3 white_plate(0.95f, 0.95f, 0.92f);
    Vec3 cam_green(0.05f, 0.52f, 0.26f);

    // Lower chassis & fenders (length 4.5, width 1.92)
    add_box(m, {0.0f, 0.58f, 0.0f}, {1.92f, 0.58f, 4.46f}, yellow, 1.0f);
    // Front hood bevel
    add_sloped_box(m, {0.0f, 0.90f, 1.50f}, {1.84f, 0.12f, 1.35f}, -0.10f, 0.96f, 0.90f, yellow, 1.0f);
    // Sculpted passenger cabin
    add_sloped_box(m, {0.0f, 1.18f, -0.12f}, {1.76f, 0.62f, 2.30f}, -0.06f, 0.86f, 0.72f, yellow, 1.0f);
    // Windshields & side windows (slightly proud dark glass)
    add_sloped_box(m, {0.0f, 1.18f, -0.12f}, {1.80f, 0.46f, 2.02f}, -0.05f, 0.88f, 0.76f, glass, 1.0f);

    // Iconic Douala taxi side racing stripe (black & white checkered blocks!)
    for (int i = 0; i < 10; ++i) {
        float z = -1.85f + i * 0.40f;
        Vec3 sq = (i % 2 == 0) ? Vec3(0.09f, 0.09f, 0.10f) : Vec3(0.96f, 0.96f, 0.96f);
        add_box(m, {-0.975f, 0.66f, z}, {0.03f, 0.13f, 0.36f}, sq, 0.0f);
        add_box(m, {0.975f, 0.66f, z}, {0.03f, 0.13f, 0.36f}, sq, 0.0f);
    }

    // Illuminated Rooftop "TAXI" sign!
    add_box(m, {0.0f, 1.53f, 0.02f}, {0.68f, 0.18f, 0.28f}, {1.0f, 0.92f, 0.38f}, 2.0f);
    add_box(m, {0.0f, 1.53f, 0.02f}, {0.72f, 0.05f, 0.30f}, bumper, 0.0f);

    // Front (+Z) & Rear (-Z) bumpers, grille, Cameroon LT license plate
    add_box(m, {0.0f, 0.34f, 2.24f}, {1.96f, 0.24f, 0.18f}, bumper, 0.0f);
    add_box(m, {0.0f, 0.34f, -2.24f}, {1.96f, 0.24f, 0.18f}, bumper, 0.0f);
    add_box(m, {0.0f, 0.60f, 2.25f}, {0.96f, 0.22f, 0.06f}, {0.14f, 0.14f, 0.16f}, 1.0f);
    add_box(m, {0.0f, 0.38f, 2.34f}, {0.52f, 0.13f, 0.02f}, white_plate, 0.0f);
    add_box(m, {-0.21f, 0.38f, 2.355f}, {0.09f, 0.13f, 0.02f}, cam_green, 0.0f);

    // Bright emissive headlights (+Z) & red taillights (-Z)
    add_box(m, {-0.68f, 0.65f, 2.25f}, {0.38f, 0.18f, 0.08f}, headlight, 2.0f);
    add_box(m, {0.68f, 0.65f, 2.25f}, {0.38f, 0.18f, 0.08f}, headlight, 2.0f);
    add_box(m, {-0.68f, 0.66f, -2.25f}, {0.38f, 0.16f, 0.08f}, taillight, 2.0f);
    add_box(m, {0.68f, 0.66f, -2.25f}, {0.38f, 0.16f, 0.08f}, taillight, 2.0f);

    // Side mirrors
    add_box(m, {-1.04f, 0.96f, 0.60f}, {0.18f, 0.11f, 0.14f}, dark_yellow, 1.0f);
    add_box(m, {1.04f, 0.96f, 0.60f}, {0.18f, 0.11f, 0.14f}, dark_yellow, 1.0f);

    // 4 wheels
    add_wheel(m, {-0.92f, 0.33f, 1.38f}, 0.33f, 0.24f, 12, tire, rim);
    add_wheel(m, {0.92f, 0.33f, 1.38f}, 0.33f, 0.24f, 12, tire, rim);
    add_wheel(m, {-0.92f, 0.33f, -1.38f}, 0.33f, 0.24f, 12, tire, rim);
    add_wheel(m, {0.92f, 0.33f, -1.38f}, 0.33f, 0.24f, 12, tire, rim);

    return m;
}

MeshData MeshBuilder::build_benskin() {
    MeshData m;
    // Iconic Douala "Benskin" moto-taxi with rider in bright reflective safety vest!
    Vec3 frame_red(0.84f, 0.14f, 0.12f);
    Vec3 metal(0.28f, 0.30f, 0.34f);
    Vec3 chrome(0.82f, 0.84f, 0.88f);
    Vec3 tire(0.10f, 0.10f, 0.11f);
    Vec3 seat(0.16f, 0.15f, 0.15f);
    Vec3 vest_neon(0.98f, 0.56f, 0.06f);
    Vec3 vest_stripe(0.88f, 0.98f, 0.22f);
    Vec3 jeans(0.16f, 0.26f, 0.44f);
    Vec3 helmet(0.12f, 0.56f, 0.30f);

    // Front (+Z) & Rear (-Z) wheels
    add_wheel(m, {0.0f, 0.32f, 0.74f}, 0.32f, 0.14f, 12, tire, chrome);
    add_wheel(m, {0.0f, 0.32f, -0.74f}, 0.32f, 0.16f, 12, tire, chrome);

    // Moto body, engine block, fuel tank, seat, exhaust pipe
    add_box(m, {0.0f, 0.52f, 0.0f}, {0.36f, 0.36f, 1.12f}, metal, 1.0f);
    add_box(m, {0.0f, 0.78f, 0.22f}, {0.38f, 0.24f, 0.56f}, frame_red, 1.0f);
    add_box(m, {0.0f, 0.76f, -0.44f}, {0.34f, 0.16f, 0.86f}, seat, 0.0f);
    add_box(m, {0.24f, 0.36f, -0.58f}, {0.09f, 0.10f, 0.92f}, chrome, 1.0f);

    // Front forks, handlebars, glowing round headlight (+Z)
    add_box(m, {-0.12f, 0.66f, 0.66f}, {0.06f, 0.66f, 0.07f}, chrome, 1.0f);
    add_box(m, {0.12f, 0.66f, 0.66f}, {0.06f, 0.66f, 0.07f}, chrome, 1.0f);
    add_box(m, {0.0f, 0.98f, 0.58f}, {0.82f, 0.07f, 0.08f}, metal, 1.0f);
    add_box(m, {0.0f, 0.86f, 0.82f}, {0.24f, 0.20f, 0.14f}, {1.0f, 0.96f, 0.72f}, 2.0f);
    add_box(m, {0.0f, 0.74f, -0.96f}, {0.18f, 0.12f, 0.08f}, {0.96f, 0.12f, 0.10f}, 2.0f);

    // Seated Benskin Rider (stylized 3D figure leaning slightly forward)
    add_box(m, {0.0f, 1.06f, -0.18f}, {0.44f, 0.52f, 0.30f}, vest_neon, 0.0f);
    add_box(m, {0.0f, 1.04f, -0.18f}, {0.46f, 0.09f, 0.32f}, vest_stripe, 2.0f);
    // Rider legs gripping tank
    add_box(m, {-0.23f, 0.64f, -0.06f}, {0.14f, 0.44f, 0.24f}, jeans, 0.0f);
    add_box(m, {0.23f, 0.64f, -0.06f}, {0.14f, 0.44f, 0.24f}, jeans, 0.0f);
    // Rider arms reaching to handlebars
    add_box(m, {-0.28f, 1.06f, 0.22f}, {0.12f, 0.12f, 0.62f}, vest_neon * 0.85f, 0.0f);
    add_box(m, {0.28f, 1.06f, 0.22f}, {0.12f, 0.12f, 0.62f}, vest_neon * 0.85f, 0.0f);
    // Helmet & dark visor
    add_box(m, {0.0f, 1.44f, -0.12f}, {0.28f, 0.28f, 0.30f}, helmet, 1.0f);
    add_box(m, {0.0f, 1.44f, 0.02f}, {0.26f, 0.12f, 0.06f}, {0.10f, 0.12f, 0.16f}, 1.0f);

    return m;
}

MeshData MeshBuilder::build_coaster_bus() {
    MeshData m;
    // Douala Coaster / SOCATUR urban minibus (runnable roof at y = 2.85!)
    Vec3 cream(0.95f, 0.94f, 0.90f);
    Vec3 transit_blue(0.08f, 0.38f, 0.72f);
    Vec3 gold_stripe(0.96f, 0.74f, 0.12f);
    Vec3 glass(0.12f, 0.20f, 0.28f);
    Vec3 tire(0.10f, 0.10f, 0.11f);
    Vec3 rim(0.75f, 0.78f, 0.82f);

    // Lower transit-blue hull
    add_box(m, {0.0f, 0.88f, 0.0f}, {2.24f, 1.12f, 8.10f}, transit_blue, 1.0f);
    // Gold waist stripe
    add_box(m, {0.0f, 1.46f, 0.0f}, {2.28f, 0.14f, 8.14f}, gold_stripe, 0.0f);
    // Upper white body up to flat runnable roof at y=2.85
    add_box(m, {0.0f, 2.16f, 0.0f}, {2.22f, 1.34f, 8.06f}, cream, 1.0f);
    // Panoramic dark tinted windows along both sides + front & back
    add_box(m, {0.0f, 2.04f, 0.0f}, {2.26f, 0.82f, 7.20f}, glass, 1.0f);
    add_box(m, {0.0f, 2.06f, 4.04f}, {2.00f, 0.92f, 0.06f}, glass, 1.0f);
    add_box(m, {0.0f, 2.06f, -4.04f}, {1.92f, 0.78f, 0.06f}, glass, 1.0f);

    // Window pillars so it reads cleanly as a passenger bus
    for (int i = -3; i <= 3; ++i) {
        add_box(m, {-1.135f, 2.04f, i * 1.12f}, {0.04f, 0.84f, 0.14f}, cream, 0.0f);
        add_box(m, {1.135f, 2.04f, i * 1.12f}, {0.04f, 0.84f, 0.14f}, cream, 0.0f);
    }

    // Destination LED sign ("AKWA - DEIDO") above front windshield
    add_box(m, {0.0f, 2.66f, 4.05f}, {1.35f, 0.22f, 0.06f}, {1.0f, 0.76f, 0.16f}, 2.0f);

    // Subtle rooftop grip rails (low profile so running on roof looks great!)
    add_box(m, {-0.98f, 2.88f, 0.0f}, {0.10f, 0.08f, 7.60f}, {0.72f, 0.74f, 0.78f}, 1.0f);
    add_box(m, {0.98f, 2.88f, 0.0f}, {0.10f, 0.08f, 7.60f}, {0.72f, 0.74f, 0.78f}, 1.0f);

    // Bumpers, headlights (+Z), taillights (-Z)
    add_box(m, {0.0f, 0.42f, 4.08f}, {2.28f, 0.28f, 0.18f}, {0.18f, 0.18f, 0.20f}, 0.0f);
    add_box(m, {0.0f, 0.42f, -4.08f}, {2.28f, 0.28f, 0.18f}, {0.18f, 0.18f, 0.20f}, 0.0f);
    add_box(m, {-0.78f, 0.82f, 4.08f}, {0.42f, 0.22f, 0.08f}, {1.0f, 0.96f, 0.78f}, 2.0f);
    add_box(m, {0.78f, 0.82f, 4.08f}, {0.42f, 0.22f, 0.08f}, {1.0f, 0.96f, 0.78f}, 2.0f);
    add_box(m, {-0.78f, 0.86f, -4.08f}, {0.40f, 0.22f, 0.08f}, {0.96f, 0.12f, 0.10f}, 2.0f);
    add_box(m, {0.78f, 0.86f, -4.08f}, {0.40f, 0.22f, 0.08f}, {0.96f, 0.12f, 0.10f}, 2.0f);

    // 4 heavy wheels
    add_wheel(m, {-1.08f, 0.42f, 2.55f}, 0.42f, 0.30f, 12, tire, rim);
    add_wheel(m, {1.08f, 0.42f, 2.55f}, 0.42f, 0.30f, 12, tire, rim);
    add_wheel(m, {-1.08f, 0.42f, -2.55f}, 0.42f, 0.30f, 12, tire, rim);
    add_wheel(m, {1.08f, 0.42f, -2.55f}, 0.42f, 0.30f, 12, tire, rim);

    return m;
}

MeshData MeshBuilder::build_cargo_truck() {
    MeshData m;
    // Port Autonome de Douala Container Truck (runnable top at y=3.15!)
    Vec3 cab_col(0.86f, 0.24f, 0.14f);
    Vec3 container_col(0.14f, 0.50f, 0.56f);
    Vec3 container_rib(0.11f, 0.40f, 0.46f);
    Vec3 chassis(0.18f, 0.19f, 0.21f);
    Vec3 glass(0.12f, 0.20f, 0.28f);
    Vec3 chrome(0.82f, 0.84f, 0.88f);
    Vec3 tire(0.10f, 0.10f, 0.11f);

    // Flatbed steel chassis running full 9.5m length
    add_box(m, {0.0f, 0.68f, 0.0f}, {2.24f, 0.36f, 9.40f}, chassis, 0.0f);

    // Tractor cab at +Z end (z in [2.45, 4.70])
    add_box(m, {0.0f, 1.84f, 3.55f}, {2.28f, 2.02f, 2.25f}, cab_col, 1.0f);
    // Cab wind-deflector fairing rising smoothly to container height (y=3.15)
    add_sloped_box(m, {0.0f, 3.00f, 3.45f}, {2.12f, 0.30f, 1.80f}, -0.35f, 0.94f, 0.55f, cab_col, 1.0f);
    // Cab windshield & chrome grille
    add_box(m, {0.0f, 2.18f, 4.69f}, {2.02f, 0.78f, 0.06f}, glass, 1.0f);
    add_box(m, {0.0f, 1.24f, 4.69f}, {1.45f, 0.56f, 0.08f}, chrome, 1.0f);
    // Dual exhaust stacks behind cab
    add_cylinder(m, {-1.16f, 0.85f, 2.38f}, 0.08f, 0.08f, 2.35f, 8, chrome, 1.0f);
    add_cylinder(m, {1.16f, 0.85f, 2.38f}, 0.08f, 0.08f, 2.35f, 8, chrome, 1.0f);

    // Shipping Container (z in [-4.72, 2.28], top at y=3.15!)
    add_box(m, {0.0f, 2.01f, -1.22f}, {2.34f, 2.28f, 7.00f}, container_col, 0.0f);
    // Corrugated ribs along container sides
    for (int i = 0; i < 11; ++i) {
        float z = -4.40f + i * 0.64f;
        add_box(m, {-1.185f, 2.01f, z}, {0.05f, 2.10f, 0.18f}, container_rib, 0.0f);
        add_box(m, {1.185f, 2.01f, z}, {0.05f, 2.10f, 0.18f}, container_rib, 0.0f);
    }
    // White port stripe ("PORT DE DOUALA" visual band)
    add_box(m, {-1.195f, 2.15f, -1.22f}, {0.04f, 0.36f, 5.40f}, {0.94f, 0.94f, 0.90f}, 0.0f);
    add_box(m, {1.195f, 2.15f, -1.22f}, {0.04f, 0.36f, 5.40f}, {0.94f, 0.94f, 0.90f}, 0.0f);

    // Headlights & Taillights
    add_box(m, {-0.84f, 0.96f, 4.70f}, {0.38f, 0.22f, 0.08f}, {1.0f, 0.96f, 0.78f}, 2.0f);
    add_box(m, {0.84f, 0.96f, 4.70f}, {0.38f, 0.22f, 0.08f}, {1.0f, 0.96f, 0.78f}, 2.0f);
    add_box(m, {-0.88f, 0.72f, -4.72f}, {0.38f, 0.20f, 0.08f}, {0.96f, 0.12f, 0.10f}, 2.0f);
    add_box(m, {0.88f, 0.72f, -4.72f}, {0.38f, 0.20f, 0.08f}, {0.96f, 0.12f, 0.10f}, 2.0f);

    // 6 heavy truck wheels
    for (float z : {3.50f, -2.50f, -3.75f}) {
        add_wheel(m, {-1.12f, 0.48f, z}, 0.48f, 0.34f, 12, tire, chrome);
        add_wheel(m, {1.12f, 0.48f, z}, 0.48f, 0.34f, 12, tire, chrome);
    }

    return m;
}

MeshData MeshBuilder::build_ramp_wood() {
    MeshData m;
    add_wedge_ramp(m, {0.0f, 0.0f, 0.0f}, 2.15f, 2.85f, 4.6f,
                   {0.58f, 0.42f, 0.24f}, {0.96f, 0.76f, 0.10f});
    return m;
}

MeshData MeshBuilder::build_barrier_low() {
    MeshData m;
    // Jump-over municipal roadblock barricade (height 1.05m)
    Vec3 red(0.90f, 0.18f, 0.16f);
    Vec3 white(0.96f, 0.96f, 0.94f);
    Vec3 steel(0.28f, 0.30f, 0.33f);
    Vec3 amber(1.0f, 0.68f, 0.10f);

    // Twin A-frame steel legs
    for (float sx : {-0.92f, 0.92f}) {
        add_box(m, {sx, 0.50f, 0.0f}, {0.12f, 1.00f, 0.14f}, steel, 1.0f);
        add_box(m, {sx, 0.06f, 0.0f}, {0.22f, 0.12f, 0.56f}, steel, 1.0f);
        // Blinking amber beacon lamp on each post
        add_box(m, {sx, 1.06f, 0.0f}, {0.16f, 0.14f, 0.16f}, amber, 2.0f);
    }
    // Red & white warning planks
    int stripes = 7;
    float sw = 2.10f / stripes;
    for (int i = 0; i < stripes; ++i) {
        float x = -1.05f + (i + 0.5f) * sw;
        Vec3 c = (i % 2 == 0) ? red : white;
        add_box(m, {x, 0.80f, 0.0f}, {sw, 0.26f, 0.12f}, c, 0.0f);
        add_box(m, {x, 0.42f, 0.0f}, {sw, 0.20f, 0.10f}, c, 0.0f);
    }
    return m;
}

MeshData MeshBuilder::build_barrier_high() {
    MeshData m;
    // Slide-under overhead construction / Douala street gantry (open underneath up to y=1.30m!)
    Vec3 steel(0.26f, 0.28f, 0.32f);
    Vec3 yellow(0.98f, 0.78f, 0.08f);
    Vec3 dark(0.14f, 0.14f, 0.16f);
    Vec3 red(0.90f, 0.18f, 0.14f);

    // Left & right support posts outside the lane center so rolling underneath looks crisp
    for (float sx : {-1.16f, 1.16f}) {
        add_box(m, {sx, 1.45f, 0.0f}, {0.18f, 2.90f, 0.18f}, steel, 1.0f);
        add_box(m, {sx, 0.12f, 0.0f}, {0.42f, 0.24f, 0.54f}, dark, 0.0f);
        add_box(m, {sx, 2.96f, 0.0f}, {0.22f, 0.16f, 0.22f}, {1.0f, 0.65f, 0.08f}, 2.0f);
    }
    // Overhead warning sign board from y=1.35 to y=2.70
    add_box(m, {0.0f, 2.02f, 0.0f}, {2.42f, 1.32f, 0.18f}, yellow, 0.0f);
    // Bold black chevron stripes along top & bottom border
    for (int i = 0; i < 8; ++i) {
        float x = -1.05f + i * 0.30f;
        Vec3 c = (i % 2 == 0) ? dark : yellow;
        add_box(m, {x, 1.46f, 0.02f}, {0.28f, 0.20f, 0.20f}, c, 0.0f);
        add_box(m, {x, 2.58f, 0.02f}, {0.28f, 0.20f, 0.20f}, c, 0.0f);
    }
    // Red hazard diamond center badge
    add_box(m, {0.0f, 2.02f, 0.11f}, {1.40f, 0.56f, 0.04f}, red, 0.0f);
    add_box(m, {0.0f, 2.02f, 0.13f}, {1.12f, 0.14f, 0.04f}, {0.98f, 0.98f, 0.95f}, 2.0f);
    return m;
}

MeshData MeshBuilder::build_barrier_full() {
    MeshData m;
    // Heavy concrete Jersey barrier + Douala traffic cones (switch lanes!)
    Vec3 concrete(0.64f, 0.63f, 0.60f);
    Vec3 cone_orange(0.98f, 0.38f, 0.06f);
    Vec3 white(0.96f, 0.96f, 0.94f);
    Vec3 yellow(0.96f, 0.78f, 0.10f);

    // Tapered Jersey concrete wall
    add_box(m, {0.0f, 0.30f, 0.0f}, {2.24f, 0.60f, 0.86f}, concrete, 0.0f);
    add_sloped_box(m, {0.0f, 1.05f, 0.0f}, {2.22f, 0.92f, 0.64f}, 0.0f, 0.96f, 0.45f, concrete * 1.04f, 0.0f);
    // Yellow/black top hazard cap
    for (int i = 0; i < 6; ++i) {
        float x = -0.92f + i * 0.37f;
        add_box(m, {x, 1.54f, 0.0f}, {0.35f, 0.16f, 0.30f},
                (i % 2 == 0) ? yellow : Vec3(0.14f, 0.14f, 0.16f), 0.0f);
    }
    // Two glowing traffic cones in front (+Z)
    for (float sx : {-0.72f, 0.72f}) {
        add_box(m, {sx, 0.04f, 0.60f}, {0.38f, 0.08f, 0.38f}, {0.16f, 0.16f, 0.18f}, 0.0f);
        add_cylinder(m, {sx, 0.08f, 0.60f}, 0.16f, 0.03f, 0.56f, 8, cone_orange, 0.0f);
        add_cylinder(m, {sx, 0.30f, 0.60f}, 0.115f, 0.085f, 0.14f, 8, white, 2.0f);
    }
    return m;
}

MeshData MeshBuilder::build_pothole_trench() {
    MeshData m;
    // Excavated Douala drainage/roadwork trench with muddy water & safety cones (jump over!)
    Vec3 asphalt_rim(0.28f, 0.26f, 0.25f);
    Vec3 laterite(0.54f, 0.27f, 0.14f);
    Vec3 puddle(0.16f, 0.28f, 0.34f);
    Vec3 cone(0.98f, 0.40f, 0.06f);

    add_box(m, {0.0f, 0.04f, 0.0f}, {2.24f, 0.08f, 2.60f}, laterite, 0.0f);
    add_box(m, {0.0f, 0.06f, 0.0f}, {1.88f, 0.08f, 2.20f}, puddle, 1.0f);
    add_box(m, {-1.08f, 0.09f, 0.0f}, {0.18f, 0.14f, 2.64f}, asphalt_rim, 0.0f);
    add_box(m, {1.08f, 0.09f, 0.0f}, {0.18f, 0.14f, 2.64f}, asphalt_rim, 0.0f);
    // Corner warning cones
    for (float sx : {-1.02f, 1.02f}) {
        for (float sz : {-1.20f, 1.20f}) {
            add_cylinder(m, {sx, 0.08f, sz}, 0.13f, 0.03f, 0.46f, 8, cone, 0.0f);
            add_cylinder(m, {sx, 0.26f, sz}, 0.09f, 0.07f, 0.10f, 8, {0.98f, 0.96f, 0.90f}, 2.0f);
        }
    }
    return m;
}

MeshData MeshBuilder::build_pushcart_plantain() {
    MeshData m;
    // Iconic Douala "Pousse-Pousse" wooden vendor handcart stacked with plantains & pineapples!
    Vec3 wood(0.56f, 0.36f, 0.18f);
    Vec3 dark_wood(0.38f, 0.24f, 0.12f);
    Vec3 wheel_col(0.16f, 0.16f, 0.18f);
    Vec3 plantain_green(0.34f, 0.68f, 0.18f);
    Vec3 plantain_yellow(0.95f, 0.80f, 0.14f);
    Vec3 tomato_red(0.88f, 0.20f, 0.16f);

    // Cart bed & side slats
    add_box(m, {0.0f, 0.56f, 0.0f}, {1.62f, 0.16f, 2.20f}, wood, 0.0f);
    add_box(m, {-0.78f, 0.84f, 0.0f}, {0.08f, 0.46f, 2.20f}, dark_wood, 0.0f);
    add_box(m, {0.78f, 0.84f, 0.0f}, {0.08f, 0.46f, 2.20f}, dark_wood, 0.0f);
    add_box(m, {0.0f, 0.84f, -1.06f}, {1.62f, 0.46f, 0.08f}, dark_wood, 0.0f);
    add_box(m, {0.0f, 0.84f, 1.06f}, {1.62f, 0.46f, 0.08f}, dark_wood, 0.0f);
    // Push handles extending toward +Z
    add_box(m, {-0.62f, 0.78f, 1.48f}, {0.08f, 0.08f, 0.84f}, wood, 0.0f);
    add_box(m, {0.62f, 0.78f, 1.48f}, {0.08f, 0.08f, 0.84f}, wood, 0.0f);

    // Mound of vibrant Cameroonian plantain bunches, ripe yellow plantains, and red fruit crates
    add_box(m, {-0.34f, 0.94f, -0.45f}, {0.72f, 0.42f, 0.96f}, plantain_green, 0.0f);
    add_box(m, {0.36f, 0.98f, -0.25f}, {0.68f, 0.48f, 0.96f}, plantain_yellow, 0.0f);
    add_box(m, {-0.18f, 0.92f, 0.48f}, {0.82f, 0.38f, 0.78f}, tomato_red, 0.0f);
    add_box(m, {0.26f, 1.24f, -0.20f}, {0.58f, 0.24f, 0.74f}, plantain_green * 1.1f, 0.0f);

    // Big spoked handcart wheels
    add_wheel(m, {-0.88f, 0.44f, 0.0f}, 0.44f, 0.12f, 12, wheel_col, wood);
    add_wheel(m, {0.88f, 0.44f, 0.0f}, 0.44f, 0.12f, 12, wheel_col, wood);
    return m;
}

MeshData MeshBuilder::build_char_torso() {
    MeshData m;
    // Sculpted athletic torso (tapered chest-to-waist + shoulder contour + collar + backpack!)
    add_sloped_box(m, {0.0f, 0.04f, 0.0f}, {0.42f, 0.56f, 0.25f}, 0.0f, 1.18f, 1.06f, {0.96f, 0.96f, 0.96f}, 0.0f);
    // Athletic waist/hip band
    add_box(m, {0.0f, -0.25f, 0.0f}, {0.41f, 0.14f, 0.24f}, {0.22f, 0.22f, 0.24f}, 0.0f);
    // Rounded shoulder caps for organic silhouette
    add_cylinder(m, {-0.26f, 0.16f, 0.0f}, 0.095f, 0.085f, 0.14f, 8, {0.94f, 0.94f, 0.94f}, 0.0f);
    add_cylinder(m, {0.26f, 0.16f, 0.0f}, 0.095f, 0.085f, 0.14f, 8, {0.94f, 0.94f, 0.94f}, 0.0f);
    // Athletic chevron band across chest & back
    add_box(m, {0.0f, 0.08f, 0.0f}, {0.49f, 0.09f, 0.28f}, {0.98f, 0.82f, 0.12f}, 0.0f);
    // Neck collar
    add_cylinder(m, {0.0f, 0.30f, 0.0f}, 0.10f, 0.085f, 0.11f, 8, {0.36f, 0.22f, 0.14f}, 0.0f);
    // Sleek urban runner backpack on +Z with shoulder straps
    add_sloped_box(m, {0.0f, 0.04f, 0.18f}, {0.34f, 0.42f, 0.15f}, -0.02f, 0.88f, 0.85f, {0.16f, 0.18f, 0.22f}, 0.0f);
    add_box(m, {0.0f, 0.12f, 0.26f}, {0.22f, 0.06f, 0.025f}, {0.98f, 0.78f, 0.12f}, 2.0f);
    return m;
}

MeshData MeshBuilder::build_char_head() {
    MeshData m;
    // Sculpted rounded stylized head (octagonal cylinder core + jaw contour + cap + headphones)
    add_cylinder(m, {0.0f, -0.14f, 0.0f}, 0.14f, 0.17f, 0.28f, 10, {0.96f, 0.96f, 0.96f}, 0.0f);
    add_cylinder(m, {0.0f, 0.12f, 0.0f}, 0.17f, 0.13f, 0.08f, 10, {0.14f, 0.14f, 0.16f}, 0.0f);
    // Athletic cap crown & forward brim (-Z)
    add_cylinder(m, {0.0f, 0.14f, 0.0f}, 0.185f, 0.155f, 0.10f, 10, {0.22f, 0.24f, 0.28f}, 0.0f);
    add_box(m, {0.0f, 0.13f, -0.20f}, {0.28f, 0.035f, 0.15f}, {0.95f, 0.76f, 0.10f}, 0.0f);
    // Headphones
    add_cylinder(m, {-0.18f, -0.03f, 0.0f}, 0.055f, 0.055f, 0.10f, 8, {0.90f, 0.18f, 0.16f}, 1.0f);
    add_cylinder(m, {0.18f, -0.03f, 0.0f}, 0.055f, 0.055f, 0.10f, 8, {0.90f, 0.18f, 0.16f}, 1.0f);
    return m;
}

MeshData MeshBuilder::build_char_limb() {
    MeshData m;
    // Sculpted tapered UPPER limb segment (thigh or upper arm), pivoted at y = 0 down to y = -0.36
    add_cylinder(m, {0.0f, -0.36f, 0.0f}, 0.070f, 0.092f, 0.36f, 8, {0.96f, 0.96f, 0.96f}, 0.0f);
    return m;
}

MeshData MeshBuilder::build_char_forelimb() {
    MeshData m;
    // Sculpted tapered LOWER limb segment (shin/calf or forearm), pivoted at knee/elbow y = 0 down to y = -0.35
    add_cylinder(m, {0.0f, -0.35f, 0.0f}, 0.055f, 0.072f, 0.35f, 8, {0.96f, 0.96f, 0.96f}, 0.0f);
    return m;
}

MeshData MeshBuilder::build_char_shoe() {
    MeshData m;
    // Sculpted athletic running sneaker (pivoted at ankle y = 0, toe pointing forward -Z!)
    add_box(m, {0.0f, -0.055f, -0.06f}, {0.145f, 0.09f, 0.28f}, {0.96f, 0.96f, 0.96f}, 1.0f);
    // Crisp white cushioned running midsole
    add_box(m, {0.0f, -0.105f, -0.06f}, {0.155f, 0.035f, 0.30f}, {0.98f, 0.98f, 0.98f}, 0.0f);
    // Angled toe cap
    add_sloped_box(m, {0.0f, -0.06f, -0.21f}, {0.135f, 0.06f, 0.09f}, 0.02f, 0.85f, 0.55f, {0.98f, 0.98f, 0.98f}, 1.0f);
    return m;
}

MeshData MeshBuilder::build_headlight_beam() {
    MeshData m;
    // Soft low-slung volumetric headlight pool projecting forward (+Z) ahead of oncoming vehicles!
    Vec3 warm(1.0f, 0.94f, 0.68f);
    add_quad(m,
             {-0.75f, 0.05f, 0.0f}, {0.75f, 0.05f, 0.0f},
             {1.75f, 0.04f, 6.2f}, {-1.75f, 0.04f, 6.2f},
             warm, 7.0f,
             {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f});
    return m;
}

MeshData MeshBuilder::build_hoverboard() {
    MeshData m;
    // Glowing Douala Lion Hoverboard!
    Vec3 board_green(0.06f, 0.68f, 0.34f);
    Vec3 gold(0.98f, 0.80f, 0.10f);
    Vec3 red(0.92f, 0.18f, 0.14f);
    Vec3 glow(0.20f, 1.0f, 0.65f);

    add_box(m, {0.0f, 0.08f, 0.0f}, {0.62f, 0.09f, 1.58f}, board_green, 1.0f);
    add_box(m, {0.0f, 0.13f, 0.0f}, {0.42f, 0.03f, 1.18f}, gold, 0.0f);
    add_box(m, {0.0f, 0.10f, -0.84f}, {0.46f, 0.07f, 0.22f}, red, 1.0f);
    add_box(m, {0.0f, 0.10f, 0.84f}, {0.46f, 0.07f, 0.22f}, red, 1.0f);
    // Glowing hover thrusters underneath
    add_box(m, {0.0f, 0.02f, -0.45f}, {0.50f, 0.05f, 0.30f}, glow, 2.0f);
    add_box(m, {0.0f, 0.02f, 0.45f}, {0.50f, 0.05f, 0.30f}, glow, 2.0f);
    return m;
}

MeshData MeshBuilder::build_jetpack() {
    MeshData m;
    Vec3 gold(0.96f, 0.72f, 0.14f);
    Vec3 metal(0.24f, 0.28f, 0.36f);
    Vec3 flame(1.0f, 0.56f, 0.12f);

    add_cylinder(m, {-0.16f, -0.26f, 0.0f}, 0.11f, 0.11f, 0.62f, 10, gold, 1.0f);
    add_cylinder(m, {0.16f, -0.26f, 0.0f}, 0.11f, 0.11f, 0.62f, 10, gold, 1.0f);
    add_box(m, {0.0f, 0.08f, 0.0f}, {0.36f, 0.26f, 0.14f}, metal, 1.0f);
    add_cylinder(m, {-0.16f, -0.42f, 0.0f}, 0.08f, 0.12f, 0.18f, 8, flame, 2.0f);
    add_cylinder(m, {0.16f, -0.42f, 0.0f}, 0.08f, 0.12f, 0.18f, 8, flame, 2.0f);
    return m;
}

MeshData MeshBuilder::build_coin() {
    MeshData m;
    // Gleaming rich metallic 3D FCFA Gold Coin with embossed inner medallion!
    Vec3 rim_gold(0.92f, 0.62f, 0.04f);
    Vec3 face_gold(1.0f, 0.78f, 0.08f);
    Vec3 inner_gold(0.84f, 0.54f, 0.02f);
    int segs = 14;
    float r = 0.38f;
    float ri = 0.24f;
    float ht = 0.068f;
    for (int i = 0; i < segs; ++i) {
        float a0 = (static_cast<float>(i) / segs) * TWO_PI;
        float a1 = (static_cast<float>(i + 1) / segs) * TWO_PI;
        float x0 = r * std::cos(a0), y0 = r * std::sin(a0);
        float x1 = r * std::cos(a1), y1 = r * std::sin(a1);
        float ix0 = ri * std::cos(a0), iy0 = ri * std::sin(a0);
        float ix1 = ri * std::cos(a1), iy1 = ri * std::sin(a1);
        // Chamfered thick rim
        add_quad(m, {x0, y0, -ht}, {x0, y0, ht}, {x1, y1, ht}, {x1, y1, -ht}, rim_gold, 1.0f);
        // Outer metallic gold ring face
        add_quad(m, {ix0, iy0, ht}, {x0, y0, ht}, {x1, y1, ht}, {ix1, iy1, ht}, face_gold, 1.0f);
        add_quad(m, {x0, y0, -ht}, {ix0, iy0, -ht}, {ix1, iy1, -ht}, {x1, y1, -ht}, face_gold, 1.0f);
        // Embossed inner FCFA medallion (gives dramatic coin contrast & sparkle!)
        add_tri(m, {0.0f, 0.0f, ht + 0.015f}, {ix0, iy0, ht + 0.01f}, {ix1, iy1, ht + 0.01f}, inner_gold, 1.0f);
        add_tri(m, {0.0f, 0.0f, -ht - 0.015f}, {ix1, iy1, -ht - 0.01f}, {ix0, iy0, -ht - 0.01f}, inner_gold, 1.0f);
    }
    return m;
}

MeshData MeshBuilder::build_pu_magnet() {
    MeshData m;
    Vec3 red(0.95f, 0.16f, 0.18f);
    Vec3 silver(0.90f, 0.94f, 0.98f);
    add_box(m, {-0.28f, 0.08f, 0.0f}, {0.18f, 0.62f, 0.18f}, red, 2.0f);
    add_box(m, {0.28f, 0.08f, 0.0f}, {0.18f, 0.62f, 0.18f}, red, 2.0f);
    add_box(m, {0.0f, -0.24f, 0.0f}, {0.74f, 0.18f, 0.18f}, red, 2.0f);
    add_box(m, {-0.28f, 0.36f, 0.0f}, {0.20f, 0.16f, 0.20f}, silver, 2.0f);
    add_box(m, {0.28f, 0.36f, 0.0f}, {0.20f, 0.16f, 0.20f}, silver, 2.0f);
    return m;
}

MeshData MeshBuilder::build_pu_sneakers() {
    MeshData m;
    Vec3 lime(0.20f, 0.96f, 0.36f);
    Vec3 white(0.98f, 0.98f, 0.98f);
    Vec3 gold(1.0f, 0.82f, 0.12f);
    // Pair of turbo spring sneakers
    for (float sx : {-0.22f, 0.22f}) {
        add_box(m, {sx, -0.08f, 0.0f}, {0.24f, 0.24f, 0.56f}, lime, 2.0f);
        add_box(m, {sx, -0.22f, 0.0f}, {0.26f, 0.09f, 0.60f}, white, 2.0f);
        add_box(m, {sx, 0.08f, -0.10f}, {0.22f, 0.20f, 0.28f}, gold, 2.0f);
    }
    return m;
}

MeshData MeshBuilder::build_pu_jetpack() {
    MeshData m = build_jetpack();
    // Scale slightly larger & brighter for easy pickup readability
    for (auto& v : m.vertices) {
        v.px *= 1.45f;
        v.py *= 1.45f;
        v.pz *= 1.45f;
        v.mat = 2.0f;
    }
    return m;
}

MeshData MeshBuilder::build_pu_multiplier() {
    MeshData m;
    // Glowing golden 2x Star Crystal
    Vec3 gold(1.0f, 0.84f, 0.10f);
    Vec3 cyan(0.20f, 0.88f, 1.0f);
    add_box(m, {0.0f, 0.0f, 0.0f}, {0.58f, 0.58f, 0.58f}, gold, 2.0f);
    add_box(m, {0.0f, 0.0f, 0.0f}, {0.82f, 0.26f, 0.26f}, cyan, 2.0f);
    add_box(m, {0.0f, 0.0f, 0.0f}, {0.26f, 0.82f, 0.26f}, cyan, 2.0f);
    return m;
}

MeshData MeshBuilder::build_pu_hoverboard() {
    MeshData m = build_hoverboard();
    for (auto& v : m.vertices) {
        v.px *= 0.72f;
        v.py = (v.py - 0.08f) * 0.72f;
        v.pz *= 0.72f;
        v.mat = 2.0f;
    }
    return m;
}

MeshData MeshBuilder::build_building_modern() {
    MeshData m;
    // Multi-story Douala commercial building (Akwa / Bonanjo style with balconies & rooftop water tank!)
    // Base footprint: 7.5 x 14.0 x 8.5, anchored at y=0
    add_box(m, {0.0f, 6.5f, 0.0f}, {7.4f, 13.0f, 8.2f}, {0.94f, 0.92f, 0.88f}, 4.0f, 2.0f, 4.0f);
    // Ground-floor shopfront arcade overhang
    add_box(m, {0.0f, 2.65f, 0.0f}, {8.1f, 0.35f, 8.8f}, {0.26f, 0.34f, 0.44f}, 0.0f);
    // Horizontal concrete balcony slabs
    for (float by : {5.4f, 8.2f, 11.0f}) {
        add_box(m, {0.0f, by, 0.0f}, {7.85f, 0.28f, 8.55f}, {0.84f, 0.82f, 0.78f}, 0.0f);
    }
    // Roof parapet & iconic blue cylindrical water tank + satellite dish!
    add_box(m, {0.0f, 13.25f, 0.0f}, {7.6f, 0.55f, 8.4f}, {0.72f, 0.70f, 0.68f}, 0.0f);
    add_cylinder(m, {1.6f, 13.5f, -1.2f}, 0.85f, 0.85f, 1.75f, 10, {0.14f, 0.42f, 0.76f}, 1.0f);
    add_box(m, {-1.8f, 14.0f, 1.4f}, {1.6f, 1.1f, 1.6f}, {0.62f, 0.62f, 0.64f}, 0.0f);
    return m;
}

MeshData MeshBuilder::build_building_shop() {
    MeshData m;
    // Vibrant Douala tropical 2-story shopfront with corrugated zinc roof & awning!
    add_box(m, {0.0f, 3.2f, 0.0f}, {6.8f, 6.4f, 7.6f}, {0.95f, 0.92f, 0.86f}, 4.0f, 2.0f, 2.0f);
    // Corrugated zinc pitched overhang roof
    add_sloped_box(m, {0.0f, 6.85f, 0.0f}, {7.6f, 0.95f, 8.2f}, 0.0f, 0.75f, 0.88f, {0.52f, 0.56f, 0.60f}, 1.0f);
    // Colorful striped storefront awning & neon shop sign bar
    add_box(m, {0.0f, 2.75f, 0.0f}, {7.8f, 0.24f, 7.9f}, {0.92f, 0.36f, 0.16f}, 0.0f);
    add_box(m, {0.0f, 3.20f, 0.0f}, {7.3f, 0.55f, 7.8f}, {0.10f, 0.56f, 0.34f}, 0.0f);
    return m;
}

MeshData MeshBuilder::build_market_umbrella() {
    MeshData m;
    // Iconic Marché Mboppi vibrant striped market parasol & fruit stall!
    Vec3 pole(0.42f, 0.32f, 0.24f);
    Vec3 table(0.58f, 0.38f, 0.20f);
    Vec3 c1(0.95f, 0.22f, 0.16f);
    Vec3 c2(0.98f, 0.82f, 0.12f);
    Vec3 c3(0.08f, 0.62f, 0.32f);

    // Wooden market table & colorful produce crates
    add_box(m, {0.0f, 0.45f, 0.0f}, {2.4f, 0.90f, 2.4f}, table, 0.0f);
    add_box(m, {-0.55f, 1.04f, -0.45f}, {0.85f, 0.32f, 0.85f}, c3, 0.0f);
    add_box(m, {0.55f, 1.06f, 0.35f}, {0.85f, 0.36f, 0.85f}, c2, 0.0f);
    add_box(m, {-0.40f, 1.02f, 0.55f}, {0.75f, 0.28f, 0.75f}, c1, 0.0f);

    // Center umbrella pole
    add_cylinder(m, {0.0f, 0.0f, 0.0f}, 0.08f, 0.07f, 3.25f, 6, pole, 0.0f);

    // Octagonal colorful striped canopy cone
    int segs = 8;
    float R = 2.05f;
    Vec3 apex(0.0f, 3.55f, 0.0f);
    for (int i = 0; i < segs; ++i) {
        float a0 = (static_cast<float>(i) / segs) * TWO_PI;
        float a1 = (static_cast<float>(i + 1) / segs) * TWO_PI;
        Vec3 p0(R * std::cos(a0), 2.75f, R * std::sin(a0));
        Vec3 p1(R * std::cos(a1), 2.75f, R * std::sin(a1));
        Vec3 col = (i % 2 == 0) ? c1 : c2;
        add_tri(m, apex, p1, p0, col, 0.0f);
        // Underside for visibility from below
        add_tri(m, apex - Vec3(0, 0.04f, 0), p0 - Vec3(0, 0.04f, 0), p1 - Vec3(0, 0.04f, 0), col * 0.78f, 0.0f);
    }
    return m;
}

MeshData MeshBuilder::build_palm_tree() {
    MeshData m;
    // Lush Douala tropical coconut/oil palm tree!
    Vec3 trunk1(0.46f, 0.34f, 0.22f);
    Vec3 trunk2(0.38f, 0.28f, 0.18f);
    Vec3 frond_bright(0.14f, 0.58f, 0.20f);
    Vec3 frond_dark(0.09f, 0.44f, 0.14f);
    Vec3 coconut(0.32f, 0.20f, 0.10f);

    // Gently curving segmented trunk up to y = 7.4m
    float cx = 0.0f, cy = 0.0f;
    for (int i = 0; i < 6; ++i) {
        float h = 1.25f;
        float rb = lerpf(0.34f, 0.18f, i / 6.0f);
        float rt = lerpf(0.34f, 0.18f, (i + 1) / 6.0f);
        add_cylinder(m, {cx, cy, 0.0f}, rb, rt, h + 0.05f, 7, (i % 2 == 0) ? trunk1 : trunk2, 0.0f);
        cx += 0.12f * (i * 0.25f);
        cy += h;
    }

    Vec3 crown(cx, cy - 0.1f, 0.0f);
    // Cluster of coconuts under the crown
    for (int i = 0; i < 4; ++i) {
        float a = i * (PI * 0.5f);
        add_box(m, crown + Vec3(0.24f * std::cos(a), -0.22f, 0.24f * std::sin(a)),
                {0.26f, 0.28f, 0.26f}, coconut, 0.0f);
    }

    // 8 gracefully arching palm fronds
    for (int i = 0; i < 8; ++i) {
        float a = (static_cast<float>(i) / 8.0f) * TWO_PI + 0.2f;
        Vec3 dir(std::cos(a), 0.0f, std::sin(a));
        Vec3 side(-std::sin(a), 0.0f, std::cos(a));
        Vec3 col = (i % 2 == 0) ? frond_bright : frond_dark;

        // Inner rising frond segment
        Vec3 p0 = crown;
        Vec3 p1 = crown + dir * 1.45f + Vec3(0.0f, 0.72f, 0.0f);
        Vec3 p2 = crown + dir * 3.15f + Vec3(0.0f, -0.35f, 0.0f);

        add_quad(m, p0 - side * 0.22f, p0 + side * 0.22f, p1 + side * 0.52f, p1 - side * 0.52f, col, 0.0f);
        add_quad(m, p1 - side * 0.52f, p1 + side * 0.52f, p2 + side * 0.14f, p2 - side * 0.14f, col * 0.92f, 0.0f);
        // Underside so fronds look thick from low runner camera
        add_quad(m, p0 + side * 0.22f, p0 - side * 0.22f, p1 - side * 0.52f, p1 + side * 0.52f, col * 0.75f, 0.0f);
        add_quad(m, p1 + side * 0.52f, p1 - side * 0.52f, p2 - side * 0.14f, p2 + side * 0.14f, col * 0.70f, 0.0f);
    }
    return m;
}

MeshData MeshBuilder::build_streetlamp() {
    MeshData m;
    Vec3 pole(0.32f, 0.35f, 0.38f);
    Vec3 lamp(1.0f, 0.90f, 0.58f);
    add_cylinder(m, {0.0f, 0.0f, 0.0f}, 0.14f, 0.09f, 6.8f, 8, pole, 1.0f);
    // Horizontal arm extending toward +X (rotate yaw=PI on right sidewalk)
    add_box(m, {1.05f, 6.72f, 0.0f}, {2.2f, 0.11f, 0.11f}, pole, 1.0f);
    add_box(m, {1.95f, 6.66f, 0.0f}, {0.68f, 0.16f, 0.34f}, pole, 1.0f);
    // Emissive warm bulb diffuser underneath
    add_box(m, {1.95f, 6.54f, 0.0f}, {0.58f, 0.09f, 0.28f}, lamp, 2.0f);
    return m;
}

MeshData MeshBuilder::build_billboard() {
    MeshData m;
    // Tall Douala roadside advertising / city welcome billboard (faces +Z!)
    Vec3 steel(0.28f, 0.30f, 0.34f);
    Vec3 frame(0.16f, 0.18f, 0.22f);

    add_cylinder(m, {-1.6f, 0.0f, 0.0f}, 0.18f, 0.15f, 4.6f, 8, steel, 1.0f);
    add_cylinder(m, {1.6f, 0.0f, 0.0f}, 0.18f, 0.15f, 4.6f, 8, steel, 1.0f);
    add_box(m, {0.0f, 6.2f, -0.08f}, {5.6f, 3.0f, 0.26f}, frame, 0.0f);
    // Front display face (material 5.0 -> procedural Douala billboard graphic texture!)
    add_quad(m,
             {-2.65f, 4.85f, 0.08f}, {2.65f, 4.85f, 0.08f},
             {2.65f, 7.55f, 0.08f}, {-2.65f, 7.55f, 0.08f},
             {1.0f, 1.0f, 1.0f}, 5.0f,
             {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f});
    // Top floodlights
    add_box(m, {-1.4f, 7.82f, 0.35f}, {0.42f, 0.12f, 0.22f}, {1.0f, 0.95f, 0.78f}, 2.0f);
    add_box(m, {1.4f, 7.82f, 0.35f}, {0.42f, 0.12f, 0.22f}, {1.0f, 0.95f, 0.78f}, 2.0f);
    return m;
}

MeshData MeshBuilder::build_monument_liberte() {
    MeshData m;
    // Iconic "Monument de la Nouvelle Liberté" at Rond-Point Deido, Douala!
    // Constructed from industrial metallic geometric forms rising proudly ~16m tall!
    Vec3 base_stone(0.68f, 0.66f, 0.62f);
    Vec3 steel_dark(0.32f, 0.34f, 0.38f);
    Vec3 steel_rust(0.64f, 0.36f, 0.22f);
    Vec3 steel_silver(0.76f, 0.78f, 0.82f);
    Vec3 gold_glow(1.0f, 0.78f, 0.18f);

    // Stepped roundabout pedestal
    add_cylinder(m, {0.0f, 0.0f, 0.0f}, 4.8f, 4.5f, 1.2f, 16, base_stone, 0.0f);
    add_cylinder(m, {0.0f, 1.2f, 0.0f}, 3.6f, 3.2f, 1.4f, 16, base_stone * 0.92f, 0.0f);

    // Striding industrial metallic legs (resembling Joseph-Francis Sumégné's iconic silhouette)
    add_box(m, {-1.15f, 5.2f, 0.35f}, {1.15f, 5.6f, 1.25f}, steel_dark, 1.0f);
    add_box(m, {1.15f, 5.2f, -0.35f}, {1.15f, 5.6f, 1.25f}, steel_rust, 1.0f);
    // Geometric scrap-metal rings & gears around torso
    add_box(m, {0.0f, 9.6f, 0.0f}, {3.2f, 4.2f, 2.2f}, steel_silver, 1.0f);
    add_cylinder(m, {0.0f, 8.2f, 0.0f}, 2.2f, 1.8f, 1.2f, 10, steel_rust, 1.0f);
    // Outstretched monumental arms raising the glowing orb/torch of Nouvelle Liberté!
    add_box(m, {-2.4f, 11.4f, 0.2f}, {2.4f, 1.0f, 1.0f}, steel_dark, 1.0f);
    add_box(m, {2.6f, 12.6f, 0.2f}, {1.1f, 3.6f, 1.1f}, steel_silver, 1.0f);
    // Sculptural head & radiant golden freedom beacon
    add_cylinder(m, {0.0f, 11.8f, 0.0f}, 1.05f, 0.85f, 1.9f, 10, steel_dark, 1.0f);
    add_box(m, {2.6f, 14.9f, 0.2f}, {1.35f, 1.35f, 1.35f}, gold_glow, 2.0f);
    return m;
}

MeshData MeshBuilder::build_port_crane() {
    MeshData m;
    // Port Autonome de Douala towering red-and-white container gantry crane
    Vec3 red(0.86f, 0.18f, 0.14f);
    Vec3 white(0.94f, 0.94f, 0.92f);
    Vec3 dark(0.22f, 0.24f, 0.28f);

    // 4 tall legs
    for (float sx : {-3.0f, 3.0f}) {
        for (float sz : {-2.4f, 2.4f}) {
            add_box(m, {sx, 9.5f, sz}, {0.75f, 19.0f, 0.75f}, red, 1.0f);
        }
    }
    // Cross beams
    add_box(m, {0.0f, 11.0f, 0.0f}, {6.6f, 0.9f, 5.4f}, white, 0.0f);
    add_box(m, {0.0f, 18.8f, 0.0f}, {7.0f, 1.1f, 5.6f}, red, 1.0f);
    // Long horizontal container boom extending over the Wouri
    add_box(m, {4.5f, 15.6f, 0.0f}, {22.0f, 1.25f, 2.4f}, white, 1.0f);
    add_box(m, {12.5f, 15.6f, 0.0f}, {5.0f, 1.30f, 2.45f}, red, 1.0f);
    // Operator cabin & warning aircraft beacon on top
    add_box(m, {3.5f, 14.4f, 0.0f}, {2.2f, 1.4f, 2.0f}, dark, 1.0f);
    add_box(m, {0.0f, 19.8f, 0.0f}, {0.65f, 0.65f, 0.65f}, {1.0f, 0.20f, 0.12f}, 2.0f);
    return m;
}

MeshData MeshBuilder::build_finish_arch() {
    MeshData m;
    // Grand Douala District Finish Line Archway in Cameroon Green-Red-Gold!
    Vec3 green(0.05f, 0.56f, 0.28f);
    Vec3 red(0.86f, 0.14f, 0.16f);
    Vec3 gold(0.98f, 0.82f, 0.10f);
    Vec3 white(0.97f, 0.97f, 0.95f);
    Vec3 black(0.12f, 0.12f, 0.14f);

    // Tall left & right gantry towers straddling all 3 lanes
    add_box(m, {-5.6f, 3.8f, 0.0f}, {0.95f, 7.6f, 0.95f}, green, 1.0f);
    add_box(m, {5.6f, 3.8f, 0.0f}, {0.95f, 7.6f, 0.95f}, gold, 1.0f);
    // Glowing star pillars on top
    add_box(m, {-5.6f, 8.0f, 0.0f}, {1.15f, 0.80f, 1.15f}, gold, 2.0f);
    add_box(m, {5.6f, 8.0f, 0.0f}, {1.15f, 0.80f, 1.15f}, gold, 2.0f);

    // Overhead banner gantry (Cameroon flag tricolor + checkered racing strip)
    add_box(m, {-3.7f, 6.4f, 0.0f}, {3.9f, 1.65f, 0.55f}, green, 0.0f);
    add_box(m, {0.0f, 6.4f, 0.0f}, {3.5f, 1.65f, 0.55f}, red, 0.0f);
    add_box(m, {3.7f, 6.4f, 0.0f}, {3.9f, 1.65f, 0.55f}, gold, 0.0f);
    // Glowing star in center of red band
    add_box(m, {0.0f, 6.4f, 0.30f}, {0.82f, 0.82f, 0.08f}, gold, 2.0f);

    // Checkered lower trim bar
    int checks = 16;
    float cw = 11.2f / checks;
    for (int i = 0; i < checks; ++i) {
        float x = -5.6f + (i + 0.5f) * cw;
        add_box(m, {x, 5.38f, 0.0f}, {cw, 0.42f, 0.62f}, (i % 2 == 0) ? white : black, 0.0f);
    }
    return m;
}

MeshData MeshBuilder::build_particle_cube() {
    MeshData m;
    add_box(m, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, 2.0f);
    return m;
}

MeshData MeshBuilder::build_rain_streak() {
    MeshData m;
    add_box(m, {0.0f, 0.0f, 0.0f}, {0.012f, 0.92f, 0.012f}, {0.58f, 0.76f, 0.96f}, 2.0f);
    return m;
}

MeshData MeshBuilder::build_shadow_blob() {
    MeshData m;
    // Soft grounded contact shadow disc beneath runner/coins
    int segs = 12;
    float r = 0.55f;
    Vec3 dark(0.04f, 0.04f, 0.06f);
    for (int i = 0; i < segs; ++i) {
        float a0 = (static_cast<float>(i) / segs) * TWO_PI;
        float a1 = (static_cast<float>(i + 1) / segs) * TWO_PI;
        add_tri(m, {0.0f, 0.02f, 0.0f},
                {r * std::cos(a1), 0.02f, r * std::sin(a1)},
                {r * std::cos(a0), 0.02f, r * std::sin(a0)},
                dark, 0.0f);
    }
    return m;
}

std::array<MeshData, MESH_COUNT> MeshBuilder::build_all_meshes() {
    std::array<MeshData, MESH_COUNT> out;
    out[MESH_ROAD_BOULEVARD] = build_road_boulevard();
    out[MESH_BRIDGE_WOURI] = build_bridge_wouri();
    out[MESH_WATER_PLANE] = build_water_plane();

    out[MESH_TAXI_YELLOW] = build_taxi_yellow();
    out[MESH_BENSKIN] = build_benskin();
    out[MESH_COASTER_BUS] = build_coaster_bus();
    out[MESH_CARGO_TRUCK] = build_cargo_truck();
    out[MESH_RAMP_WOOD] = build_ramp_wood();
    out[MESH_BARRIER_LOW] = build_barrier_low();
    out[MESH_BARRIER_HIGH] = build_barrier_high();
    out[MESH_BARRIER_FULL] = build_barrier_full();
    out[MESH_POTHOLE_TRENCH] = build_pothole_trench();
    out[MESH_PUSHCART_PLANTAIN] = build_pushcart_plantain();

    out[MESH_CHAR_TORSO] = build_char_torso();
    out[MESH_CHAR_HEAD] = build_char_head();
    out[MESH_CHAR_LIMB] = build_char_limb();
    out[MESH_CHAR_FORELIMB] = build_char_forelimb();
    out[MESH_CHAR_SHOE] = build_char_shoe();
    out[MESH_HOVERBOARD] = build_hoverboard();
    out[MESH_JETPACK] = build_jetpack();
    out[MESH_HEADLIGHT_BEAM] = build_headlight_beam();

    out[MESH_COIN] = build_coin();
    out[MESH_PU_MAGNET] = build_pu_magnet();
    out[MESH_PU_SNEAKERS] = build_pu_sneakers();
    out[MESH_PU_JETPACK] = build_pu_jetpack();
    out[MESH_PU_MULTIPLIER] = build_pu_multiplier();
    out[MESH_PU_HOVERBOARD] = build_pu_hoverboard();

    out[MESH_BUILDING_MODERN] = build_building_modern();
    out[MESH_BUILDING_SHOP] = build_building_shop();
    out[MESH_MARKET_UMBRELLA] = build_market_umbrella();
    out[MESH_PALM_TREE] = build_palm_tree();
    out[MESH_STREETLAMP] = build_streetlamp();
    out[MESH_BILLBOARD] = build_billboard();
    out[MESH_MONUMENT_LIBERTE] = build_monument_liberte();
    out[MESH_PORT_CRANE] = build_port_crane();
    out[MESH_FINISH_ARCH] = build_finish_arch();

    out[MESH_PARTICLE_CUBE] = build_particle_cube();
    out[MESH_RAIN_STREAK] = build_rain_streak();
    out[MESH_SHADOW_BLOB] = build_shadow_blob();
    return out;
}

}  // namespace maximus
