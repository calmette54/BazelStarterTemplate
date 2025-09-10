#pragma once

#include <string>
#include <vector>
#include <gz/math/Pose3.hh>

namespace lidar3d {

enum class EnvironmentType {
    MINIMAL,     // Current 3-box world (ultra-stable)
    WAREHOUSE    // Tugbot depot warehouse
};

struct EnvironmentConfig {
    std::string name;
    std::string description;
    EnvironmentType type;
    std::string world_file_path;      // Local file path
    std::string fuel_url;             // Gazebo Fuel URL (if applicable)
    gz::math::Pose3d lidar_spawn_pose;
    gz::math::Pose3d camera_pose;
    bool requires_internet;
    
    // Default constructor
    EnvironmentConfig() : type(EnvironmentType::MINIMAL), requires_internet(false) {}
    
    EnvironmentConfig(const std::string& n, const std::string& desc, EnvironmentType t,
                     const std::string& path, const std::string& url = "")
        : name(n), description(desc), type(t), world_file_path(path), 
          fuel_url(url), requires_internet(!url.empty()) {}
};

class GazeboLauncher {
public:
    GazeboLauncher();
    ~GazeboLauncher();

    // Original launch method (preserved for backward compatibility)
    bool Launch(const std::string& world_file);
    
    // Enhanced launch with environment selection
    bool LaunchWithEnvironment(EnvironmentType env_type);
    bool LaunchWithEnvironment(const EnvironmentConfig& env_config);
    
    // Environment management
    std::vector<EnvironmentConfig> GetAvailableEnvironments() const;
    EnvironmentConfig GetEnvironmentConfig(EnvironmentType env_type) const;
    EnvironmentType GetEnvironmentTypeFromString(const std::string& env_name) const;
    std::string GetEnvironmentTypeString(EnvironmentType env_type) const;
    
    // Get current world name (for MovementController and sensor integration)
    std::string GetCurrentWorldName() const { return current_world_name_; }
    
    bool IsRunning() const;
    void Shutdown();

    // Wait for simulation to be ready
    bool WaitForReady(int timeout_seconds = 30);

private:
    bool running_ = false;
    
    // Environment initialization
    void InitializeEnvironments();
    std::vector<EnvironmentConfig> available_environments_;
    
    // Runtime model injection methods
    bool InjectLidarPlatformRuntime(const gz::math::Pose3d& spawn_pose, const std::string& world_name);
    std::string DetectWorldName();
    std::string PrepareLidarModelSDF(const std::string& world_name, const gz::math::Pose3d& spawn_pose);
    bool SpawnModelViaTransport(const std::string& model_sdf, const std::string& model_name);
    
    // Current world name tracking
    std::string current_world_name_;
    EnvironmentConfig current_environment_;
};

} // namespace lidar3d
