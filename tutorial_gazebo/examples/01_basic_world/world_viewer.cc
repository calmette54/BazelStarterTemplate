/*
 * Gazebo Tutorial Example 01: Headless World Viewer
 * 
 * This example demonstrates:
 * 1. Loading and running SDF worlds in headless mode
 * 2. Server-side simulation without GUI dependencies
 * 3. Long-running simulation processes
 * 4. Signal handling for graceful shutdown
 * 
 * Learning objectives:
 * - Understand Gazebo's headless server mode
 * - Learn about simulation lifecycle management
 * - Explore server-side world processing
 * - Signal-based application control
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/common/Console.hh>
#include <gz/common/Filesystem.hh>

#include <iostream>
#include <memory>
#include <thread>
#include <signal.h>
#include <atomic>
#include <chrono>
#include <cstdlib>

using namespace gz;

// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

/// \brief Signal handler for graceful shutdown
void signalHandler(int signal)
{
    gzmsg << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    g_running = false;
}

/// \brief Find SDF file in runfiles or relative paths
std::string FindSdfFile(const std::string &filename)
{
    // Try common locations for SDF files
    std::vector<std::string> paths = {
        filename,                                    // Direct path
        "worlds/" + filename,                        // Relative to current directory
        "../worlds/" + filename,                     // Parent directory
        "tutorial_gazebo/worlds/" + filename,        // Bazel runfiles path
        "external/tutorial_gazebo/worlds/" + filename, // External workspace path
    };

    for (const auto &path : paths)
    {
        if (gz::common::exists(path))
        {
            gzmsg << "Found SDF file at: " << path << std::endl;
            return path;
        }
    }

    gzerr << "Could not find SDF file: " << filename << std::endl;
    gzmsg << "Searched paths:" << std::endl;
    for (const auto &path : paths)
    {
        gzmsg << "  - " << path << std::endl;
    }
    
    return filename; // Return original if not found
}

class HeadlessWorldViewer
{
public:
    HeadlessWorldViewer(const std::string &worldFile) : worldFile_(FindSdfFile(worldFile)) {}

    int Run()
    {
        gzmsg << "Starting Headless World Viewer..." << std::endl;
        gzmsg << "World file: " << worldFile_ << std::endl;

        // Set console verbosity and disable config file loading
        gz::common::Console::SetVerbosity(2);
        setenv("GZ_SIM_SERVER_CONFIG_PATH", "", 1);
        setenv("GZ_CONFIG_PATH", "", 1);

        // Configure the simulation server for headless operation
        sim::ServerConfig serverConfig;
        serverConfig.SetSdfFile(worldFile_);
        serverConfig.SetUpdateRate(100.0);  // 100 Hz update rate
        serverConfig.SetUseLevels(true);

        // Note: In Gazebo Harmonic, system plugins should be configured via SDF
        // rather than programmatically through ServerConfig::AddPlugin
        // The world SDF file includes the necessary plugin configurations
        
        // Create and start the server
        server_ = std::make_unique<sim::Server>(serverConfig);
        
        // Check if server was constructed properly and start manually if needed
        gzmsg << "Server constructed, checking if running..." << std::endl;
        
        if (!server_->Running())
        {
            gzmsg << "Server not running after construction, attempting to start manually..." << std::endl;
            bool started = server_->Run();
            if (!started)
            {
                gzerr << "Failed to start server manually" << std::endl;
                return -1;
            }
            
            if (!server_->Running())
            {
                gzerr << "Server still not running after manual start" << std::endl;
                return -1;
            }
        }

        gzmsg << "Headless simulation server started successfully!" << std::endl;
        gzmsg << "World loaded: " << worldFile_ << std::endl;
        gzmsg << "Press Ctrl+C to stop the simulation" << std::endl;

        // Run the simulation loop until stopped
        size_t iterations = 0;
        auto startTime = std::chrono::steady_clock::now();
        
        while (server_->Running() && g_running)
        {
            // Run one simulation step
            server_->RunOnce();
            iterations++;
            
            // Print status every 1000 iterations (10 seconds at 100Hz)
            if (iterations % 1000 == 0)
            {
                auto currentTime = std::chrono::steady_clock::now();
                auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>
                    (currentTime - startTime).count();
                
                gzmsg << "Simulation running - Steps: " << iterations 
                      << ", Elapsed: " << elapsedSeconds << "s" << std::endl;
            }
            
            // Small sleep to prevent excessive CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        gzmsg << "Simulation stopped after " << iterations << " iterations" << std::endl;
        
        // Cleanup
        Cleanup();
        return 0;
    }

private:
    void Cleanup()
    {
        gzmsg << "Shutting down Headless World Viewer..." << std::endl;
        
        if (server_)
        {
            server_.reset();
        }

        gzmsg << "Headless World Viewer shutdown complete." << std::endl;
    }

private:
    std::string worldFile_;
    std::unique_ptr<sim::Server> server_;
};

int main(int argc, char *argv[])
{
    // Set up console logging
    gz::common::Console::SetVerbosity(2);

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // Termination signal

    // Determine world file to load
    std::string worldFile = "basic_world_simple.sdf";
    if (argc > 1)
    {
        worldFile = argv[1];
    }

    try
    {
        HeadlessWorldViewer viewer(worldFile);
        return viewer.Run();
    }
    catch (const std::exception &e)
    {
        gzerr << "Exception in Headless World Viewer: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        gzerr << "Unknown exception in Headless World Viewer" << std::endl;
        return -1;
    }
}

/*
 * Key Concepts Demonstrated:
 * 
 * 1. **Headless Simulation**: How to run Gazebo simulations without GUI
 *    dependencies, suitable for server environments or CI/CD.
 * 
 * 2. **Server-side Processing**: Understanding how to run long-running
 *    simulations with minimal resource overhead.
 * 
 * 3. **Signal Handling**: Proper application lifecycle management with
 *    graceful shutdown on system signals.
 * 
 * 4. **Plugin Architecture**: Essential server-side systems for physics,
 *    commands, and data broadcasting.
 * 
 * 5. **Resource Management**: Efficient memory and CPU usage patterns
 *    for continuous simulation operation.
 * 
 * Usage Examples:
 * - ./world_viewer                          # Load default world
 * - ./world_viewer worlds/lighting_demo.sdf # Load specific world
 * - ./world_viewer my_custom_world.sdf      # Load custom world
 * 
 * Advanced Features to Explore:
 * - Integration with external visualization tools (Rviz, custom viewers)
 * - Remote control via Gazebo Transport topics and services
 * - Data logging and analysis for batch simulation runs
 * - Integration with containerized deployment environments
 */