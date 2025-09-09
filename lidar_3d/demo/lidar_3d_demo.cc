#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <signal.h>
#include <mutex>
#include <atomic>

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
    bool Initialize() {
        // Launch Gazebo with visualization
        std::cout << "Launching Gazebo GUI..." << std::endl;
        if (!launcher_.Launch("demo/worlds/lidar_3d_world.sdf")) {
            std::cerr << "Failed to launch Gazebo" << std::endl;
            return false;
        }

        // Wait for simulation
        if (!launcher_.WaitForReady()) {
            std::cerr << "Simulation failed to start" << std::endl;
            return false;
        }

        // Initialize sensor with callback
        if (!sensor_.Initialize("/world/lidar_world/model/lidar_platform/link/lidar_3d_link/sensor/lidar_3d/scan",
                               [this](const PointCloud& cloud) { ProcessPointCloud(cloud); })) {
            std::cerr << "Failed to initialize sensor" << std::endl;
            return false;
        }

        // Initialize movement control
        if (!movement_.Initialize("lidar_world", "lidar_platform")) {
            std::cerr << "Failed to initialize movement" << std::endl;
            return false;
        }

        return true;
    }

    void Run() {
        std::cout << "\n=== 3D LiDAR Demo ===" << std::endl;
        std::cout << "======================" << std::endl;
        std::cout << "\nKeyboard controls:" << std::endl;
        std::cout << "  W/S: Forward/Backward" << std::endl;
        std::cout << "  A/D: Turn Left/Right" << std::endl;
        std::cout << "  Q/E: Move Left/Right" << std::endl;
        std::cout << "  R/F: Move Up/Down" << std::endl;
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

int main(int argc, char** argv) {
    signal(SIGINT, signalHandler);

    std::cout << "3D LiDAR Production Demo" << std::endl;
    std::cout << "========================" << std::endl;

    LidarDemo demo;
    if (!demo.Initialize()) {
        return 1;
    }

    demo.Run();

    std::cout << "\nDemo completed successfully!" << std::endl;
    return 0;
}
