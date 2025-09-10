#!/bin/bash

echo "=========================="
echo "LiDAR 3D Warehouse Test"
echo "=========================="
echo ""

echo "Step 1: Launch warehouse environment..."
echo "Note: This will take a moment to download and load the world"
echo ""

# Start the warehouse environment in background
bazelisk run //demo:lidar_3d_demo -- --env warehouse &
DEMO_PID=$!

# Wait for Gazebo to start
sleep 10

echo ""
echo "Step 2: Verify topics after GUI plugin setup..."
echo "After manually adding KeyPublisher and VisualizeLidar plugins:"
echo ""

# Function to check topics
check_topics() {
    echo "Checking available topics..."
    gz topic -l | grep -E "(keyboard|lidar|scan)" || echo "No matching topics found"
    echo ""
    echo "Total topics: $(gz topic -l | wc -l)"
}

# Initial check (should show no keyboard topic)
echo "Before plugin setup:"
check_topics

echo "MANUAL STEPS REQUIRED:"
echo "1. In the Gazebo GUI, right-click and select 'Add plugins'"
echo "2. Add 'KeyPublisher' plugin"
echo "3. Configure KeyPublisher topic to '/keyboard/keypress'"
echo "4. Add 'VisualizeLidar' plugin"
echo "5. Press PLAY button to start simulation"
echo ""
echo "Press Enter after completing the manual steps..."
read -p ""

echo "After plugin setup:"
check_topics

# Test keyboard topic specifically
echo "Testing keyboard topic creation..."
if gz topic -l | grep -q "/keyboard/keypress"; then
    echo "✅ SUCCESS: /keyboard/keypress topic found!"
    echo "✅ Keyboard controls should now work (WASD, QE, RF, arrows)"
else
    echo "❌ FAIL: /keyboard/keypress topic not found"
    echo "❌ Please verify KeyPublisher plugin was added correctly"
fi

# Test LiDAR topic
echo ""
echo "Testing LiDAR sensor topic..."
LIDAR_TOPIC=$(gz topic -l | grep "lidar_platform.*scan" | head -1)
if [ -n "$LIDAR_TOPIC" ]; then
    echo "✅ SUCCESS: LiDAR topic found: $LIDAR_TOPIC"
    echo "✅ Point cloud visualization should work"
else
    echo "❌ FAIL: LiDAR sensor topic not found"
fi

echo ""
echo "Test complete. Press Ctrl+C in the demo terminal to exit."

# Wait for user to finish
wait $DEMO_PID