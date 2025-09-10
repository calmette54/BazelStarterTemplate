# LiDAR 3D Multi-Environment Support

This document describes the multi-environment system for the LiDAR 3D demo.

## Available Environments

### Minimal Environment (default)
- **Name**: Minimal Test Environment  
- **Description**: Simple test world for basic LiDAR testing - ultra-stable
- **Features**: 
  - Built-in LiDAR platform with collision geometry
  - Keyboard controls work automatically
  - GUI plugins load automatically
  - Perfect for development and testing

### Warehouse Environment
- **Name**: Tugbot Depot Warehouse
- **Description**: MovAI Tugbot depot - realistic warehouse with charging stations
- **Features**:
  - Real-world warehouse environment from Gazebo Fuel
  - LiDAR platform injected at runtime
  - Dynamic world name detection
  - Requires manual GUI plugin setup

## Usage

### Basic Usage
```bash
# Run with minimal environment (default)
bazelisk run //demo:lidar_3d_demo

# Run with warehouse environment
bazelisk run //demo:lidar_3d_demo -- --env warehouse

# List available environments
bazelisk run //demo:lidar_3d_demo -- --list-environments

# Show help
bazelisk run //demo:lidar_3d_demo -- --help
```

### Warehouse Environment Setup

Due to Gazebo Fuel world limitations, the warehouse environment requires manual GUI plugin setup:

1. **Launch the warehouse**:
   ```bash
   bazelisk run //demo:lidar_3d_demo -- --env warehouse
   ```

2. **Add required plugins manually**:
   - Right-click in the GUI → "Add plugins"
   - Add "KeyPublisher" plugin
   - Add "VisualizeLidar" plugin
   - Configure KeyPublisher topic to `/keyboard/keypress`

3. **Start simulation**:
   - Press the PLAY button ▶️ in the GUI
   - Use keyboard controls: WASD, QE, RF, arrow keys

## Keyboard Controls

- **W/S**: Forward/Backward (orientation-aware)
- **A/D**: Turn Left/Right (yaw rotation) 
- **Q/E**: Strafe Left/Right (orientation-aware)
- **R/F**: Move Up/Down (world space)
- **↑/↓**: Pitch Up/Down
- **←/→**: Yaw Left/Right (same as A/D)
- **Ctrl+C**: Exit

## Technical Details

### Architecture
- **GazeboLauncher**: Enhanced with environment support
- **EnvironmentType**: MINIMAL, WAREHOUSE enums
- **Dynamic Configuration**: World names and topics adapt automatically
- **Runtime Model Injection**: LiDAR platform added to external worlds

### Key Components
- `lib/visualization/gazebo_launcher.h/cc`: Environment management
- `demo/lidar_3d_demo.cc`: CLI interface and environment selection
- `config/lidar_gui_config.xml`: GUI configuration for external worlds
- `models/lidar_platform.sdf`: Standalone model for runtime injection

### Debugging Features
- Comprehensive logging for troubleshooting
- Topic detection and validation
- World name detection from transport topics
- GUI configuration path validation

## Known Limitations

1. **Fuel World GUI Config**: External Fuel worlds ignore `--gui-config` parameter
2. **Manual Plugin Setup**: Warehouse environment requires manual GUI plugin loading
3. **Internet Required**: Warehouse environment downloads from Gazebo Fuel (first run only)

## Future Improvements

- Automatic plugin injection via transport services
- Additional environment types (urban, indoor, etc.)
- Saved GUI configurations for different environments
- Pre-built local copies of popular Fuel worlds