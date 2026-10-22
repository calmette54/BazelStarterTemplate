#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <signal.h>
#include <mutex>
#include <atomic>
#include <cstring>

#include "lib/core/point_cloud.h"
#include "lib/sensor/lidar_3d_sensor.h"
#include "lib/control/movement_controller.h"
#include "lib/visualization/gazebo_launcher.h"

using namespace lidar3d;

std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    g_running = false;
}

class LidarDemo {
public:
    bool Initialize(const std::string& world_file) {
        if (!launcher_.Launch(world_file)) {
            return false;
        }

        if (!launcher_.WaitForReady()) {
            return false;
        }

        std::string world_name = launcher_.GetWorldName();
        std::string sensor_topic = "/world/" + world_name + "/model/lidar_platform/link/lidar_3d_link/sensor/lidar_3d/scan";

        if (!sensor_.Initialize(sensor_topic,
                               [this](const PointCloud& cloud) { ProcessPointCloud(cloud); })) {
            return false;
        }

        if (!movement_.Initialize(world_name, "lidar_platform")) {
            return false;
        }

        return true;
    }

    void Run() {
        std::cout << "\nKeyboard controls:" << std::endl;
        std::cout << "  WASD: Move and Turn\n" << std::endl;

        movement_.StartKeyboardControl();

        auto last_report = std::chrono::steady_clock::now();
        while (g_running) {
            movement_.UpdatePlatformPose();

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(
                    now - last_report).count() >= 5) {
                PrintStatus();
                last_report = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

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
        std::cout << "Point clouds: " << total_clouds_
                  << ", Points: " << (last_cloud_.Empty() ? 0 : last_cloud_.Size()) << std::endl;
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
    std::cout << "Usage: " << program_name << " [warehouse]" << std::endl;
    std::cout << "  Default: minimal world" << std::endl;
    std::cout << "  'warehouse': warehouse environment" << std::endl;
}

int main(int argc, char** argv) {
    signal(SIGINT, signalHandler);

    std::string world_file = "demo/worlds/lidar_3d_world.sdf";
    
    if (argc > 1) {
        if (strcmp(argv[1], "warehouse") == 0) {
            world_file = "demo/worlds/tugbot_depot.sdf";
        } else {
            printUsage(argv[0]);
            return 1;
        }
    }

    LidarDemo demo;
    if (!demo.Initialize(world_file)) {
        return 1;
    }

    demo.Run();
    return 0;
}
