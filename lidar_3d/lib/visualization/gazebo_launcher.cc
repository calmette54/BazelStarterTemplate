#include "gazebo_launcher.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <gz/transport/Node.hh>
#include <gz/msgs/entity_factory.pb.h>
#include <gz/msgs/boolean.pb.h>

namespace lidar3d {

GazeboLauncher::GazeboLauncher() {
    InitializeEnvironments();
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

    // For external worlds (Fuel URLs), use GUI-first approach
    if (world_file.find("fuel.gazebosim.org") != std::string::npos ||
        world_file.find("http") != std::string::npos) {

        std::cout << "External world detected, using GUI-first launch approach..." << std::endl;

        // Try to find the GUI config file
        std::vector<std::string> gui_config_paths = {
            "config/lidar_gui_config.xml",
            "lidar_3d/config/lidar_gui_config.xml",
            "/home/user/Documents/BazelStarterTemplate/lidar_3d/config/lidar_gui_config.xml"
        };

        std::string gui_config_path;
        for (const auto& path : gui_config_paths) {
            std::cout << "Checking path: " << path << std::endl;
            std::ifstream test_file(path);
            if (test_file.good()) {
                gui_config_path = path;
                std::cout << "Found GUI config at: " << path << std::endl;
                break;
            }
        }

        if (!gui_config_path.empty()) {
            // Use GUI-first approach: launch with GUI config, then load world
            std::string command = "gz sim --gui-config " + gui_config_path + " " + world_file + " --verbose 1";
            std::cout << "Using GUI-first approach with config: " << gui_config_path << std::endl;
            std::cout << "Executing: " << command << std::endl;

            command += " &";

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
        } else {
            std::cerr << "WARNING: Could not find GUI configuration file!" << std::endl;
        }
    }

    // Build standard command for local worlds
    std::string command = "gz sim " + world_file + " --verbose 1";

    command += " &";
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

// Enhanced launch methods
bool GazeboLauncher::LaunchWithEnvironment(EnvironmentType env_type) {
    try {
        EnvironmentConfig config = GetEnvironmentConfig(env_type);
        return LaunchWithEnvironment(config);
    } catch (const std::exception& e) {
        std::cerr << "Failed to get environment config: " << e.what() << std::endl;
        return false;
    }
}

bool GazeboLauncher::LaunchWithEnvironment(const EnvironmentConfig& env_config) {
    if (IsRunning()) {
        std::cerr << "Gazebo is already running" << std::endl;
        return false;
    }

    current_environment_ = env_config;

    std::cout << "Launching environment: " << env_config.name << std::endl;
    std::cout << "Description: " << env_config.description << std::endl;

    std::string world_file_to_use;

    // Handle Fuel URLs vs local files
    if (!env_config.fuel_url.empty()) {
        std::cout << "Using Gazebo Fuel world: " << env_config.fuel_url << std::endl;
        world_file_to_use = env_config.fuel_url;
    } else {
        // Use local world file
        world_file_to_use = env_config.world_file_path;
    }

    // Launch the world first
    if (!Launch(world_file_to_use)) {
        return false;
    }

    // Wait for world to be ready
    if (!WaitForReady()) {
        std::cerr << "World failed to become ready" << std::endl;
        return false;
    }

    // Detect the world name for all environments
    current_world_name_ = DetectWorldName();

    // Only inject LiDAR platform for environments that need it (currently none, as all SDF files have it embedded)
    // For warehouse environment, LiDAR platform is already embedded in tugbot_depot.sdf
    // For minimal environment, LiDAR platform is already embedded in lidar_3d_world.sdf
    std::cout << "Using embedded LiDAR platform from SDF file" << std::endl;

    return true;
}

// Environment management methods
std::vector<EnvironmentConfig> GazeboLauncher::GetAvailableEnvironments() const {
    return available_environments_;
}

EnvironmentConfig GazeboLauncher::GetEnvironmentConfig(EnvironmentType env_type) const {
    auto it = std::find_if(available_environments_.begin(), available_environments_.end(),
                          [env_type](const EnvironmentConfig& config) {
                              return config.type == env_type;
                          });

    if (it != available_environments_.end()) {
        return *it;
    }

    throw std::runtime_error("Environment type not found");
}

EnvironmentType GazeboLauncher::GetEnvironmentTypeFromString(const std::string& env_name) const {
    std::string lower_name = env_name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    if (lower_name == "minimal") return EnvironmentType::MINIMAL;
    if (lower_name == "warehouse") return EnvironmentType::WAREHOUSE;

    throw std::runtime_error("Unknown environment type: " + env_name);
}

std::string GazeboLauncher::GetEnvironmentTypeString(EnvironmentType env_type) const {
    switch (env_type) {
        case EnvironmentType::MINIMAL: return "minimal";
        case EnvironmentType::WAREHOUSE: return "warehouse";
        default: return "unknown";
    }
}

// Environment initialization
void GazeboLauncher::InitializeEnvironments() {
    available_environments_.clear();

    // Minimal environment (current stable world)
    available_environments_.emplace_back(
        "Minimal Test Environment",
        "Simple test world for basic LiDAR testing - ultra-stable",
        EnvironmentType::MINIMAL,
        "demo/worlds/lidar_3d_world.sdf"
    );

    // Warehouse environment (tugbot_depot) - now using local file with GUI plugins
    available_environments_.emplace_back(
        "Tugbot Depot Warehouse",
        "MovAI Tugbot depot - realistic warehouse with charging stations (local)",
        EnvironmentType::WAREHOUSE,
        "demo/worlds/tugbot_depot.sdf"
    );

    // Set spawn poses and camera poses
    for (auto& env : available_environments_) {
        switch (env.type) {
            case EnvironmentType::MINIMAL:
                env.lidar_spawn_pose = gz::math::Pose3d(0, 0, 1, 0, 0, 0);
                env.camera_pose = gz::math::Pose3d(-6, 0, 6, 0, 0.5, 0);
                break;
            case EnvironmentType::WAREHOUSE:
                env.lidar_spawn_pose = gz::math::Pose3d(0, 0, 1, 0, 0, 0);
                env.camera_pose = gz::math::Pose3d(-10, 0, 8, 0, 0.5, 0);
                break;
            default:
                env.lidar_spawn_pose = gz::math::Pose3d(0, 0, 1, 0, 0, 0);
                env.camera_pose = gz::math::Pose3d(-6, 0, 6, 0, 0.5, 0);
                break;
        }
    }
}

// Runtime model injection implementation
std::string GazeboLauncher::DetectWorldName() {
    auto node = std::make_shared<gz::transport::Node>();

    // Get list of available topics to find world name
    std::vector<std::string> topics;
    node->TopicList(topics);

    std::cout << "Available topics for world detection:" << std::endl;
    for (const auto& topic : topics) {
        if (topic.find("/world/") != std::string::npos) {
            std::cout << "  " << topic << std::endl;
        }
    }

    // Look for world-specific topics (e.g., /world/world_name/clock)
    for (const auto& topic : topics) {
        if (topic.find("/world/") != std::string::npos && topic.find("/clock") != std::string::npos) {
            // Extract world name from topic: /world/world_name/clock
            size_t start = topic.find("/world/") + 7;  // Skip "/world/"
            size_t end = topic.find("/", start);
            if (end != std::string::npos) {
                std::string world_name = topic.substr(start, end - start);
                std::cout << "Detected world name: " << world_name << std::endl;
                return world_name;
            }
        }
    }

    // Default fallback
    std::cout << "Could not detect world name, using default: default" << std::endl;
    return "default";
}

std::string GazeboLauncher::PrepareLidarModelSDF(const std::string& world_name, const gz::math::Pose3d& spawn_pose) {
    // Try multiple possible paths for the LiDAR platform template
    std::vector<std::string> possible_paths = {
        "models/lidar_platform.sdf",
        "../models/lidar_platform.sdf",
        "./lidar_3d/models/lidar_platform.sdf",
        "/home/user/Documents/BazelStarterTemplate/lidar_3d/models/lidar_platform.sdf"
    };

    std::ifstream file;
    std::string successful_path;

    for (const auto& path : possible_paths) {
        file.open(path);
        if (file.is_open()) {
            successful_path = path;
            std::cout << "Found LiDAR model file at: " << path << std::endl;
            break;
        }
    }

    if (!file.is_open()) {
        std::cerr << "Failed to open LiDAR platform model file at any of the following paths:" << std::endl;
        for (const auto& path : possible_paths) {
            std::cerr << "  - " << path << std::endl;
        }
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string model_sdf = buffer.str();

    // Replace world name placeholder
    size_t pos = model_sdf.find("WORLD_NAME_PLACEHOLDER");
    while (pos != std::string::npos) {
        model_sdf.replace(pos, 22, world_name);  // 22 is length of "WORLD_NAME_PLACEHOLDER"
        pos = model_sdf.find("WORLD_NAME_PLACEHOLDER", pos + world_name.length());
    }

    // Update pose in the SDF
    std::stringstream pose_str;
    pose_str << spawn_pose.Pos().X() << " " << spawn_pose.Pos().Y() << " " << spawn_pose.Pos().Z() << " "
             << spawn_pose.Rot().Roll() << " " << spawn_pose.Rot().Pitch() << " " << spawn_pose.Rot().Yaw();

    // Replace pose placeholder (assumes default pose is "0 0 1 0 0 0")
    pos = model_sdf.find("<pose>0 0 1 0 0 0</pose>");
    if (pos != std::string::npos) {
        model_sdf.replace(pos + 6, 11, pose_str.str());  // Replace "0 0 1 0 0 0" with actual pose
    }

    return model_sdf;
}

bool GazeboLauncher::SpawnModelViaTransport(const std::string& model_sdf, const std::string& model_name) {
    auto node = std::make_shared<gz::transport::Node>();

    // Prepare the entity factory message
    gz::msgs::EntityFactory req;
    req.set_sdf(model_sdf);
    req.set_name(model_name);

    // Construct service topic for entity creation
    std::string service_topic = "/world/" + current_world_name_ + "/create";

    std::cout << "Spawning LiDAR platform via service: " << service_topic << std::endl;

    // Call the service to spawn the model
    gz::msgs::Boolean resp;
    bool result;
    unsigned int timeout = 5000;  // 5 second timeout

    bool service_result = node->Request(service_topic, req, timeout, resp, result);

    if (service_result && result && resp.data()) {
        std::cout << "Successfully spawned LiDAR platform model" << std::endl;
        return true;
    } else {
        std::cerr << "Failed to spawn LiDAR platform model" << std::endl;
        std::cerr << "Service result: " << service_result << ", Result: " << result;
        if (service_result) {
            std::cerr << ", Response: " << resp.data();
        }
        std::cerr << std::endl;
        return false;
    }
}

bool GazeboLauncher::InjectLidarPlatformRuntime(const gz::math::Pose3d& spawn_pose, const std::string& world_name) {
    current_world_name_ = world_name;

    // Prepare LiDAR model SDF with correct world name and pose
    std::string model_sdf = PrepareLidarModelSDF(world_name, spawn_pose);
    if (model_sdf.empty()) {
        return false;
    }

    // Spawn the model via transport
    return SpawnModelViaTransport(model_sdf, "lidar_platform");
}

} // namespace lidar3d
