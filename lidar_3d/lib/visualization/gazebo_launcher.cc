#include "gazebo_launcher.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <vector>

namespace lidar3d {

void setupLibraryPaths() {
    const char* current_path = getenv("LD_LIBRARY_PATH");
    std::string existing_paths = current_path ? current_path : "";
    std::string system_paths = "/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu";
    std::string full_path = system_paths + ":" + existing_paths;
    setenv("LD_LIBRARY_PATH", full_path.c_str(), 1);
}

GazeboLauncher::GazeboLauncher() = default;

GazeboLauncher::~GazeboLauncher() {
    Shutdown();
}

bool GazeboLauncher::Launch(const std::string& world_file) {
    if (IsRunning()) {
        std::cerr << "Gazebo is already running" << std::endl;
        return false;
    }

    // Set up library paths to fix GUI plugin loading issues
    setupLibraryPaths();

    std::cout << "Launching Gazebo with world: " << world_file << std::endl;

    // Build command string
    std::string command = "gz sim " + world_file + " --verbose 1 &";
    std::cout << "Executing: " << command << std::endl;

    // Use system() to launch Gazebo in background
    int result = system(command.c_str());

    if (result != 0) {
        std::cerr << "Failed to launch Gazebo via system(): " << result << std::endl;
        return false;
    }

    // Give Gazebo time to start
    std::this_thread::sleep_for(std::chrono::seconds(3));
    running_ = true;

    return true;
}

bool GazeboLauncher::IsRunning() const {
    return running_;
}

void GazeboLauncher::Shutdown() {
    if (running_) {
        std::cout << "Shutting down Gazebo..." << std::endl;

        // Use pkill to terminate gz sim processes
        system("pkill -f 'gz sim'");

        // Wait a bit for cleanup
        std::this_thread::sleep_for(std::chrono::seconds(2));
        running_ = false;
    }
}

bool GazeboLauncher::WaitForReady(int timeout_seconds) {
    if (!running_) {
        return false;
    }

    std::cout << "Waiting for Gazebo to be ready..." << std::endl;

    // Create a transport node to check for topics
    auto node = std::make_shared<gz::transport::Node>();

    auto start_time = std::chrono::steady_clock::now();
    auto timeout_duration = std::chrono::seconds(timeout_seconds);

    while (std::chrono::steady_clock::now() - start_time < timeout_duration) {
        // Check if essential topics are available
        std::vector<std::string> topics;
        node->TopicList(topics);

        bool has_clock = false;
        bool has_scene = false;

        for (const auto& topic : topics) {
            if (topic.find("/clock") != std::string::npos) {
                has_clock = true;
            }
            if (topic.find("/scene") != std::string::npos) {
                has_scene = true;
            }
        }

        if (has_clock && has_scene) {
            std::cout << "Gazebo is ready!" << std::endl;
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cerr << "Timeout waiting for Gazebo to be ready" << std::endl;
    return false;
}

} // namespace lidar3d
