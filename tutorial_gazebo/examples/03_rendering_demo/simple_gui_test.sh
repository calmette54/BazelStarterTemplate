#!/bin/bash
set -e

echo "==========================================="
echo "Simple Gazebo GUI Test"
echo "==========================================="
echo "This script launches Gazebo GUI directly to test if it works."
echo "If you see a 3D window with a red sphere, the GUI is working!"
echo "Press Ctrl+C to stop."
echo ""

# Find the world file
WORLD_FILE="worlds/simple_transport_demo.sdf"

if [[ ! -f "$WORLD_FILE" ]]; then
    echo "ERROR: World file not found: $WORLD_FILE"
    echo "Make sure you're running from the correct directory."
    exit 1
fi

echo "Launching gz sim with world file: $WORLD_FILE"
echo "Looking for GUI window..."
echo ""

# Launch gz sim directly (this should show GUI)
exec gz sim "$WORLD_FILE" --verbose 1
