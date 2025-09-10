#!/bin/bash

echo "======================="
echo "LiDAR 3D Environment Test"
echo "======================="
echo ""

# Test selection
if [ "$1" == "warehouse" ]; then
    echo "Testing WAREHOUSE environment (tugbot_depot)..."
    echo "NOTE: This will download the world from Gazebo Fuel on first run"
    echo ""
    bazelisk run //demo:lidar_3d_demo -- --env warehouse
elif [ "$1" == "minimal" ]; then
    echo "Testing MINIMAL environment..."
    echo ""
    bazelisk run //demo:lidar_3d_demo -- --env minimal
else
    echo "Usage: $0 [minimal|warehouse]"
    echo ""
    echo "Examples:"
    echo "  $0 minimal     # Test minimal environment (default)"
    echo "  $0 warehouse   # Test tugbot_depot warehouse"
    echo ""
    echo "Available environments:"
    bazelisk run //demo:lidar_3d_demo -- --list-environments
fi