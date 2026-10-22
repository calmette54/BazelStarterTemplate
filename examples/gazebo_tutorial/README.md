# Example: Gazebo tutorial (gz-sim from the BCR)

Demo programs and SDF worlds for [Gazebo](https://gazebosim.org) Ionic
(gz-sim 9), with every Gazebo library — gz-sim, gz-physics, gz-rendering,
gz-sensors, sdformat, ogre-next — built from the BCR with the template's
toolchain.

> **Status: builds, not yet a working tutorial.** All targets build and the
> worlds are validated, but the demos that embed `gz::sim::Server` do not
> simulate yet: the worlds ask for system plugins by library name
> (`gz-sim-physics-system`), while the binaries link them statically
> (`static://gz::sim::systems::Physics`), and the physics engine plugin
> (`gz-physics-dartsim-plugin`) is not provided at all. Demos that start the
> host's `gz sim` (`03_rendering_demo:transport_demo`,
> `04_sensors_demo:lidar_demo`) need Gazebo Ionic installed.

## Run it

From this directory (the repository root ignores `examples/`):

```bash
bazelisk build //...          # long first build: gz-sim, ogre-next, assimp, ...
bazelisk test //...           # parses every world with sdformat
bazelisk run //examples/01_basic_world:world_viewer
```

## Contents

| Path | Contents |
|---|---|
| `examples/01_basic_world/` | `basic_world_demo`, `world_viewer` (headless server) |
| `examples/02_physics_demo/` | collisions, joints, physics engine comparison, benchmark, configurator |
| `examples/03_rendering_demo/` | cameras, lighting, materials, PBR, visual effects, a gz-transport material demo |
| `examples/04_sensors_demo/` | `lidar_demo` |
| `worlds/`, `models/` | SDF worlds and models; `//:worlds_test` loads every world with sdformat |
| `.bazelrc` | Compiles assimp as C++17 (it does not build as C++23) |

Chapters 05–08 (transport, plugins, robot simulation, advanced features)
and the IMU/GPS/fusion sensor demos from `amo/ros2-bazelified` are not
included: they never compiled (removed headers such as `gz/common/Time.hh`,
non-existent messages and methods).
