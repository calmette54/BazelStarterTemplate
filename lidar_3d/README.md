# 3D LiDAR Library

Production-ready 3D LiDAR processing library with Gazebo integration.

## Features

- **Eigen-based Point Cloud Processing**: Efficient N×3 matrix representation
- **Gazebo Integration**: Seamless 3D LiDAR sensor subscription
- **Keyboard Control**: 6DOF platform movement (WASD + QE/RF)
- **Real-time Visualization**: Point cloud display in Gazebo GUI
- **Modular Architecture**: Clean separation of concerns

## Quick Start

```bash
# Build and run the demo
bazelisk run //lidar_3d/demo:lidar_3d_demo

# Run tests
bazelisk test //lidar_3d/test:all_tests
```

## Usage

```cpp
#include "lidar_3d/lib/sensor/lidar_3d_sensor.h"

lidar3d::Lidar3DSensor sensor;
sensor.Initialize("/lidar");
sensor.SetCallback([](const lidar3d::PointCloud& cloud) {
    const auto& points = cloud.GetPoints();  // Eigen::MatrixXf (N×3)
    // Process point cloud...
});
sensor.Start();
```

## Architecture

- `lib/core/`: Point cloud data structures
- `lib/sensor/`: 3D LiDAR sensor interface
- `lib/control/`: Movement and keyboard control
- `lib/visualization/`: Gazebo launcher and GUI management
- `demo/`: Complete demonstration application
- `test/`: Unit tests for all components
