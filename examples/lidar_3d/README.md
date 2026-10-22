# Example: 3D LiDAR point clouds from Gazebo (gz-transport)

A small C++ library that subscribes to a simulated 3D LiDAR in
[Gazebo](https://gazebosim.org) over gz-transport, turns each
`gz::msgs::PointCloudPacked` into an Eigen N×3 matrix, and moves the sensor
platform from the keyboard.

The Gazebo libraries (`gz-transport`, `gz-msgs`, `gz-math`, `gz-common`) come
from the BCR and are built from source with the template's toolchain: the
library and its tests need nothing installed. Only the demo needs Gazebo
(`gz sim` from Gazebo Ionic, matching gz-transport 14 and gz-msgs 11) on
the host.

## Run it

From this directory (the repository root ignores `examples/`):

```bash
bazelisk test //...                         # unit tests, no Gazebo needed
bazelisk run //demo:lidar_3d_demo           # starts `gz sim` with demo/worlds/lidar_3d_world.sdf
bazelisk run //demo:lidar_3d_demo -- warehouse   # depot world (downloads models from Gazebo Fuel)
```

The demo prints statistics for each point cloud. With the Gazebo window
focused, move the platform: W/S forward/back, A/D turn, Q/E sideways, R/F up
and down, arrow keys up/down to pitch (keys arrive on `/keyboard/keypress`
from the worlds' KeyPublisher plugin).

The first build compiles the gz-* libraries and protobuf from source and
takes several minutes.

## Layout

| Path | Contents |
|---|---|
| `lib/core/` | `PointCloud`: parses `PointCloudPacked` (x, y, z fields) into an Eigen matrix |
| `lib/sensor/` | `Lidar3DSensor`: subscribes to a scan topic and calls back with a `PointCloud` |
| `lib/control/` | `MovementController`: moves a model with `/world/<name>/set_pose` on key presses |
| `lib/visualization/` | `GazeboLauncher`: starts `gz sim` in its own process group and stops only that one |
| `demo/` | The demo binary and its SDF worlds |
| `test/` | GoogleTest unit tests; the launcher test uses `fake_gz.sh` instead of Gazebo |

```cpp
#include "lib/sensor/lidar_3d_sensor.h"

lidar3d::Lidar3DSensor sensor;
sensor.Initialize(topic, [](const lidar3d::PointCloud& cloud) {
    const auto& points = cloud.GetPoints();  // N×3, row-major
});
```
