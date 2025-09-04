#!/bin/bash

#
# Gazebo Tutorial Example 01: Basic World Launch Script
#
# This script demonstrates how to launch Gazebo simulations and provides
# various options for running the basic world examples.
#

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Gazebo Tutorial: Basic World Demo ===${NC}"
echo ""

# Function to print usage
print_usage() {
    echo "Usage: $0 [demo|viewer|validator|help]"
    echo ""
    echo "Options:"
    echo "  demo      - Run the basic world simulation (default)"
    echo "  viewer    - Launch interactive world viewer"
    echo "  validator - Validate SDF files"
    echo "  help      - Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                    # Run basic demo"
    echo "  $0 demo              # Same as above"
    echo "  $0 viewer            # Interactive viewer"
    echo "  $0 validator         # Validate SDF files"
}

# Function to check if a file exists
check_file() {
    if [ ! -f "$1" ]; then
        echo -e "${RED}Error: Required file not found: $1${NC}"
        echo "Make sure you've built the project with Bazel:"
        echo "  bazel build //examples/01_basic_world:all"
        exit 1
    fi
}

# Function to run the basic world demo
run_demo() {
    echo -e "${GREEN}Starting Basic World Demo...${NC}"
    echo ""
    echo "This demo will:"
    echo "1. Load the basic world SDF file"
    echo "2. Run simulation for 10 seconds"
    echo "3. Display world entities and statistics"
    echo "4. Show simulation timing information"
    echo ""
    
    DEMO_BINARY="./basic_world_demo"
    WORLD_FILE="../../worlds/basic_world.sdf"
    
    check_file "$DEMO_BINARY"
    
    if [ -f "$WORLD_FILE" ]; then
        echo -e "${BLUE}Using world file: $WORLD_FILE${NC}"
        $DEMO_BINARY "$WORLD_FILE"
    else
        echo -e "${YELLOW}World file not found, using default${NC}"
        $DEMO_BINARY
    fi
    
    echo ""
    echo -e "${GREEN}Demo completed successfully!${NC}"
}

# Function to run the interactive viewer
run_viewer() {
    echo -e "${GREEN}Starting Interactive World Viewer...${NC}"
    echo ""
    echo "Controls:"
    echo "- Left mouse + drag: Rotate camera"
    echo "- Right mouse + drag: Pan camera"
    echo "- Mouse wheel: Zoom in/out"
    echo "- Esc: Close application"
    echo ""
    
    VIEWER_BINARY="./world_viewer"
    WORLD_FILE="../../worlds/basic_world.sdf"
    
    check_file "$VIEWER_BINARY"
    
    if [ -f "$WORLD_FILE" ]; then
        echo -e "${BLUE}Loading world: $WORLD_FILE${NC}"
        $VIEWER_BINARY "$WORLD_FILE"
    else
        echo -e "${YELLOW}World file not found, using default${NC}"
        $VIEWER_BINARY
    fi
}

# Function to run SDF validation
run_validator() {
    echo -e "${GREEN}Starting SDF Validator...${NC}"
    echo ""
    
    VALIDATOR_BINARY="./sdf_validator"
    
    check_file "$VALIDATOR_BINARY"
    
    # Find SDF files to validate
    SDF_FILES=""
    
    # Check for world files
    if [ -d "../../worlds" ]; then
        WORLD_FILES=$(find ../../worlds -name "*.sdf" 2>/dev/null || true)
        if [ ! -z "$WORLD_FILES" ]; then
            SDF_FILES="$SDF_FILES $WORLD_FILES"
        fi
    fi
    
    # Check for model files
    if [ -d "../../models" ]; then
        MODEL_FILES=$(find ../../models -name "*.sdf" 2>/dev/null || true)
        if [ ! -z "$MODEL_FILES" ]; then
            SDF_FILES="$SDF_FILES $MODEL_FILES"
        fi
    fi
    
    if [ -z "$SDF_FILES" ]; then
        echo -e "${YELLOW}No SDF files found to validate${NC}"
        echo "Creating a test SDF file for validation..."
        
        # Create a simple test SDF
        TEST_SDF="/tmp/test_world.sdf"
        cat > "$TEST_SDF" << 'EOF'
<?xml version="1.0" ?>
<sdf version="1.10">
    <world name="test_world">
        <physics name="1ms" type="ignored">
            <max_step_size>0.001</max_step_size>
            <real_time_factor>1.0</real_time_factor>
        </physics>
        
        <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics">
        </plugin>
        <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands">
        </plugin>
        <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster">
        </plugin>
        
        <light type="directional" name="sun">
            <cast_shadows>true</cast_shadows>
            <pose>0 0 10 0 0 0</pose>
            <diffuse>0.8 0.8 0.8 1</diffuse>
            <specular>0.2 0.2 0.2 1</specular>
        </light>
        
        <model name="ground_plane">
            <static>true</static>
            <link name="link">
                <collision name="collision">
                    <geometry>
                        <plane>
                            <normal>0 0 1</normal>
                        </plane>
                    </geometry>
                </collision>
                <visual name="visual">
                    <geometry>
                        <plane>
                            <normal>0 0 1</normal>
                            <size>100 100</size>
                        </plane>
                    </geometry>
                </visual>
            </link>
        </model>
    </world>
</sdf>
EOF
        
        SDF_FILES="$TEST_SDF"
        echo -e "${BLUE}Created test SDF file: $TEST_SDF${NC}"
    fi
    
    echo -e "${BLUE}Validating SDF files...${NC}"
    $VALIDATOR_BINARY $SDF_FILES
    
    # Clean up test file if we created it
    if [ -f "/tmp/test_world.sdf" ]; then
        rm -f "/tmp/test_world.sdf"
    fi
}

# Function to show environment info
show_environment() {
    echo -e "${BLUE}Environment Information:${NC}"
    echo "Working directory: $(pwd)"
    echo "Bazel workspace: $(bazel info workspace 2>/dev/null || echo 'Not in Bazel workspace')"
    echo ""
    
    # Check for required binaries
    echo -e "${BLUE}Available Executables:${NC}"
    for binary in basic_world_demo world_viewer sdf_validator; do
        if [ -f "./$binary" ]; then
            echo -e "  ✓ $binary"
        else
            echo -e "  ✗ $binary (not found)"
        fi
    done
    echo ""
}

# Main script logic
main() {
    # Show environment info
    show_environment
    
    # Parse command line argument
    case "${1:-demo}" in
        "demo")
            run_demo
            ;;
        "viewer")
            run_viewer
            ;;
        "validator")
            run_validator
            ;;
        "help"|"-h"|"--help")
            print_usage
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo ""
            print_usage
            exit 1
            ;;
    esac
}

# Check if running as source or executed
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi

#
# Key Concepts Demonstrated:
#
# 1. **Launch Scripts**: Creating reusable scripts to run Gazebo simulations
#    with different configurations and options.
#
# 2. **Environment Validation**: Checking for required files and binaries
#    before attempting to run simulations.
#
# 3. **Multiple Execution Modes**: Providing different ways to interact with
#    the same simulation (headless, interactive, validation).
#
# 4. **Error Handling**: Robust error checking and user-friendly messages
#    for common issues and missing dependencies.
#
# 5. **Documentation**: Inline help and usage information for users.
#
# Usage Examples:
#   ./run_basic_world.sh              # Run basic demo
#   ./run_basic_world.sh viewer       # Interactive mode
#   ./run_basic_world.sh validator    # Validate SDF files
#   ./run_basic_world.sh help         # Show help
#
# Integration with Bazel:
# This script assumes it's run from the bazel-bin directory after building:
#   bazel build //examples/01_basic_world:all
#   bazel-bin/examples/01_basic_world/run_basic_world.sh
#