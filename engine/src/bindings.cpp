#include "game_engine.hpp"
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace maximus;

PYBIND11_MODULE(_maximus_engine, m) {
    m.doc() = "Maximus: Douala Runner 3D — Native C++17 physics, procedural 3D meshes, world generator, and instance batcher";

    m.attr("MESH_COUNT") = static_cast<int>(MESH_COUNT);
    m.attr("MESH_ROAD_BOULEVARD") = static_cast<int>(MESH_ROAD_BOULEVARD);
    m.attr("MESH_BRIDGE_WOURI") = static_cast<int>(MESH_BRIDGE_WOURI);
    m.attr("MESH_WATER_PLANE") = static_cast<int>(MESH_WATER_PLANE);
    m.attr("MESH_TAXI_YELLOW") = static_cast<int>(MESH_TAXI_YELLOW);
    m.attr("MESH_BENSKIN") = static_cast<int>(MESH_BENSKIN);
    m.attr("MESH_COASTER_BUS") = static_cast<int>(MESH_COASTER_BUS);
    m.attr("MESH_CARGO_TRUCK") = static_cast<int>(MESH_CARGO_TRUCK);
    m.attr("MESH_RAMP_WOOD") = static_cast<int>(MESH_RAMP_WOOD);
    m.attr("MESH_BARRIER_LOW") = static_cast<int>(MESH_BARRIER_LOW);
    m.attr("MESH_BARRIER_HIGH") = static_cast<int>(MESH_BARRIER_HIGH);
    m.attr("MESH_BARRIER_FULL") = static_cast<int>(MESH_BARRIER_FULL);
    m.attr("MESH_POTHOLE_TRENCH") = static_cast<int>(MESH_POTHOLE_TRENCH);
    m.attr("MESH_PUSHCART_PLANTAIN") = static_cast<int>(MESH_PUSHCART_PLANTAIN);
    m.attr("MESH_HEADLIGHT_BEAM") = static_cast<int>(MESH_HEADLIGHT_BEAM);
    m.attr("MESH_PARTICLE_CUBE") = static_cast<int>(MESH_PARTICLE_CUBE);
    m.attr("MESH_RAIN_STREAK") = static_cast<int>(MESH_RAIN_STREAK);
    m.attr("MESH_SHADOW_BLOB") = static_cast<int>(MESH_SHADOW_BLOB);

    m.attr("STATUS_DEMO") = static_cast<int>(STATUS_DEMO);
    m.attr("STATUS_PLAYING") = static_cast<int>(STATUS_PLAYING);
    m.attr("STATUS_PAUSED") = static_cast<int>(STATUS_PAUSED);
    m.attr("STATUS_GAME_OVER") = static_cast<int>(STATUS_GAME_OVER);
    m.attr("STATUS_LEVEL_COMPLETE") = static_cast<int>(STATUS_LEVEL_COMPLETE);

    m.attr("SFX_NONE") = static_cast<int>(SFX_NONE);
    m.attr("SFX_COIN") = static_cast<int>(SFX_COIN);
    m.attr("SFX_JUMP") = static_cast<int>(SFX_JUMP);
    m.attr("SFX_ROLL") = static_cast<int>(SFX_ROLL);
    m.attr("SFX_LANE_SWITCH") = static_cast<int>(SFX_LANE_SWITCH);
    m.attr("SFX_POWERUP") = static_cast<int>(SFX_POWERUP);
    m.attr("SFX_CRASH") = static_cast<int>(SFX_CRASH);
    m.attr("SFX_STUMBLE") = static_cast<int>(SFX_STUMBLE);
    m.attr("SFX_HOVERBOARD_ON") = static_cast<int>(SFX_HOVERBOARD_ON);
    m.attr("SFX_HOVERBOARD_BREAK") = static_cast<int>(SFX_HOVERBOARD_BREAK);
    m.attr("SFX_NEAR_MISS") = static_cast<int>(SFX_NEAR_MISS);
    m.attr("SFX_HORN") = static_cast<int>(SFX_HORN);
    m.attr("SFX_LEVEL_COMPLETE") = static_cast<int>(SFX_LEVEL_COMPLETE);
    m.attr("SFX_MILESTONE") = static_cast<int>(SFX_MILESTONE);

    py::class_<LevelParams>(m, "LevelParams")
        .def(py::init<>())
        .def_readwrite("level_id", &LevelParams::level_id)
        .def_readwrite("is_endless", &LevelParams::is_endless)
        .def_readwrite("is_bridge", &LevelParams::is_bridge)
        .def_readwrite("is_market", &LevelParams::is_market)
        .def_readwrite("has_monument", &LevelParams::has_monument)
        .def_readwrite("is_rainy", &LevelParams::is_rainy)
        .def_readwrite("is_night", &LevelParams::is_night)
        .def_readwrite("target_distance", &LevelParams::target_distance)
        .def_readwrite("base_speed", &LevelParams::base_speed)
        .def_readwrite("max_speed", &LevelParams::max_speed)
        .def_readwrite("speed_accel", &LevelParams::speed_accel)
        .def_readwrite("obstacle_density", &LevelParams::obstacle_density)
        .def_readwrite("moving_traffic_chance", &LevelParams::moving_traffic_chance)
        .def_readwrite("ramp_combo_chance", &LevelParams::ramp_combo_chance)
        .def_readwrite("powerup_chance", &LevelParams::powerup_chance)
        .def_readwrite("seed", &LevelParams::seed);

    py::class_<UpgradeParams>(m, "UpgradeParams")
        .def(py::init<>())
        .def_readwrite("magnet_duration", &UpgradeParams::magnet_duration)
        .def_readwrite("sneakers_duration", &UpgradeParams::sneakers_duration)
        .def_readwrite("jetpack_duration", &UpgradeParams::jetpack_duration)
        .def_readwrite("multiplier_duration", &UpgradeParams::multiplier_duration)
        .def_readwrite("starting_hoverboards", &UpgradeParams::starting_hoverboards)
        .def_readwrite("outfit_id", &UpgradeParams::outfit_id);

    py::class_<InputCommands>(m, "InputCommands")
        .def(py::init<>())
        .def_readwrite("left", &InputCommands::left)
        .def_readwrite("right", &InputCommands::right)
        .def_readwrite("jump", &InputCommands::jump)
        .def_readwrite("roll", &InputCommands::roll)
        .def_readwrite("hoverboard", &InputCommands::hoverboard);

    py::class_<HudSnapshot>(m, "HudSnapshot")
        .def_readonly("status", &HudSnapshot::status)
        .def_readonly("level_id", &HudSnapshot::level_id)
        .def_readonly("endless_zone", &HudSnapshot::endless_zone)
        .def_readonly("distance", &HudSnapshot::distance)
        .def_readonly("target_distance", &HudSnapshot::target_distance)
        .def_readonly("progress", &HudSnapshot::progress)
        .def_readonly("speed_mps", &HudSnapshot::speed_mps)
        .def_readonly("speed_kmh", &HudSnapshot::speed_kmh)
        .def_readonly("score", &HudSnapshot::score)
        .def_readonly("coins", &HudSnapshot::coins)
        .def_readonly("multiplier", &HudSnapshot::multiplier)
        .def_readonly("combo_streak", &HudSnapshot::combo_streak)
        .def_readonly("near_misses", &HudSnapshot::near_misses)
        .def_readonly("stumbles", &HudSnapshot::stumbles)
        .def_readonly("hoverboards_left", &HudSnapshot::hoverboards_left)
        .def_readonly("magnet_timer", &HudSnapshot::magnet_timer)
        .def_readonly("magnet_max", &HudSnapshot::magnet_max)
        .def_readonly("sneakers_timer", &HudSnapshot::sneakers_timer)
        .def_readonly("sneakers_max", &HudSnapshot::sneakers_max)
        .def_readonly("jetpack_timer", &HudSnapshot::jetpack_timer)
        .def_readonly("jetpack_max", &HudSnapshot::jetpack_max)
        .def_readonly("mult_timer", &HudSnapshot::mult_timer)
        .def_readonly("mult_max", &HudSnapshot::mult_max)
        .def_readonly("hoverboard_timer", &HudSnapshot::hoverboard_timer)
        .def_readonly("hoverboard_max", &HudSnapshot::hoverboard_max)
        .def_readonly("chaser_proximity", &HudSnapshot::chaser_proximity)
        .def_readonly("camera_shake", &HudSnapshot::camera_shake)
        .def_readonly("player_x", &HudSnapshot::player_x)
        .def_readonly("player_y", &HudSnapshot::player_y)
        .def_readonly("player_s", &HudSnapshot::player_s)
        .def_readonly("on_roof", &HudSnapshot::on_roof)
        .def_readonly("is_rolling", &HudSnapshot::is_rolling)
        .def_readonly("popup_text", &HudSnapshot::popup_text)
        .def_readonly("popup_timer", &HudSnapshot::popup_timer)
        .def_readonly("crash_reason", &HudSnapshot::crash_reason)
        .def_property_readonly("view_bytes", [](const HudSnapshot& s) {
            return py::bytes(reinterpret_cast<const char*>(s.view_mat), sizeof(s.view_mat));
        })
        .def_property_readonly("proj_bytes", [](const HudSnapshot& s) {
            return py::bytes(reinterpret_cast<const char*>(s.proj_mat), sizeof(s.proj_mat));
        })
        .def_property_readonly("cam_pos", [](const HudSnapshot& s) {
            return py::make_tuple(s.cam_pos[0], s.cam_pos[1], s.cam_pos[2]);
        })
        .def_property_readonly("sun_focus", [](const HudSnapshot& s) {
            return py::make_tuple(s.sun_focus[0], s.sun_focus[1], s.sun_focus[2]);
        });

    py::class_<GameEngine>(m, "GameEngine")
        .def(py::init<>())
        .def("start_level", &GameEngine::start_level,
             py::arg("params"), py::arg("upgrades"), py::arg("demo_mode") = false)
        .def("set_paused", &GameEngine::set_paused)
        .def("set_aspect", &GameEngine::set_aspect)
        .def("push_input", &GameEngine::push_input)
        .def("step", &GameEngine::step)
        .def("snapshot", &GameEngine::snapshot, py::return_value_policy::reference_internal)
        .def("consume_sound_events", &GameEngine::consume_sound_events)
        .def("obstacle_count", &GameEngine::obstacle_count)
        .def("collectible_count", &GameEngine::collectible_count)
        .def("particle_count", &GameEngine::particle_count)
        .def("npc_count", &GameEngine::npc_count)
        .def("total_instance_count", &GameEngine::total_instance_count)
        .def("get_mesh_buffers", [](const GameEngine& eng, int mesh_id) {
            if (mesh_id < 0 || mesh_id >= MESH_COUNT) {
                throw std::out_of_range("Invalid mesh_id");
            }
            const MeshData& md = eng.meshes()[mesh_id];
            py::bytes vb(reinterpret_cast<const char*>(md.vertices.data()),
                         md.vertices.size() * sizeof(Vertex));
            py::bytes ib(reinterpret_cast<const char*>(md.indices.data()),
                         md.indices.size() * sizeof(uint32_t));
            return py::make_tuple(vb, ib, md.vertices.size(), md.indices.size());
        })
        .def("get_instance_view", [](const GameEngine& eng, int mesh_id) -> py::tuple {
            if (mesh_id < 0 || mesh_id >= MESH_COUNT) {
                throw std::out_of_range("Invalid mesh_id");
            }
            const auto& vec = eng.mesh_instances(mesh_id);
            if (vec.empty()) {
                return py::make_tuple(0, py::none());
            }
            py::memoryview mv = py::memoryview::from_memory(
                static_cast<const void*>(vec.data()),
                static_cast<py::ssize_t>(vec.size() * sizeof(InstanceData))
            );
            return py::make_tuple(static_cast<int>(vec.size()), mv);
        });
}
