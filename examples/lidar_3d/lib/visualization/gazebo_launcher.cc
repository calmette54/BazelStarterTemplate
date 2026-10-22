#include "gazebo_launcher.h"

#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <gz/transport/Node.hh>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

extern char** environ;

namespace lidar3d {

GazeboLauncher::GazeboLauncher(std::string gz_executable) : gz_executable_(std::move(gz_executable)) {}

GazeboLauncher::~GazeboLauncher() { Shutdown(); }

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

    // No shell: the world path is passed as is. `gz sim` starts the server and
    // the GUI as child processes, so give it its own process group and stop
    // the whole group in Shutdown().
    std::vector<std::string> args = {gz_executable_, "sim", world_file, "--verbose", "1"};
    std::vector<char*> argv;
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    argv.push_back(nullptr);

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP);
    posix_spawnattr_setpgroup(&attr, 0);
    const int error = posix_spawnp(&pid_, gz_executable_.c_str(), nullptr, &attr, argv.data(), environ);
    posix_spawnattr_destroy(&attr);

    if (error != 0) {
        std::cerr << "Failed to launch Gazebo (" << gz_executable_ << "): " << std::strerror(error)
                  << std::endl;
        pid_ = -1;
        return false;
    }
    current_world_name_.clear();
    return true;
}

bool GazeboLauncher::IsRunning() {
    if (pid_ <= 0) {
        return false;
    }
    if (waitpid(pid_, nullptr, WNOHANG) == 0) {
        return true;
    }
    pid_ = -1;  // exited (or was never ours)
    return false;
}

void GazeboLauncher::Shutdown() {
    if (!IsRunning()) {
        return;
    }
    std::cout << "Shutting down Gazebo..." << std::endl;
    kill(-pid_, SIGTERM);
    for (int i = 0; i < 50 && waitpid(pid_, nullptr, WNOHANG) == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (waitpid(pid_, nullptr, WNOHANG) == 0) {
        kill(-pid_, SIGKILL);
        waitpid(pid_, nullptr, 0);
    }
    pid_ = -1;
}

bool GazeboLauncher::WaitForReady(int timeout_seconds) {
    if (!IsRunning()) {
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
        current_world_name_ = DetectWorldName();
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