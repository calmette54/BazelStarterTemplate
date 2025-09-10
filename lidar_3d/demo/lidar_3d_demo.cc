#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <signal.h>
#include <mutex>
#include <atomic>
#include <cstring>

#include "../lib/core/point_cloud.h"
#include "../lib/sensor/lidar_3d_sensor.h"
#include "../lib/control/movement_controller.h"
#include "../lib/visualization/gazebo_launcher.h"

using namespace lidar3d;

std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    std::cout << "\nShutting down..." << std::endl;
    g_running = false;
}

class LidarDemo {
public:
    bool Initialize(EnvironmentType env_type = EnvironmentType::MINIMAL) {
        // Launch Gazebo with selected environment
        try {
            auto env_config = launcher_.GetEnvironmentConfig(env_type);
            std::cout << "Launching environment: " << env_config.name << std::endl;
            std::cout << "Description: " << env_config.description << std::endl;
            
            if (!launcher_.LaunchWithEnvironment(env_type)) {
                std::cerr << "Failed to launch Gazebo with environment" << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error launching environment: " << e.what() << std::endl;
            return false;
        }

        // Wait for simulation
        if (!launcher_.WaitForReady()) {
            std::cerr << "Simulation failed to start" << std::endl;
            return false;
        }

        // Get dynamic world name from launcher
        std::string world_name = launcher_.GetCurrentWorldName();
        std::cout << "Connected to world: " << world_name << std::endl;

        // Build dynamic sensor topic path
        std::string sensor_topic = "/world/" + world_name + "/model/lidar_platform/link/lidar_3d_link/sensor/lidar_3d/scan";
        std::cout << "Sensor topic: " << sensor_topic << std::endl;

        // Initialize sensor with callback
        if (!sensor_.Initialize(sensor_topic,
                               [this](const PointCloud& cloud) { ProcessPointCloud(cloud); })) {
            std::cerr << "Failed to initialize sensor" << std::endl;
            return false;
        }

        // Initialize movement control with dynamic world name
        if (!movement_.Initialize(world_name, "lidar_platform")) {
            std::cerr << "Failed to initialize movement" << std::endl;
            return false;
        }

        return true;
    }

    void Run() {
        std::cout << "\n=== 3D LiDAR Demo ===" << std::endl;
        std::cout << "======================" << std::endl;
        std::cout << "\nKeyboard controls:" << std::endl;
        std::cout << "  W/S: Forward/Backward (orientation-aware)" << std::endl;
        std::cout << "  A/D: Turn Left/Right (yaw rotation)" << std::endl;
        std::cout << "  Q/E: Strafe Left/Right (orientation-aware)" << std::endl;
        std::cout << "  R/F: Move Up/Down (world space)" << std::endl;
        std::cout << "  ↑/↓: Pitch Up/Down" << std::endl;
        std::cout << "  ←/→: Yaw Left/Right (same as A/D)" << std::endl;
        std::cout << "  Ctrl+C: Exit\n" << std::endl;

        std::cout << "GUI Instructions:" << std::endl;
        std::cout << "- Press the PLAY button ▶️  to start simulation" << std::endl;
        std::cout << "- Point clouds will appear as colored dots" << std::endl;
        std::cout << "- Use mouse to rotate, zoom, and navigate the 3D view" << std::endl;
        std::cout << "- Analysis results are printed every 5 seconds\n" << std::endl;

        // Start systems
        movement_.StartKeyboardControl();

        // Main loop
        auto last_report = std::chrono::steady_clock::now();
        while (g_running) {
            // Check for batched pose updates (30Hz timer)
            movement_.UpdatePlatformPose();

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(
                    now - last_report).count() >= 5) {
                PrintStatus();
                last_report = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Cleanup
        movement_.StopKeyboardControl();
        sensor_.Shutdown();
        launcher_.Shutdown();
    }

private:
    void ProcessPointCloud(const PointCloud& cloud) {
        std::lock_guard<std::mutex> lock(mutex_);
        last_cloud_ = cloud;
        total_clouds_++;

        // Simple processing for demo
        if (!cloud.Empty()) {
            auto centroid = cloud.ComputeCentroid();
            auto bounds = cloud.ComputeBounds();
            // Store for status display
            last_centroid_ = centroid;
            last_bounds_ = bounds;
        }
    }

    void PrintStatus() {
        std::lock_guard<std::mutex> lock(mutex_);

        std::cout << "\n--- Status Report ---" << std::endl;
        std::cout << "Point clouds received: " << total_clouds_ << std::endl;
        std::cout << "Messages received: " << sensor_.GetMessagesReceived() << std::endl;

        if (!last_cloud_.Empty()) {
            std::cout << "Latest cloud: " << last_cloud_.Size() << " points" << std::endl;
            std::cout << "Centroid: [" << std::fixed << std::setprecision(2)
                      << last_centroid_.x() << ", "
                      << last_centroid_.y() << ", "
                      << last_centroid_.z() << "]" << std::endl;

            std::cout << "Bounds: ["
                      << last_bounds_.first.x() << " to " << last_bounds_.second.x() << ", "
                      << last_bounds_.first.y() << " to " << last_bounds_.second.y() << ", "
                      << last_bounds_.first.z() << " to " << last_bounds_.second.z() << "]"
                      << std::endl;

            // Matrix info
            const auto& points = last_cloud_.GetPoints();
            if (points.rows() > 0) {
                std::cout << "Eigen Matrix: " << points.rows() << " × " << points.cols()
                          << " (first point: [" << std::setprecision(2)
                          << points(0, 0) << ", " << points(0, 1) << ", " << points(0, 2) << "])"
                          << std::endl;
            }
        }
    }

    GazeboLauncher launcher_;
    Lidar3DSensor sensor_;
    MovementController movement_;

    std::mutex mutex_;
    PointCloud last_cloud_;
    Eigen::Vector3f last_centroid_{0, 0, 0};
    std::pair<Eigen::Vector3f, Eigen::Vector3f> last_bounds_;
    size_t total_clouds_ = 0;
};

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  --env <environment>    Environment to use (minimal, warehouse)" << std::endl;
    std::cout << "  --list-environments    List available environments" << std::endl;
    std::cout << "  --help                 Show this help message" << std::endl;
    std::cout << "\nEnvironments:" << std::endl;
    std::cout << "  minimal     - Simple test world for basic LiDAR testing (default)" << std::endl;
    std::cout << "  warehouse   - Tugbot depot warehouse environment (realistic)" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  " << program_name << "                    # Run with minimal environment" << std::endl;
    std::cout << "  " << program_name << " --env warehouse    # Run with warehouse environment" << std::endl;
}

int main(int argc, char** argv) {
    signal(SIGINT, signalHandler);

    // Parse command line arguments
    EnvironmentType env_type = EnvironmentType::MINIMAL;  // Default
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--list-environments") == 0) {
            std::cout << "Available environments:" << std::endl;
            GazeboLauncher temp_launcher;
            auto environments = temp_launcher.GetAvailableEnvironments();
            for (const auto& env : environments) {
                std::cout << "  " << temp_launcher.GetEnvironmentTypeString(env.type) 
                          << " - " << env.name << ": " << env.description << std::endl;
            }
            return 0;
        } else if (strcmp(argv[i], "--env") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: --env requires an environment name" << std::endl;
                printUsage(argv[0]);
                return 1;
            }
            
            try {
                GazeboLauncher temp_launcher;
                env_type = temp_launcher.GetEnvironmentTypeFromString(argv[i + 1]);
                i++;  // Skip next argument since we consumed it
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid environment '" << argv[i + 1] << "'" << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown argument '" << argv[i] << "'" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    std::cout << "3D LiDAR Production Demo" << std::endl;
    std::cout << "========================" << std::endl;

    LidarDemo demo;
    if (!demo.Initialize(env_type)) {
        return 1;
    }

    demo.Run();

    std::cout << "\nDemo completed successfully!" << std::endl;
    return 0;
}
