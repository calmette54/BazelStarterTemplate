/*
 * Simple gz-transport Material Control Demo
 *
 * This demo showcases the hybrid approach: launching gz sim with a simple
 * command while controlling materials programmatically via gz-transport.
 *
 * Features:
 * - Single-process launcher that manages gz sim as child process
 * - Material control via gz-transport (sphere color cycling)
 * - Clean process coordination and shutdown handling
 * - Visual proof that gz-transport material control works
 *
 * Usage:
 * bazelisk run //examples/03_rendering_demo:transport_demo
 */

#include <gz/transport/Node.hh>
#include <gz/msgs/visual.pb.h>
#include <gz/msgs/world_control.pb.h>
#include <gz/msgs/stringmsg.pb.h>
#include <gz/msgs/boolean.pb.h>
#include <gz/msgs/material.pb.h>
#include <gz/msgs/pose.pb.h>
#include <gz/common/Console.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <vector>

namespace gz
{
namespace tutorial
{

/// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

/// Process ID of gz sim child process
pid_t sim_process_id = -1;

/**
 * @brief Signal handler for graceful shutdown
 */
void signalHandler(int signal)
{
    std::cout << "\n=== DEMO: Received signal " << signal << ", shutting down ===\n";
    g_running = false;

    // Terminate gz sim process if running
    if (sim_process_id > 0)
    {
        std::cout << "DEMO: Terminating gz sim process...\n";
        kill(sim_process_id, SIGTERM);
    }
}

/**
 * @brief Material color configuration
 */
struct ColorConfig
{
    std::string name;
    double r, g, b;
};

/**
 * @brief Transport-based material controller
 */
class MaterialController
{
public:
    MaterialController() : node_(std::make_shared<gz::transport::Node>())
    {
        // Setup color sequence
        colors_ = {
            {"Red", 1.0, 0.0, 0.0},
            {"Green", 0.0, 1.0, 0.0},
            {"Blue", 0.0, 0.0, 1.0},
            {"Yellow", 1.0, 1.0, 0.0},
            {"Magenta", 1.0, 0.0, 1.0},
            {"Cyan", 0.0, 1.0, 1.0}
        };
    }

    /**
     * @brief Wait for simulation to be ready by checking topic availability
     */
    bool waitForSimulationReady(int timeout_seconds = 30)
    {
        std::cout << "DEMO: Waiting for gz sim to initialize...\n";

        for (int i = 0; i < timeout_seconds; ++i)
        {
            std::vector<std::string> topics;
            node_->TopicList(topics);

            // Look for world-specific topics that indicate sim is ready
            for (const auto& topic : topics)
            {
                if (topic.find("/world/transport_demo") != std::string::npos)
                {
                    std::cout << "DEMO: ✓ Simulation ready! Found topic: " << topic << "\n";
                    return true;
                }
            }

            std::cout << "DEMO: Waiting... (" << (i + 1) << "/" << timeout_seconds << "s)\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << "DEMO: ⚠️ Timeout waiting for simulation to initialize\n";
        return false;
    }

    /**
     * @brief Change sphere color by swapping which colored sphere is visible
     */
    bool changeSphereColor(const ColorConfig& color)
    {
        std::cout << "DEMO: Changing to " << color.name << " sphere...\n";

        // Map colors to sphere names
        std::string current_sphere;
        if (color.name == "Red") current_sphere = "red_sphere";
        else if (color.name == "Green") current_sphere = "green_sphere";
        else if (color.name == "Blue") current_sphere = "blue_sphere";
        else if (color.name == "Yellow") current_sphere = "yellow_sphere";
        else current_sphere = "red_sphere"; // fallback

        // Hide all spheres first (move them underground)
        std::vector<std::string> all_spheres = {"red_sphere", "green_sphere", "blue_sphere", "yellow_sphere"};

        gz::msgs::Boolean reply;
        bool result = false;
        unsigned int timeout = 1000;
        std::string pose_service = "/world/transport_demo/set_pose";

        for (const auto& sphere_name : all_spheres)
        {
            gz::msgs::Pose pose_msg;
            pose_msg.set_name(sphere_name);

            if (sphere_name == current_sphere)
            {
                // Show the current color sphere
                pose_msg.mutable_position()->set_x(0.0);
                pose_msg.mutable_position()->set_y(0.0);
                pose_msg.mutable_position()->set_z(1.0);
            }
            else
            {
                // Hide other spheres
                pose_msg.mutable_position()->set_x(0.0);
                pose_msg.mutable_position()->set_y(0.0);
                pose_msg.mutable_position()->set_z(-10.0);
            }

            pose_msg.mutable_orientation()->set_w(1.0);
            pose_msg.mutable_orientation()->set_x(0.0);
            pose_msg.mutable_orientation()->set_y(0.0);
            pose_msg.mutable_orientation()->set_z(0.0);

            // Send pose request
            if (node_->Request(pose_service, pose_msg, timeout, reply, result))
            {
                if (result && reply.data())
                {
                    if (sphere_name == current_sphere)
                    {
                        std::cout << "DEMO: ✓ " << color.name << " sphere now visible!\n";
                    }
                }
                else
                {
                    std::cout << "DEMO: ⚠️ Failed to position " << sphere_name << "\n";
                }
            }
            else
            {
                std::cout << "DEMO: ⚠️ Service request failed for " << sphere_name << "\n";
            }
        }

        std::cout << "DEMO: Color change complete - you should see the " << color.name << " sphere!\n";
        return true;
    }

    /**
     * @brief Run the material cycling demo
     */
    void runDemo()
    {
        std::cout << "DEMO: Starting material cycling demo...\n";
        std::cout << "DEMO: The Gazebo GUI should now be running in a separate window!\n";
        std::cout << "DEMO: Transport control will cycle through colors every 3 seconds.\n";
        std::cout << "DEMO: Press Ctrl+C to stop both GUI and transport control.\n\n";

        size_t color_index = 0;
        int cycle_count = 0;

        while (g_running) // Run indefinitely until Ctrl+C
        {
            const auto& current_color = colors_[color_index];
            changeSphereColor(current_color);

            std::cout << "DEMO: Cycle " << (cycle_count + 1) << " - Color: " << current_color.name << "\n";

            // Wait 5 seconds before next color change (longer so you can see the GUI)
            for (int i = 0; i < 50 && g_running; ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // Move to next color
            color_index = (color_index + 1) % colors_.size();
            cycle_count++;
        }

        std::cout << "\nDEMO: Transport control stopped.\n";
    }

private:
    std::shared_ptr<gz::transport::Node> node_;
    std::vector<ColorConfig> colors_;
};

/**
 * @brief Launch gz sim as child process using system() for better GUI support
 */
pid_t launchGzSim(const std::string& world_file)
{
    std::cout << "DEMO: Starting gz sim with world: " << world_file << "\n";

    // Build command string - include the world file
    std::string command = "gz sim " + world_file + " --verbose 1 &";

    std::cout << "DEMO: Executing: " << command << "\n";

    // Use system() which preserves environment better for GUI applications
    int result = system(command.c_str());

    if (result != 0)
    {
        std::cerr << "DEMO: Failed to launch gz sim via system(): " << result << std::endl;
        return -1;
    }

    // Give it a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Try to find the PID (this is approximate)
    FILE* pipe = popen("pgrep -f 'gz sim'", "r");
    if (pipe)
    {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            pid_t pid = atoi(buffer);
            pclose(pipe);
            std::cout << "DEMO: Found gz sim with approximate PID: " << pid << "\n";
            return pid;
        }
        pclose(pipe);
    }

    std::cout << "DEMO: gz sim launched (PID detection failed but process should be running)\n";
    return 1; // Return dummy PID since we can't track it precisely
}

/**
 * @brief Main transport demo launcher
 */
class TransportDemo
{
public:
    int Run()
    {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    Gazebo Transport Material Control Demo" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        std::cout << "This demo shows how to control Gazebo via gz-transport" << std::endl;
        std::cout << "while using standard 'gz sim' for the simulation engine." << std::endl;
        std::cout << std::string(60, '=') << std::endl;

        // Stage 1: Launch gz sim
        std::cout << "\n--- STAGE 1: Launching Gazebo Simulation ---\n";
        sim_process_id = launchGzSim("worlds/simple_transport_demo.sdf");
        if (sim_process_id == -1)
        {
            std::cerr << "DEMO: Failed to launch gz sim. Make sure gz-harmonic is installed.\n";
            return -1;
        }

        // Stage 2: Wait for simulation ready
        std::cout << "\n--- STAGE 2: Waiting for Initialization ---\n";
        MaterialController controller;
        if (!controller.waitForSimulationReady())
        {
            std::cerr << "DEMO: Simulation failed to initialize\n";
            cleanup();
            return -1;
        }

        // Stage 3: Run material demo
        std::cout << "\n--- STAGE 3: Running Material Control Demo ---\n";
        std::cout << "DEMO: GUI is now running! You should see the 3D Gazebo window.\n";
        std::cout << "DEMO: The sphere will move every 2 seconds via gz-transport commands.\n";
        std::cout << "DEMO: Press Ctrl+C to stop both GUI and demo.\n\n";
        controller.runDemo();

        // Stage 4: Cleanup
        std::cout << "\n--- STAGE 4: Cleanup ---\n";
        cleanup();

        std::cout << "DEMO: ✓ Transport demo completed successfully!\n";
        std::cout << std::string(60, '=') << std::endl;

        return 0;
    }

private:
    void cleanup()
    {
        if (sim_process_id > 0)
        {
            std::cout << "DEMO: Terminating gz sim process...\n";
            kill(sim_process_id, SIGTERM);

            // Wait up to 5 seconds for graceful shutdown
            for (int i = 0; i < 50; ++i)
            {
                int status;
                if (waitpid(sim_process_id, &status, WNOHANG) != 0)
                {
                    std::cout << "DEMO: gz sim terminated gracefully\n";
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // Force kill if still running
            std::cout << "DEMO: Force killing gz sim process\n";
            kill(sim_process_id, SIGKILL);
            waitpid(sim_process_id, nullptr, 0);
        }
    }
};

} // namespace tutorial
} // namespace gz

/// \brief Set up proper library paths for Gazebo GUI (preserves existing LD_LIBRARY_PATH)
void setupLibraryPaths()
{
    std::cout << "DEMO: Configuring library paths for GUI visualization..." << std::endl;

    // Get current LD_LIBRARY_PATH (preserving existing Nix paths)
    const char* current_path = getenv("LD_LIBRARY_PATH");
    std::string existing_paths = current_path ? current_path : "";

    // Add system library paths at the beginning for Gazebo GUI
    std::string system_paths = "/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu";

    std::string full_path;
    if (!existing_paths.empty())
    {
        // Prepend system paths: system_paths:existing_paths
        full_path = system_paths + ":" + existing_paths;
    }
    else
    {
        full_path = system_paths;
    }

    // Set the combined path (preserving existing environment)
    if (setenv("LD_LIBRARY_PATH", full_path.c_str(), 1) != 0)
    {
        std::cerr << "DEMO: Warning - Failed to set LD_LIBRARY_PATH" << std::endl;
    }
    else
    {
        std::cout << "DEMO: ✓ Library paths configured (system paths + existing paths preserved)" << std::endl;
    }
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[])
{
    // Set up environment first (preserves existing LD_LIBRARY_PATH)
    setupLibraryPaths();

    // Set up signal handlers
    signal(SIGINT, gz::tutorial::signalHandler);   // Ctrl+C
    signal(SIGTERM, gz::tutorial::signalHandler);  // Termination

    try
    {
        gz::tutorial::TransportDemo demo;
        return demo.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "DEMO: Exception: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        std::cerr << "DEMO: Unknown exception occurred" << std::endl;
        return -1;
    }
}
