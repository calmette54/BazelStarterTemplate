/*
 * Gazebo Tutorial Example 01: Basic World Creation
 * 
 * This example demonstrates the fundamental concepts of Gazebo simulation:
 * 1. Loading and running SDF worlds
 * 2. Understanding the simulation loop
 * 3. Basic entity management
 * 4. World state inspection
 * 
 * Learning objectives:
 * - Understand SDF world format
 * - Learn Gazebo's entity-component-system architecture  
 * - Explore basic simulation control
 * - Inspect simulation state and entities
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/common/Console.hh>
#include <gz/common/Filesystem.hh>
#include <gz/transport/Node.hh>

#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>

using namespace gz;
using namespace sim;

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

// Removed BasicWorldInspector class as we're using a simplified simulation approach

// Note: In Gazebo Harmonic, custom system plugins should be properly registered
// through the PluginInfo structure or configured via SDF files. For simplicity,
// this example uses the core simulation without custom systems.

int main(int argc, char **argv)
{
    gzmsg << "Starting Basic World Demo..." << std::endl;

    // Set console verbosity
    gz::common::Console::SetVerbosity(4);

    // Disable config file loading to avoid issues
    // This prevents Gazebo from trying to create/copy config files  
    setenv("GZ_SIM_SERVER_CONFIG_PATH", "", 1);
    setenv("GZ_CONFIG_PATH", "", 1);

    // Configure the server
    ServerConfig serverConfig;
    
    // Use the provided SDF world file or default to basic world
    std::string worldFile = "basic_world_simple.sdf";
    if (argc > 1) {
        worldFile = argv[1];
    }
    
    // Find the actual SDF file path
    std::string resolvedPath = FindSdfFile(worldFile);
    serverConfig.SetSdfFile(resolvedPath);
    serverConfig.SetUpdateRate(100.0);  // 100 Hz update rate

    // Note: Custom system plugins in Gazebo Harmonic should be registered
    // through SDF configuration or using the proper PluginInfo structure
    // For this example, we'll use the basic server without custom plugins

    // Create and start the server
    Server server(serverConfig);

    // Check if server was constructed properly
    gzmsg << "Server constructed, checking if running..." << std::endl;
    
    if (!server.Running())
    {
        gzerr << "Server not running after construction" << std::endl;
        
        // Try to manually start the server
        gzmsg << "Attempting to start server manually..." << std::endl;
        bool started = server.Run();
        if (!started)
        {
            gzerr << "Failed to start server manually" << std::endl;
            return -1;
        }
        
        if (!server.Running())
        {
            gzerr << "Server still not running after manual start" << std::endl;
            return -1;
        }
    }

    gzmsg << "Server started successfully with world: " << resolvedPath << std::endl;
    gzmsg << "Running simulation for 10 seconds..." << std::endl;

    // Run simulation for a specific duration
    const auto maxIterations = 1000;  // 10 seconds at 100 Hz
    size_t iterations = 0;

    // Create transport node for potential communication
    transport::Node node;

    gzmsg << "Running simulation loop..." << std::endl;

    while (server.Running() && iterations < maxIterations)
    {
        // Run one simulation step
        server.RunOnce();
        
        iterations++;
        
        // Print progress every 100 iterations
        if (iterations % 100 == 0) {
            gzmsg << "Simulation step: " << iterations << "/1000" << std::endl;
        }
        
        // Sleep to maintain real-time factor
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    gzmsg << "Simulation completed after " << iterations << " iterations" << std::endl;
    
    // Print final world state
    std::cout << "\n=== Final World State ===" << std::endl;
    std::cout << "Total simulation time: " << iterations * 0.01 << " seconds" << std::endl;

    return 0;
}

/*
 * Key Concepts Demonstrated:
 * 
 * 1. **Server Configuration**: How to set up a Gazebo simulation server
 *    with SDF files and update rates.
 * 
 * 2. **Entity Component System**: Understanding how Gazebo organizes
 *    simulation data using entities, components, and systems.
 * 
 * 3. **Custom Systems**: Creating custom systems to inspect and interact
 *    with the simulation during runtime.
 * 
 * 4. **Simulation Loop**: Managing the simulation execution and timing.
 * 
 * 5. **World Inspection**: Querying entities and their components to
 *    understand the world structure.
 * 
 * Next Steps:
 * - Run this example with different SDF world files
 * - Modify the inspection frequency and observe different behaviors
 * - Experiment with different update rates and their effects
 * - Try creating custom SDF worlds and loading them with this demo
 */