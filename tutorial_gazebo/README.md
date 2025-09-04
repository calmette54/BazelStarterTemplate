# Gazebo Harmonic LTS Comprehensive Tutorial

A complete, hermetic Bazel-based tutorial covering all major Gazebo functionalities using only Bazel Central Registry (BCR) modules.

## 🎯 Overview

This tutorial provides an in-depth, step-by-step guide to mastering Gazebo Harmonic LTS (Long Term Support) - the latest generation of the Gazebo robotics simulator. Each example builds upon previous concepts while introducing new features and capabilities.

### What You'll Learn

- **Gazebo Architecture**: Understanding the modular design and plugin system
- **SDF Worlds**: Creating simulation environments with physics, lighting, and models  
- **Physics Engines**: Working with DART, Bullet, and TPE physics engines
- **Rendering Pipeline**: Ogre2 rendering, materials, PBR, and visual effects
- **Sensor Simulation**: Cameras, LiDAR, IMU, GPS, and custom sensors
- **Communication**: Inter-process messaging with gz-transport
- **Plugin Development**: Creating custom system, visual, and sensor plugins
- **Robot Simulation**: Complete multi-DOF robots with sensors and control
- **Advanced Features**: Actors, nested models, and performance optimization

## 🏗️ Project Structure

```
tutorial_gazebo/
├── MODULE.bazel                    # Bazel module with Gazebo dependencies
├── BUILD.bazel                     # Root build configuration
├── README.md                       # This comprehensive guide
├── examples/                       # Progressive tutorial examples
│   ├── 01_basic_world/            # SDF worlds, models, lighting
│   ├── 02_physics_demo/           # Physics engines and simulations
│   ├── 03_rendering_demo/         # Materials, PBR, visual effects
│   ├── 04_sensors_demo/           # Sensor simulation and data
│   ├── 05_transport_demo/         # Communication and messaging
│   ├── 06_plugins_demo/           # Custom plugin development
│   ├── 07_robot_simulation/       # Complete robot systems
│   └── 08_advanced_features/      # Advanced techniques
├── worlds/                         # SDF world files
├── models/                         # Robot and object models
├── plugins/                        # Custom C++ plugins
└── launch/                         # Launch configurations
```

## 🚀 Quick Start

### Prerequisites

- **Bazel 7.0+** with bzlmod support
- **C++ Compiler** with C++17 support
- **Linux/macOS/Windows** (cross-platform)
- **Git** for cloning the repository

### Build and Run

```bash
# Clone the repository (if not already done)
git clone <repository_url>
cd tutorial_gazebo

# Build all examples
bazel build //...

# Run the first example
bazel run //examples/01_basic_world:basic_world_demo

# Launch interactive viewer
bazel run //examples/01_basic_world:world_viewer

# Validate SDF files
bazel run //examples/01_basic_world:sdf_validator
```

## 📚 Tutorial Examples

### 🌍 Example 01: Basic World Creation

**Learning Objectives**: Understanding fundamental Gazebo concepts

**Key Topics**:
- SDF world format and structure
- Essential system plugins (Physics, UserCommands, SceneBroadcaster)
- Basic model creation and positioning
- World state inspection and entity management

**What You'll Build**:
- Basic world with primitive shapes
- Interactive world viewer
- SDF validation tool
- Launch automation scripts

```bash
# Run the basic world demo
bazel run //examples/01_basic_world:basic_world_demo

# Interactive GUI viewer
bazel run //examples/01_basic_world:world_viewer

# Validate SDF files
bazel run //examples/01_basic_world:sdf_validator
```

**Files**:
- `basic_world_demo.cc`: Core simulation with entity inspection
- `world_viewer.cc`: Interactive GUI application
- `sdf_validator.cc`: SDF file validation tool
- `run_basic_world.sh`: Launch script with options
- `worlds/basic_world.sdf`: Demonstration world file

### ⚡ Example 02: Physics Simulation

**Learning Objectives**: Understanding physics engines and simulation

**Key Topics**:
- Multiple physics engines (DART, Bullet, TPE)
- Collision detection and response
- Joints and constraints
- Material properties and friction
- Performance optimization

**What You'll Build**:
- Physics engine comparison tool
- Collision demonstration world
- Joint and constraint examples
- Performance benchmarking

```bash
# Physics engine comparison
bazel run //examples/02_physics_demo:physics_comparison

# Collision demo
bazel run //examples/02_physics_demo:collision_demo

# Joint constraints
bazel run //examples/02_physics_demo:joint_demo
```

### 🎨 Example 03: Rendering and Materials

**Learning Objectives**: Advanced visualization and material systems

**Key Topics**:
- Ogre2 rendering engine configuration
- Physically Based Rendering (PBR)
- Lighting systems and shadows
- Material properties and textures
- Camera controls and viewpoints

**What You'll Build**:
- Material showcase demonstration
- Lighting comparison tool
- PBR parameter explorer
- Multi-camera viewer

```bash
# Material showcase
bazel run //examples/03_rendering_demo:material_showcase

# Lighting demo
bazel run //examples/03_rendering_demo:lighting_demo

# Camera controls
bazel run //examples/03_rendering_demo:camera_demo
```

### 📡 Example 04: Sensor Simulation

**Learning Objectives**: Comprehensive sensor integration and data processing

**Key Topics**:
- Camera sensors (RGB, depth, stereo)
- LiDAR and point cloud processing
- IMU and navigation sensors
- GPS and positioning systems
- Sensor data publishing and visualization

**What You'll Build**:
- Multi-sensor platform
- Sensor data recorder and player
- Point cloud processor
- Sensor calibration tools

```bash
# Multi-sensor demo
bazel run //examples/04_sensors_demo:sensor_platform

# LiDAR visualization
bazel run //examples/04_sensors_demo:lidar_demo

# Camera array
bazel run //examples/04_sensors_demo:camera_array
```

### 📨 Example 05: Communication and Transport

**Learning Objectives**: Inter-process communication and distributed systems

**Key Topics**:
- gz-transport topics and services
- Message types and serialization
- Distributed simulation architectures
- Performance monitoring and debugging
- Custom message definitions

**What You'll Build**:
- Message publisher/subscriber demo
- Service request/response system
- Multi-node communication
- Performance monitoring tools

```bash
# Basic messaging
bazel run //examples/05_transport_demo:basic_messaging

# Service demo
bazel run //examples/05_transport_demo:service_demo

# Multi-node system
bazel run //examples/05_transport_demo:distributed_demo
```

### 🔌 Example 06: Plugin Development

**Learning Objectives**: Extending Gazebo with custom functionality

**Key Topics**:
- System plugin architecture
- Visual and GUI plugins
- Sensor plugins
- Plugin lifecycle management
- Configuration and parameters

**What You'll Build**:
- Custom system plugin
- Visual effect plugin
- Sensor plugin with custom data
- GUI control plugin

```bash
# System plugin demo
bazel run //examples/06_plugins_demo:system_plugin_demo

# Visual plugin
bazel run //examples/06_plugins_demo:visual_plugin_demo

# Custom sensor plugin
bazel run //examples/06_plugins_demo:sensor_plugin_demo
```

### 🤖 Example 07: Robot Simulation

**Learning Objectives**: Complete robotic system integration

**Key Topics**:
- Multi-link robot models
- Joint controllers and actuators
- Sensor integration on robots
- Control system architecture
- Robot-environment interaction

**What You'll Build**:
- Mobile robot with sensors
- Manipulator arm with control
- Complete robot system integration
- Autonomous behavior implementation

```bash
# Mobile robot
bazel run //examples/07_robot_simulation:mobile_robot_demo

# Manipulator arm
bazel run //examples/07_robot_simulation:manipulator_demo

# Complete system
bazel run //examples/07_robot_simulation:complete_robot_system
```

### 🎭 Example 08: Advanced Features

**Learning Objectives**: Advanced simulation techniques and optimization

**Key Topics**:
- Actor systems and animations
- Nested model hierarchies
- Performance profiling and optimization
- Advanced scripting and automation
- Custom world generation

**What You'll Build**:
- Animated actor system
- Nested model demonstration
- Performance analysis tools
- Procedural world generation

```bash
# Actor animation
bazel run //examples/08_advanced_features:actor_demo

# Nested models
bazel run //examples/08_advanced_features:nested_model_demo

# Performance tools
bazel run //examples/08_advanced_features:performance_demo
```

## 🛠️ Development Environment

### Bazel Configuration

This project uses **hermetic builds** with all dependencies managed through the Bazel Central Registry (BCR):

```starlark
# MODULE.bazel dependencies
bazel_dep(name = "gz-sim", version = "9.3.0")           # Main simulator
bazel_dep(name = "gz-sensors", version = "9.2.0.bcr.1") # Sensor simulation
bazel_dep(name = "gz-rendering", version = "9.2.0")     # Rendering engine
bazel_dep(name = "gz-physics", version = "8.3.0")       # Physics engines
bazel_dep(name = "gz-common", version = "6.1.0")        # Common utilities
```

**Auto-resolved dependencies**:
- `gz-math`: Mathematical utilities and data types
- `gz-msgs`: Message definitions for communication
- `gz-plugin`: Plugin loading and management
- `gz-transport`: Inter-process communication
- `gz-utils`: General utility libraries

### Build Commands

```bash
# Build everything
bazel build //...

# Build specific example
bazel build //examples/01_basic_world:all

# Run tests
bazel test //...

# Run specific example
bazel run //examples/01_basic_world:basic_world_demo

# Build with optimization
bazel build -c opt //...

# Build with debug info
bazel build -c dbg //...
```

### IDE Support

Generate compile_commands.json for IDE integration:

```bash
bazel run //:refresh_all
```

## 🎓 Learning Path

### Beginner (Examples 1-2)
Start here if you're new to Gazebo or robotics simulation:

1. **Basic World Creation**: Understand SDF format and basic concepts
2. **Physics Simulation**: Learn about physics engines and object interaction

### Intermediate (Examples 3-5)  
Continue with visualization and communication:

3. **Rendering and Materials**: Master visual appearance and lighting
4. **Sensor Simulation**: Integrate cameras, LiDAR, and other sensors
5. **Communication**: Implement inter-process messaging and services

### Advanced (Examples 6-8)
Tackle complex topics and customization:

6. **Plugin Development**: Create custom Gazebo extensions
7. **Robot Simulation**: Build complete robotic systems
8. **Advanced Features**: Performance optimization and complex scenarios

## 📖 Key Concepts Reference

### SDF (Simulation Description Format)

SDF is the XML-based format for describing simulation worlds:

```xml
<sdf version="1.10">
  <world name="my_world">
    <!-- World configuration -->
    <physics name="default">
      <max_step_size>0.001</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    
    <!-- Models, lights, etc. -->
  </world>
</sdf>
```

### Entity-Component-System (ECS)

Gazebo uses ECS architecture for flexible and performant simulation:

- **Entities**: Unique identifiers for simulation objects
- **Components**: Data containers (pose, mass, velocity, etc.)
- **Systems**: Logic processors that operate on entity-component data

### Plugin Types

Gazebo supports multiple plugin types:

1. **System Plugins**: Core simulation logic
2. **Visual Plugins**: Rendering and GUI extensions  
3. **Sensor Plugins**: Custom sensor implementations
4. **GUI Plugins**: User interface components
5. **World Plugins**: World-level functionality

### Physics Engines

Available physics engines with different characteristics:

- **DART**: Default, feature-rich, good for complex robots
- **Bullet**: Fast, good for large numbers of objects
- **TPE**: Simple, custom engine for basic simulations

## 🔍 Troubleshooting

### Common Issues

**Build Errors**:
```bash
# Clear Bazel cache
bazel clean --expunge

# Update MODULE.bazel.lock
bazel mod deps

# Check Bazel version
bazel version
```

**Runtime Issues**:
```bash
# Check library paths
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(bazel info output_base)

# Enable verbose logging
export GZ_VERBOSE=1

# Check gz command availability
gz --version
```

**SDF Validation**:
```bash
# Use the built-in validator
bazel run //examples/01_basic_world:sdf_validator -- worlds/*.sdf

# Check with gz tool (if available)
gz sdf -k worlds/basic_world.sdf
```

### Performance Tips

1. **Use static models** for immovable objects
2. **Simplify collision geometry** compared to visual geometry
3. **Limit shadow-casting lights** for better performance
4. **Use appropriate physics timesteps** (1ms default)
5. **Enable/disable plugins** based on needs

### Debugging

```bash
# Enable debug mode
bazel build -c dbg //examples/01_basic_world:basic_world_demo

# Run with GDB
gdb bazel-bin/examples/01_basic_world/basic_world_demo

# Use Gazebo's built-in debugging
export GZ_DEBUG=1
```

## 🤝 Contributing

### Code Structure

Follow these conventions when contributing:

```cpp
// File header with description
/*
 * Brief description of the file
 * 
 * Learning objectives and key concepts
 */

// Standard includes
#include <gz/sim/Server.hh>
#include <gz/common/Console.hh>

// Namespace usage
using namespace gz;
using namespace sim;

// Comprehensive comments explaining concepts
```

### Adding New Examples

1. Create directory in `examples/`
2. Add `BUILD.bazel` with targets
3. Implement `.cc` files with documentation
4. Create supporting SDF files in `worlds/` or `models/`
5. Update main `README.md`

### Testing

All examples should be tested:

```bash
# Test compilation
bazel build //examples/new_example:all

# Test execution
bazel run //examples/new_example:demo

# Test with different configurations
bazel run //examples/new_example:demo -- --config=test
```

## 📄 License

This tutorial is part of the BazelStarterTemplate project and follows the same licensing terms.

## 🔗 Resources

### Official Documentation
- [Gazebo Documentation](https://gazebosim.org/docs/)
- [Gazebo Harmonic](https://gazebosim.org/docs/harmonic/)
- [SDF Format Specification](http://sdformat.org/)
- [Bazel Documentation](https://bazel.build/)

### Community
- [Gazebo Community Forum](https://community.gazebosim.org/)
- [GitHub Issues](https://github.com/gazebosim/)
- [Gazebo Discord](https://discord.gg/gazebo)

### Tutorials and Examples
- [Official Gazebo Tutorials](https://gazebosim.org/docs/harmonic/tutorials)
- [Gazebo Classic Migration Guide](https://gazebosim.org/docs/harmonic/migrating_gazebo_classic)
- [ROS 2 + Gazebo Integration](https://gazebosim.org/docs/harmonic/ros2_integration)

---

**Happy Simulating! 🚀**

*This tutorial provides a comprehensive foundation for mastering Gazebo Harmonic LTS. Each example builds upon previous knowledge while introducing new concepts and practical applications. Follow the examples in order, experiment with the code, and don't hesitate to modify and extend the demonstrations to fit your specific needs.*