#pragma once

#include <string>

namespace lidar3d {

class GazeboLauncher {
public:
    GazeboLauncher();
    ~GazeboLauncher();

    // Launch Gazebo GUI with specified world file
    bool Launch(const std::string& world_file);
    bool IsRunning() const;
    void Shutdown();

    // Wait for simulation to be ready
    bool WaitForReady(int timeout_seconds = 30);

private:
    bool running_ = false;
};

} // namespace lidar3d
