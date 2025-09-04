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
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/SystemLoader.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Model.hh>
#include <gz/sim/components/World.hh>
#include <gz/common/Console.hh>
#include <gz/transport/Node.hh>

#include <iostream>
#include <chrono>
#include <thread>

using namespace gz;
using namespace sim;

class BasicWorldInspector
{
public:
    BasicWorldInspector(EntityComponentManager &_ecm) : ecm(_ecm) {}

    /// \brief Print all entities in the world
    void PrintAllEntities()
    {
        std::cout << "\n=== World Entities ===" << std::endl;
        
        // Find the world entity
        auto worldEntity = ecm.EntityByComponents(components::World());
        if (worldEntity != kNullEntity)
        {
            auto worldName = ecm.Component<components::Name>(worldEntity);
            std::cout << "World: " << worldName->Data() << " (Entity: " << worldEntity << ")" << std::endl;
        }

        // Find all model entities
        ecm.Each<components::Model, components::Name>(
            [&](const Entity &entity, const components::Model *, const components::Name *name) -> bool
            {
                std::cout << "  Model: " << name->Data() << " (Entity: " << entity << ")" << std::endl;
                return true;
            });
    }

    /// \brief Print simulation statistics
    void PrintSimStats()
    {
        std::cout << "\n=== Simulation Statistics ===" << std::endl;
        std::cout << "Total entities: " << ecm.EntityCount() << std::endl;
        // Additional stats can be added here
    }

private:
    EntityComponentManager &ecm;
};

/// \brief Custom system to inspect the world during simulation
class WorldInspectorSystem : public System, public ISystemPostUpdate
{
public:
    WorldInspectorSystem() : inspectionCount(0) {}

    void PostUpdate(const UpdateInfo &_info, const EntityComponentManager &_ecm) override
    {
        // Cast away const to allow inspection (read-only operations)
        auto &ecm = const_cast<EntityComponentManager&>(_ecm);
        BasicWorldInspector inspector(ecm);

        // Print world information every 100 iterations
        if (++inspectionCount % 100 == 0)
        {
            std::cout << "\n--- Simulation Step: " << inspectionCount << " ---" << std::endl;
            std::cout << "Simulation Time: " << std::chrono::duration<double>(_info.simTime).count() << "s" << std::endl;
            inspector.PrintAllEntities();
            inspector.PrintSimStats();
        }
    }

private:
    size_t inspectionCount;
};

int main(int argc, char **argv)
{
    gzmsg << "Starting Basic World Demo..." << std::endl;

    // Configure the server
    ServerConfig serverConfig;
    
    // Use the provided SDF world file or default to basic world
    std::string worldFile = "worlds/basic_world.sdf";
    if (argc > 1) {
        worldFile = argv[1];
    }
    
    serverConfig.SetSdfFile(worldFile);
    serverConfig.SetUpdateRate(100.0);  // 100 Hz update rate

    // Add our custom inspection system
    serverConfig.AddPlugin("", "", 
                          std::make_shared<WorldInspectorSystem>());

    // Create and start the server
    Server server(serverConfig);

    if (!server.Running())
    {
        gzerr << "Failed to start server" << std::endl;
        return -1;
    }

    gzmsg << "Server started successfully with world: " << worldFile << std::endl;
    gzmsg << "Running simulation for 10 seconds..." << std::endl;

    // Run simulation for a specific duration
    const auto maxIterations = 1000;  // 10 seconds at 100 Hz
    size_t iterations = 0;

    // Create transport node for potential communication
    transport::Node node;

    while (server.Running() && iterations < maxIterations)
    {
        // Run one simulation step
        server.RunOnce();
        
        iterations++;
        
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