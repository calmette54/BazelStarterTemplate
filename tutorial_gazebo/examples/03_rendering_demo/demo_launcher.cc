/*
 * Integrated Demo Launcher for Gazebo Tutorial Examples
 *
 * This launcher integrates Bazel-built gz::sim::Server with system-installed
 * gz-gui to provide complete 3D visualization for tutorial demos.
 *
 * Architecture:
 * 1. Detects system Gazebo installation (gz command availability)
 * 2. Starts gz::sim::Server as headless simulation backend (Bazel-built)
 * 3. Launches system gz-gui for 3D visualization frontend
 * 4. Connects them via gz-transport for real-time scene updates
 * 5. Coordinates graceful shutdown of both processes
 *
 * Dependencies:
 * - System Gazebo installation (gz-harmonic recommended)
 *   Ubuntu/Debian: sudo apt install gz-harmonic
 *   Fedora: sudo dnf install gz-harmonic
 *   macOS: brew install gazebo
 * - All simulation libraries from Bazel Central Registry (hermetic)
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/common/Console.hh>
#include <gz/transport/Node.hh>
#include <gz/sim/Entity.hh>

// Note: Static plugins are linked via BUILD.bazel dependencies
// They are automatically discovered when statically linked

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <string>
#include <vector>

namespace gz
{
namespace tutorial
{

/// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

/// Process IDs for child processes
pid_t gui_process_id = -1;

/**
 * @brief Verify that scene broadcasting is working via gz-transport
 * @return True if scene topics are being published
 */
bool VerifySceneBroadcasting()
{
    auto node = std::make_shared<gz::transport::Node>();

    // Wait briefly for topics to be advertised
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Check for scene-related topics
    std::vector<std::string> topics;
    node->TopicList(topics);

    gzmsg << "TUTORIAL: Available gz-transport topics:" << std::endl;
    bool found_scene_topics = false;

    for (const auto& topic : topics)
    {
        gzmsg << "  - " << topic << std::endl;

        // Look for critical scene broadcasting topics
        if (topic.find("/world/material_showcase/") != std::string::npos ||
            topic.find("/scene/info") != std::string::npos ||
            topic.find("/world/") != std::string::npos)
        {
            found_scene_topics = true;
            gzmsg << "    ^^ SCENE-RELATED TOPIC FOUND!" << std::endl;
        }
    }

    if (found_scene_topics)
    {
        gzmsg << "TUTORIAL: ✓ Scene broadcasting topics detected - 3D visualization should work!" << std::endl;
    }
    else
    {
        gzwarn << "TUTORIAL: ⚠️ No scene topics found - SceneBroadcaster may not be working" << std::endl;
    }

    return found_scene_topics;
}

/// Signal handler for graceful shutdown
void signalHandler(int signal)
{
    gzmsg << "\n=== TUTORIAL: Received signal " << signal << ", shutting down demo launcher ===\n";
    g_running = false;

    // Terminate GUI process if running
    if (gui_process_id > 0)
    {
        kill(gui_process_id, SIGTERM);
        gzmsg << "TUTORIAL: Sent termination signal to gz-gui process\n";
    }
}

/**
 * @brief Check if system Gazebo installation is available
 * @return True if gz command is available and functional
 */
bool CheckSystemGazebo()
{
    // Check for gz command in PATH
    int gz_check = system("which gz > /dev/null 2>&1");
    if (gz_check != 0)
    {
        gzerr << "TUTORIAL: System Gazebo not found!" << std::endl;
        gzerr << "TUTORIAL: Please install Gazebo for 3D visualization:" << std::endl;
        gzerr << "TUTORIAL:   Ubuntu/Debian: sudo apt install gz-harmonic" << std::endl;
        gzerr << "TUTORIAL:   Fedora: sudo dnf install gz-harmonic" << std::endl;
        gzerr << "TUTORIAL:   macOS: brew install gazebo" << std::endl;
        return false;
    }

    // Get Gazebo version info
    FILE* version_pipe = popen("gz --version 2>/dev/null", "r");
    if (version_pipe)
    {
        char version_buffer[256];
        if (fgets(version_buffer, sizeof(version_buffer), version_pipe) != nullptr)
        {
            // Remove trailing newline
            version_buffer[strcspn(version_buffer, "\n")] = 0;
            gzmsg << "TUTORIAL: Found system Gazebo: " << version_buffer << std::endl;
        }
        pclose(version_pipe);
    }

    // Check if gz sim command is available (needed for GUI)
    int sim_check = system("gz sim --help > /dev/null 2>&1");
    if (sim_check != 0)
    {
        gzwarn << "TUTORIAL: 'gz sim' command not available, GUI may not work" << std::endl;
        return false;
    }

    gzmsg << "TUTORIAL: System Gazebo installation verified!" << std::endl;
    return true;
}

/**
 * @brief Launch system gz-gui as a separate process with custom configuration
 * @param world_file Path to SDF world file for visualization
 * @return Process ID of gz-gui or -1 on failure
 */
pid_t LaunchSystemGzGui(const std::string& world_file = "")
{
    pid_t pid = fork();

    if (pid == 0)
    {
        // Child process - execute system gz-gui with custom configuration
        // This loads our specific plugins for 3D visualization
        std::vector<const char*> args;

        args.push_back("gz");
        args.push_back("gui");
        args.push_back("--config");

        // Use absolute path to GUI config file - stored as static string to ensure lifetime
        static std::string config_path;
        if (config_path.empty())
        {
            const char* workspace_dir = getenv("BUILD_WORKSPACE_DIRECTORY");
            if (workspace_dir)
            {
                config_path = std::string(workspace_dir) + "/examples/03_rendering_demo/gz_gui_minimal.xml";
            }
            else
            {
                config_path = "examples/03_rendering_demo/gz_gui_minimal.xml";
            }
            gzmsg << "TUTORIAL: Using GUI config file: " << config_path << std::endl;
        }
        args.push_back(config_path.c_str());
        args.push_back("-v");  // Verbose output
        args.push_back("4");   // Verbosity level

        args.push_back(nullptr);

        // Set environment for coordination with our server
        setenv("GZ_PARTITION", "tutorial_demo", 1);

        // Give server time to start first and begin broadcasting
        gzmsg << "TUTORIAL: Starting GUI client with custom 3D visualization config..." << std::endl;
        sleep(3);  // Longer wait for server initialization

        // Execute system gz-gui command
        execvp(args[0], const_cast<char* const*>(args.data()));

        // If we reach here, exec failed
        gzerr << "TUTORIAL: Failed to launch system gz-gui: " << strerror(errno) << std::endl;
        exit(1);
    }
    else if (pid > 0)
    {
        // Parent process
        gzmsg << "TUTORIAL: Launched system gz-gui with custom config, PID: " << pid << std::endl;
        return pid;
    }
    else
    {
        // Fork failed
        gzerr << "TUTORIAL: Failed to fork gz-gui process: " << strerror(errno) << std::endl;
        return -1;
    }
}

/**
 * @brief Integrated demo launcher class
 */
class DemoLauncher
{
public:
    DemoLauncher(const std::string& demo_name, const std::string& sdf_file)
        : demo_name_(demo_name), sdf_file_(sdf_file)
    {}

    /**
     * @brief Run the integrated demo with 3D visualization
     */
    int Run()
    {
        gzmsg << "\n" << std::string(80, '=') << std::endl;
        gzmsg << "TUTORIAL: Integrated Demo Launcher - " << demo_name_ << std::endl;
        gzmsg << std::string(80, '=') << std::endl;

        // Check system Gazebo availability for 3D visualization
        bool has_system_gazebo = CheckSystemGazebo();
        if (!has_system_gazebo)
        {
            gzwarn << "TUTORIAL: No system Gazebo installation found" << std::endl;
            gzwarn << "TUTORIAL: Running in console-only mode (no 3D visualization)" << std::endl;
            gzwarn << "TUTORIAL: For 3D visualization, please install system Gazebo" << std::endl;
            // Continue execution in console mode - don't return error
        }

        // Stage 1: Setup simulation server
        gzmsg << "\n--- STAGE 1: Starting Gazebo Simulation Server ---" << std::endl;

        gz::sim::ServerConfig config;

        // Resolve SDF file path - try different locations
        std::string resolved_sdf;
        std::vector<std::string> possible_paths = {
            sdf_file_,  // Direct path as provided
            std::string(getenv("BUILD_WORKSPACE_DIRECTORY") ? getenv("BUILD_WORKSPACE_DIRECTORY") : ".") +
                "/tutorial_gazebo/" + sdf_file_,
            "tutorial_gazebo/" + sdf_file_,
            sdf_file_
        };

        for (const auto& path : possible_paths)
        {
            if (access(path.c_str(), F_OK) == 0)
            {
                resolved_sdf = path;
                gzmsg << "TUTORIAL: Found SDF file at: " << resolved_sdf << std::endl;
                break;
            }
        }

        if (resolved_sdf.empty())
        {
            gzwarn << "TUTORIAL: Could not find SDF file: " << sdf_file_ << std::endl;
            gzwarn << "TUTORIAL: Will try to use original path" << std::endl;
            resolved_sdf = sdf_file_;
        }

        config.SetSdfFile(resolved_sdf);
        config.SetUpdateRate(1000.0);  // High update rate for smooth visualization

        // Set up environment exactly like material_showcase for consistency
        gz::common::Console::SetVerbosity(4);  // Match material_showcase verbosity

        // Set transport partition for isolation
        setenv("GZ_PARTITION", "tutorial_demo", 1);

        // Add system library path for dependencies
        const char* existing_ld_path = getenv("LD_LIBRARY_PATH");
        std::string new_ld_path = "/usr/lib/x86_64-linux-gnu";
        if (existing_ld_path) {
            new_ld_path += ":" + std::string(existing_ld_path);
        }
        setenv("LD_LIBRARY_PATH", new_ld_path.c_str(), 1);

        // Disable server config path to ensure defaults are loaded when SDF plugins fail
        setenv("GZ_SIM_SERVER_CONFIG_PATH", "", 1);
        setenv("GZ_CONFIG_PATH", "", 1);
        gzmsg << "TUTORIAL: Environment configured to match material_showcase for default plugin loading..." << std::endl;

        gz::sim::Server server(config);

        // Get the ECM to add SceneBroadcaster manually if needed
        gzmsg << "TUTORIAL: Server created with static plugins - SceneBroadcaster should be active" << std::endl;
        gzmsg << "TUTORIAL: Gazebo simulation server initialized with static plugins" << std::endl;

        // Stage 2: Start simulation first, then launch GUI
        gzmsg << "\n--- STAGE 2: Starting Simulation Server ---" << std::endl;

        // Start simulation in background thread to allow GUI to connect
        gzmsg << "TUTORIAL: Starting Bazel-built simulation server..." << std::endl;
        gzmsg << "TUTORIAL: Server will run with static plugins for reliable operation" << std::endl;

        // Start simulation steps to initialize the world and plugins
        gzmsg << "TUTORIAL: Running initial simulation steps to initialize SceneBroadcaster..." << std::endl;
        for (int i = 0; i < 100; ++i)
        {
            server.RunOnce(false);
        }

        gzmsg << "TUTORIAL: Simulation server initialized and running" << std::endl;

        // Verify that SceneBroadcaster is working and publishing topics
        gzmsg << "TUTORIAL: Verifying scene broadcasting via gz-transport..." << std::endl;
        bool scene_broadcasting = VerifySceneBroadcasting();

        if (scene_broadcasting)
        {
            gzmsg << "TUTORIAL: ✓ Scene broadcasting verified - GUI should receive 3D data!" << std::endl;
        }
        else
        {
            gzwarn << "TUTORIAL: ⚠️ Scene broadcasting not detected - GUI may show empty scene" << std::endl;
        }

        if (has_system_gazebo)
        {
            gzmsg << "\n--- STAGE 3: Launching 3D Visualization GUI ---" << std::endl;

            gui_process_id = LaunchSystemGzGui(sdf_file_);
            if (gui_process_id == -1)
            {
                gzwarn << "TUTORIAL: Failed to launch 3D visualization, continuing in console mode" << std::endl;
                has_system_gazebo = false;  // Fall back to console mode
            }
            else
            {
                // Wait a moment for GUI to initialize and connect
                gzmsg << "TUTORIAL: Waiting for 3D GUI to connect to simulation server..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        else
        {
            gzmsg << "\n--- STAGE 3: Console-Only Mode ---" << std::endl;
            gzmsg << "TUTORIAL: No 3D visualization (system Gazebo not available)" << std::endl;
            gzmsg << "TUTORIAL: Simulation will run with detailed console analysis" << std::endl;
        }

        // Stage 4: Run integrated demo
        gzmsg << "\n--- STAGE 4: Running Integrated Demo ---" << std::endl;
        if (has_system_gazebo && gui_process_id > 0)
        {
            gzmsg << "TUTORIAL: Both simulation and 3D visualization are now running!" << std::endl;
            gzmsg << "TUTORIAL: A 3D window should appear showing the simulation" << std::endl;
            gzmsg << "TUTORIAL: Use mouse to navigate: drag=rotate, wheel=zoom, Shift+drag=pan" << std::endl;
        }
        else
        {
            gzmsg << "TUTORIAL: Simulation is running in console-only mode" << std::endl;
            gzmsg << "TUTORIAL: Educational material analysis will be shown in the console" << std::endl;
        }
        gzmsg << "TUTORIAL: Press Ctrl+C to stop the demo" << std::endl;

        // Main execution loop
        size_t iteration_count = 0;
        auto start_time = std::chrono::steady_clock::now();

        while (g_running)
        {
            // Check if GUI process is still running
            int gui_status;
            pid_t gui_check = waitpid(gui_process_id, &gui_status, WNOHANG);
            if (gui_check != 0)
            {
                gzmsg << "TUTORIAL: GUI process has exited, stopping demo" << std::endl;
                break;
            }

            // Step simulation
            server.RunOnce(false);
            iteration_count++;

            // Progress updates every 5 seconds
            if (iteration_count % 5000 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                double sim_time = iteration_count * 0.001;
                double rtf = sim_time / (elapsed > 0 ? elapsed : 1);

                gzmsg << "TUTORIAL: Demo running - Sim: " << std::fixed << std::setprecision(1)
                      << sim_time << "s | RTF: " << std::setprecision(2) << rtf << "x" << std::endl;
            }

            // Brief pause to prevent excessive CPU usage
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        // Stage 5: Cleanup
        gzmsg << "\n--- STAGE 5: Demo Cleanup ---" << std::endl;

        // Terminate GUI process gracefully
        if (gui_process_id > 0)
        {
            kill(gui_process_id, SIGTERM);

            // Wait up to 5 seconds for graceful shutdown
            for (int i = 0; i < 50; ++i)
            {
                int status;
                if (waitpid(gui_process_id, &status, WNOHANG) != 0)
                {
                    gzmsg << "TUTORIAL: GUI process terminated gracefully" << std::endl;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // Force kill if still running
            kill(gui_process_id, SIGKILL);
            waitpid(gui_process_id, nullptr, 0);
        }

        gzmsg << "TUTORIAL: ✓ Integrated demo completed successfully!" << std::endl;
        gzmsg << std::string(80, '=') << std::endl;

        return 0;
    }

private:
    std::string demo_name_;
    std::string sdf_file_;
};

} // namespace tutorial
} // namespace gz

/**
 * @brief Main function for demo launcher
 */
int main(int argc, char* argv[])
{
    // Set up signal handlers
    signal(SIGINT, gz::tutorial::signalHandler);
    signal(SIGTERM, gz::tutorial::signalHandler);

    // Parse command line arguments
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <demo_name> <sdf_file>" << std::endl;
        std::cerr << "Example: " << argv[0] << " \"Material Showcase\" worlds/material_showcase_visual.sdf" << std::endl;
        return -1;
    }

    std::string demo_name = argv[1];
    std::string sdf_file = argv[2];

    try
    {
        gz::tutorial::DemoLauncher launcher(demo_name, sdf_file);
        return launcher.Run();
    }
    catch (const std::exception& e)
    {
        gzerr << "TUTORIAL: Exception in demo launcher: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        gzerr << "TUTORIAL: Unknown exception in demo launcher" << std::endl;
        return -1;
    }
}
