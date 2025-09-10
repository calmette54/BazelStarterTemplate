#pragma once

#include <string>

namespace lidar3d {

class GazeboLauncher {
public:
    GazeboLauncher();
    ~GazeboLauncher();

    // Launch Gazebo with a world file
    bool Launch(const std::string& world_file);
    
    // Get current world name (detected from running simulation)
    std::string GetWorldName() const;
    
    bool IsRunning() const;
    void Shutdown();

    // Wait for simulation to be ready
    bool WaitForReady(int timeout_seconds = 30);

private:
    bool running_ = false;
    std::string current_world_name_;
    
    // Detect world name from transport topics
    std::string DetectWorldName() const;
};

} // namespace lidar3d