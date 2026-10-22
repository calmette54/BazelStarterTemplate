#pragma once

#include <sys/types.h>

#include <string>

namespace lidar3d {

// Starts `gz sim <world>` in the background and stops it again.
class GazeboLauncher {
public:
    // `gz_executable` is looked up in PATH; tests pass a stand-in.
    explicit GazeboLauncher(std::string gz_executable = "gz");
    ~GazeboLauncher();

    GazeboLauncher(const GazeboLauncher&) = delete;
    GazeboLauncher& operator=(const GazeboLauncher&) = delete;

    // Starts the simulation. Returns false if one is already running or the
    // executable cannot be started.
    bool Launch(const std::string& world_file);

    // Get current world name (detected from running simulation)
    std::string GetWorldName() const;

    // True while the started process is alive.
    bool IsRunning();

    // Stops the started simulation (and only that one).
    void Shutdown();

    // Wait for simulation to be ready
    bool WaitForReady(int timeout_seconds = 30);

private:
    std::string gz_executable_;
    pid_t pid_ = -1;
    mutable std::string current_world_name_;

    // Detect world name from transport topics
    std::string DetectWorldName() const;
};

}  // namespace lidar3d
