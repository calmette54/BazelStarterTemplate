#include "gazebo_launcher.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <vector>
#include <cstdlib>
#include <gz/transport/Node.hh>

namespace lidar3d {

GazeboLauncher::GazeboLauncher() {
}

GazeboLauncher::~GazeboLauncher() {
    Shutdown();
}

bool GazeboLauncher::Launch(const std::string& world_file) {
    if (IsRunning()) {
        std::cerr << "Gazebo is already running" << std::endl;
        return false;
    }

    // Set up library paths to fix GUI plugin loading issues
    const char* current_path = getenv("LD_LIBRARY_PATH");
    std::string existing_paths = current_path ? current_path : "";
    std::string system_paths = "/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu";
    std::string full_path = system_paths + ":" + existing_paths;
    setenv("LD_LIBRARY_PATH", full_path.c_str(), 1);

    std::cout << "Launching Gazebo with world: " << world_file << std::endl;

    // Launch Gazebo in background
    std::string command = "gz sim " + world_file + " --verbose 1 &";
    int result = system(command.c_str());

    if (result != 0) {
        std::cerr << "Failed to launch Gazebo" << std::endl;
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
        system("pkill -f 'gz sim'");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        running_ = false;
    }
}

bool GazeboLauncher::WaitForReady(int timeout_seconds) {
    if (!running_) {
        return false;
    }

    auto node = std::make_shared<gz::transport::Node>();
    auto start_time = std::chrono::steady_clock::now();
    auto timeout_duration = std::chrono::seconds(timeout_seconds);

    while (std::chrono::steady_clock::now() - start_time < timeout_duration) {
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
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cerr << "Timeout waiting for Gazebo to be ready" << std::endl;
    return false;
}

std::string GazeboLauncher::GetWorldName() const {
    if (current_world_name_.empty()) {
        const_cast<GazeboLauncher*>(this)->current_world_name_ = DetectWorldName();
    }
    return current_world_name_;
}

std::string GazeboLauncher::DetectWorldName() const {
    auto node = std::make_shared<gz::transport::Node>();
    
    std::vector<std::string> topics;
    node->TopicList(topics);
    
    // Look for world-specific topics (e.g., /world/world_name/clock)
    for (const auto& topic : topics) {
        if (topic.find("/world/") != std::string::npos && topic.find("/clock") != std::string::npos) {
            // Extract world name from topic: /world/world_name/clock
            size_t start = topic.find("/world/") + 7;  // Skip "/world/"
            size_t end = topic.find("/", start);
            if (end != std::string::npos) {
                std::string world_name = topic.substr(start, end - start);
                return world_name;
            }
        }
    }
    
    // Default fallback
    return "default";
}

} // namespace lidar3d