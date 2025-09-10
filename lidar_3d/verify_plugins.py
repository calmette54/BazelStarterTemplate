#!/usr/bin/env python3
"""
Verify that LiDAR plugins are working correctly in the warehouse environment.
This script monitors Gazebo topics to detect when plugins are properly loaded.
"""

import subprocess
import time
import sys

def get_topics():
    """Get list of current Gazebo topics."""
    try:
        result = subprocess.run(['gz', 'topic', '-l'], 
                              capture_output=True, text=True, timeout=5)
        return result.stdout.strip().split('\n') if result.returncode == 0 else []
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return []

def check_keyboard_topic():
    """Check if keyboard topic exists."""
    topics = get_topics()
    return '/keyboard/keypress' in topics

def check_lidar_topic():
    """Check if LiDAR sensor topic exists."""
    topics = get_topics()
    lidar_topics = [t for t in topics if 'lidar_platform' in t and 'scan' in t]
    return lidar_topics

def monitor_topics():
    """Monitor topics and report status."""
    print("🔍 Monitoring Gazebo topics...")
    print("⏳ Waiting for plugins to be loaded manually...\n")
    
    keyboard_found = False
    lidar_found = False
    
    for i in range(60):  # Monitor for 60 seconds
        # Check keyboard topic
        if not keyboard_found and check_keyboard_topic():
            print("✅ KeyPublisher plugin detected! /keyboard/keypress topic found")
            keyboard_found = True
        
        # Check LiDAR topic
        lidar_topics = check_lidar_topic()
        if not lidar_found and lidar_topics:
            print(f"✅ LiDAR sensor detected! Topics: {lidar_topics}")
            lidar_found = True
        
        # If both found, we're done
        if keyboard_found and lidar_found:
            print("\n🎉 SUCCESS: All plugins detected!")
            print("🎮 You can now use keyboard controls (WASD, QE, RF, arrows)")
            print("👁️  Point cloud visualization should be working")
            return True
        
        # Show progress
        if i % 10 == 0 and i > 0:
            print(f"⏳ Still waiting... ({i}s elapsed)")
            topics = get_topics()
            print(f"   Total topics: {len(topics)}")
            if not keyboard_found:
                print("   ❌ KeyPublisher not found - please add it manually")
            if not lidar_found:
                print("   ❌ LiDAR topic not found - check if simulation is running")
        
        time.sleep(1)
    
    print("\n⚠️  Timeout reached. Plugin status:")
    print(f"   KeyPublisher: {'✅ Found' if keyboard_found else '❌ Not found'}")
    print(f"   LiDAR sensor: {'✅ Found' if lidar_found else '❌ Not found'}")
    return False

def main():
    print("LiDAR 3D Plugin Verification Tool")
    print("=" * 35)
    print()
    
    # Initial check
    print("📊 Initial topic scan...")
    topics = get_topics()
    if not topics:
        print("❌ Cannot connect to Gazebo. Is it running?")
        sys.exit(1)
    
    print(f"   Found {len(topics)} topics")
    
    keyboard_initial = check_keyboard_topic()
    lidar_initial = check_lidar_topic()
    
    print(f"   KeyPublisher: {'✅ Found' if keyboard_initial else '❌ Not found'}")
    print(f"   LiDAR sensor: {'✅ Found' if lidar_initial else '❌ Not found'}")
    print()
    
    if keyboard_initial and lidar_initial:
        print("🎉 All plugins already working!")
        return
    
    # Instructions for manual setup
    if not keyboard_initial:
        print("📝 MANUAL SETUP REQUIRED:")
        print("   1. In Gazebo GUI, right-click and select 'Add plugins'")
        print("   2. Find and add 'KeyPublisher' plugin")
        print("   3. Configure KeyPublisher topic to '/keyboard/keypress'")
        print("   4. Add 'VisualizeLidar' plugin")
        print("   5. Press PLAY button to start simulation")
        print()
    
    # Monitor for changes
    success = monitor_topics()
    
    if success:
        print("\n🚀 System is ready for testing!")
    else:
        print("\n🔧 Please check the manual setup steps and try again")

if __name__ == "__main__":
    main()