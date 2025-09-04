/*
 * Gazebo Physics Engine Comparison Tool
 * 
 * This example demonstrates:
 * 1. Running simulations with different physics engines (DART, Bullet, TPE)
 * 2. Comparing physics engine performance and behavior
 * 3. Understanding physics engine configuration parameters
 * 4. Analyzing simulation stability and accuracy
 * 5. Real-time switching between physics engines
 * 
 * Learning objectives:
 * - Understanding differences between physics engines
 * - Physics engine configuration and tuning
 * - Performance analysis and benchmarking
 * - Engine-specific features and limitations
 * - Best practices for physics engine selection
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/components/Physics.hh>
#include <gz/sim/components/PhysicsEnginePlugin.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/System.hh>
#include <gz/sim/SystemLoader.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>
#include <gz/common/Profiler.hh>

#include <gz/msgs/world_stats.pb.h>
#include <gz/msgs/entity.pb.h>
#include <gz/msgs/physics.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <vector>
#include <iomanip>

using namespace gz;
using namespace sim;

/**
 * Physics Engine Comparison System
 * Analyzes and compares different physics engine behaviors
 */
class PhysicsComparisonAnalyzer
{
public:
    struct EngineMetrics
    {
        std::string name;
        double avg_step_time = 0.0;
        double max_step_time = 0.0;
        double min_step_time = std::numeric_limits<double>::max();
        int simulation_steps = 0;
        double total_simulation_time = 0.0;
        double real_time_factor = 0.0;
        bool stable = true;
        std::vector<double> step_times;
    };

    PhysicsComparisonAnalyzer() : node_(std::make_shared<transport::Node>())
    {
        // Subscribe to world statistics
        if (!node_->Subscribe("/world/physics_comparison/stats", &PhysicsComparisonAnalyzer::OnWorldStats, this))
        {
            gzerr << "Failed to subscribe to world stats topic\n";
        }

        // Subscribe to physics information
        if (!node_->Subscribe("/world/physics_comparison/physics/info", &PhysicsComparisonAnalyzer::OnPhysicsInfo, this))
        {
            gzerr << "Failed to subscribe to physics info topic\n";
        }

        gzmsg << "Physics Comparison Analyzer initialized\n";
        gzmsg << "Available Physics Engines to compare:\n";
        gzmsg << "  1. DART (Default) - Feature-rich, good for complex robots\n";
        gzmsg << "  2. Bullet - Fast, good for large numbers of objects\n";
        gzmsg << "  3. TPE - Simple, lightweight custom engine\n\n";
    }

    /**
     * Run physics engine comparison with different configurations
     */
    void RunComparison(const std::vector<std::string>& engines_to_test)
    {
        gzmsg << "=== Physics Engine Comparison Started ===\n";
        
        for (const auto& engine : engines_to_test)
        {
            gzmsg << "\nTesting Physics Engine: " << engine << "\n";
            gzmsg << "----------------------------------------\n";
            
            // Create server configuration for this engine
            ServerConfig config = CreateEngineConfig(engine);
            
            // Run simulation with this engine
            RunEngineSimulation(engine, config);
        }
        
        // Print comparison results
        PrintComparisonResults();
    }

private:
    std::shared_ptr<transport::Node> node_;
    std::map<std::string, EngineMetrics> engine_metrics_;
    std::string current_engine_;
    common::Time comparison_start_time_;

    /**
     * Create server configuration for specific physics engine
     */
    ServerConfig CreateEngineConfig(const std::string& engine)
    {
        ServerConfig config;
        
        // Set the physics engine plugin based on the engine name
        if (engine == "DART")
        {
            // DART is the default, create world with DART physics
            config.SetSdfString(CreateWorldSdf("gz-physics-dartsim-plugin"));
        }
        else if (engine == "Bullet") 
        {
            // Bullet physics engine
            config.SetSdfString(CreateWorldSdf("gz-physics-bullet-plugin"));
        }
        else if (engine == "TPE")
        {
            // Trivial Physics Engine
            config.SetSdfString(CreateWorldSdf("gz-physics-tpe-plugin"));
        }
        else
        {
            gzerr << "Unknown physics engine: " << engine << "\n";
            config.SetSdfString(CreateWorldSdf("gz-physics-dartsim-plugin"));
        }

        // Configure simulation parameters
        config.SetUpdateRate(1000.0);  // 1000 Hz update rate
        config.SetUseLevelsExtension(false);
        
        return config;
    }

    /**
     * Create SDF world content for physics testing
     */
    std::string CreateWorldSdf(const std::string& physics_plugin)
    {
        std::string world_sdf = R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="physics_comparison">
    
    <!-- Physics configuration for testing -->
    <physics name="default" type="ode">
      <max_step_size>0.001</max_step_size>
      <real_time_factor>1.0</real_time_factor>
      <real_time_update_rate>1000</real_time_update_rate>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename=")" + physics_plugin + R"(" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    
    <!-- Ground plane -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry><plane><normal>0 0 1</normal><size>100 100</size></plane></geometry>
        </collision>
        <visual name="visual">
          <geometry><plane><normal>0 0 1</normal><size>100 100</size></plane></geometry>
          <material><ambient>0.8 0.8 0.8 1</ambient><diffuse>0.8 0.8 0.8 1</diffuse></material>
        </visual>
      </link>
    </model>
    
    <!-- Test objects for physics comparison -->
    <model name="test_box_1">
      <pose>0 0 5 0 0 0</pose>
      <link name="link">
        <inertial><mass>1.0</mass><inertia><ixx>0.166</ixx><iyy>0.166</iyy><izz>0.166</izz></inertia></inertial>
        <collision name="collision">
          <geometry><box><size>1 1 1</size></box></geometry>
          <surface><friction><ode><mu>1.0</mu></ode></friction><restitution>0.3</restitution></surface>
        </collision>
        <visual name="visual">
          <geometry><box><size>1 1 1</size></box></geometry>
          <material><ambient>1 0 0 1</ambient><diffuse>1 0 0 1</diffuse></material>
        </visual>
      </link>
    </model>
    
    <model name="test_sphere_1">
      <pose>2 0 8 0 0 0</pose>
      <link name="link">
        <inertial><mass>0.5</mass><inertia><ixx>0.1</ixx><iyy>0.1</iyy><izz>0.1</izz></inertia></inertial>
        <collision name="collision">
          <geometry><sphere><radius>0.5</radius></sphere></geometry>
          <surface><friction><ode><mu>1.0</mu></ode></friction><restitution>0.8</restitution></surface>
        </collision>
        <visual name="visual">
          <geometry><sphere><radius>0.5</radius></sphere></geometry>
          <material><ambient>0 1 0 1</ambient><diffuse>0 1 0 1</diffuse></material>
        </visual>
      </link>
    </model>
    
    <model name="test_cylinder_1">
      <pose>-2 0 6 0 0 0</pose>
      <link name="link">
        <inertial><mass>2.0</mass><inertia><ixx>0.5</ixx><iyy>0.5</iyy><izz>0.25</izz></inertia></inertial>
        <collision name="collision">
          <geometry><cylinder><radius>0.5</radius><length>2</length></cylinder></geometry>
          <surface><friction><ode><mu>1.0</mu></ode></friction><restitution>0.1</restitution></surface>
        </collision>
        <visual name="visual">
          <geometry><cylinder><radius>0.5</radius><length>2</length></cylinder></geometry>
          <material><ambient>0 0 1 1</ambient><diffuse>0 0 1 1</diffuse></material>
        </visual>
      </link>
    </model>
    
  </world>
</sdf>
        )";
        
        return world_sdf;
    }

    /**
     * Run simulation for specific physics engine and collect metrics
     */
    void RunEngineSimulation(const std::string& engine, const ServerConfig& config)
    {
        current_engine_ = engine;
        
        // Initialize metrics for this engine
        EngineMetrics metrics;
        metrics.name = engine;
        engine_metrics_[engine] = metrics;
        
        // Create and start server
        Server server(config);
        
        gzmsg << "Starting simulation with " << engine << " physics engine...\n";
        comparison_start_time_ = common::Time::SystemTime();
        
        // Run simulation for specified duration
        const int simulation_steps = 5000;  // 5 seconds at 1000 Hz
        const auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < simulation_steps; ++i)
        {
            auto step_start = std::chrono::high_resolution_clock::now();
            
            // Run one simulation step
            server.RunOnce(false);
            
            auto step_end = std::chrono::high_resolution_clock::now();
            double step_time = std::chrono::duration<double, std::milli>(step_end - step_start).count();
            
            // Record step timing
            engine_metrics_[engine].step_times.push_back(step_time);
            engine_metrics_[engine].simulation_steps++;
            
            // Update min/max step times
            if (step_time < engine_metrics_[engine].min_step_time)
                engine_metrics_[engine].min_step_time = step_time;
            if (step_time > engine_metrics_[engine].max_step_time)
                engine_metrics_[engine].max_step_time = step_time;
                
            // Check for simulation instability (very large step times)
            if (step_time > 100.0)  // 100ms is considered unstable
            {
                engine_metrics_[engine].stable = false;
            }
            
            // Brief pause to prevent overwhelming the system
            if (i % 100 == 0)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                gzmsg << "Progress: " << (i * 100 / simulation_steps) << "%\r";
                std::cout.flush();
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double total_time = std::chrono::duration<double>(end_time - start_time).count();
        
        // Calculate final metrics
        engine_metrics_[engine].total_simulation_time = total_time;
        engine_metrics_[engine].real_time_factor = (simulation_steps * 0.001) / total_time; // sim_time / real_time
        
        // Calculate average step time
        double sum = 0.0;
        for (double time : engine_metrics_[engine].step_times)
        {
            sum += time;
        }
        engine_metrics_[engine].avg_step_time = sum / engine_metrics_[engine].step_times.size();
        
        gzmsg << "\nCompleted simulation with " << engine << " physics engine\n";
        gzmsg << "Simulation steps: " << simulation_steps << "\n";
        gzmsg << "Total time: " << std::fixed << std::setprecision(3) << total_time << " seconds\n";
        gzmsg << "Real-time factor: " << std::fixed << std::setprecision(2) << engine_metrics_[engine].real_time_factor << "x\n\n";
        
        // Allow some time for cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    /**
     * Print comprehensive comparison results
     */
    void PrintComparisonResults()
    {
        gzmsg << "\n=== PHYSICS ENGINE COMPARISON RESULTS ===\n";
        gzmsg << "\nPerformance Summary:\n";
        gzmsg << "---------------------------------------------------\n";
        gzmsg << std::left << std::setw(10) << "Engine"
              << std::setw(15) << "Avg Step (ms)"
              << std::setw(15) << "Max Step (ms)" 
              << std::setw(15) << "RTF"
              << std::setw(10) << "Stable" << "\n";
        gzmsg << "---------------------------------------------------\n";

        // Sort engines by performance (real-time factor)
        std::vector<std::pair<std::string, double>> sorted_engines;
        for (const auto& pair : engine_metrics_)
        {
            sorted_engines.push_back({pair.first, pair.second.real_time_factor});
        }
        std::sort(sorted_engines.begin(), sorted_engines.end(), 
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        for (const auto& engine_perf : sorted_engines)
        {
            const std::string& engine = engine_perf.first;
            const EngineMetrics& metrics = engine_metrics_.at(engine);
            
            gzmsg << std::left << std::setw(10) << engine
                  << std::setw(15) << std::fixed << std::setprecision(3) << metrics.avg_step_time
                  << std::setw(15) << std::fixed << std::setprecision(3) << metrics.max_step_time  
                  << std::setw(15) << std::fixed << std::setprecision(2) << metrics.real_time_factor
                  << std::setw(10) << (metrics.stable ? "Yes" : "No") << "\n";
        }

        gzmsg << "\nDetailed Analysis:\n";
        gzmsg << "==================\n";
        
        for (const auto& engine_perf : sorted_engines)
        {
            const std::string& engine = engine_perf.first;
            const EngineMetrics& metrics = engine_metrics_.at(engine);
            
            gzmsg << "\n" << engine << " Physics Engine:\n";
            gzmsg << "  - Performance: " << GetPerformanceRating(metrics.real_time_factor) << "\n";
            gzmsg << "  - Stability: " << (metrics.stable ? "Stable" : "Unstable") << "\n";
            gzmsg << "  - Step time range: " << std::fixed << std::setprecision(3) 
                  << metrics.min_step_time << " - " << metrics.max_step_time << " ms\n";
            gzmsg << "  - Total simulation time: " << std::fixed << std::setprecision(3) 
                  << metrics.total_simulation_time << " seconds\n";
            gzmsg << "  - Recommendation: " << GetEngineRecommendation(engine, metrics) << "\n";
        }

        gzmsg << "\n=== COMPARISON COMPLETE ===\n";
    }

    /**
     * Get performance rating based on real-time factor
     */
    std::string GetPerformanceRating(double rtf)
    {
        if (rtf >= 1.0) return "Excellent (Real-time or faster)";
        else if (rtf >= 0.8) return "Good (Near real-time)";
        else if (rtf >= 0.5) return "Fair (Moderate performance)";
        else return "Poor (Slow performance)";
    }

    /**
     * Get engine-specific recommendations
     */
    std::string GetEngineRecommendation(const std::string& engine, const EngineMetrics& metrics)
    {
        if (engine == "DART")
        {
            return "Best for complex robots, accurate simulations, advanced joints";
        }
        else if (engine == "Bullet")
        {
            return "Best for many objects, fast collisions, real-time applications";
        }
        else if (engine == "TPE")
        {
            return "Best for simple simulations, educational purposes, minimal overhead";
        }
        else
        {
            return "Engine-specific recommendations not available";
        }
    }

    /**
     * Callback for world statistics updates
     */
    void OnWorldStats(const msgs::WorldStatistics& _msg)
    {
        // This callback receives performance statistics from the simulation
        // We could use this for additional analysis if needed
    }

    /**
     * Callback for physics information updates  
     */
    void OnPhysicsInfo(const msgs::Physics& _msg)
    {
        // This callback receives physics engine configuration information
        // Could be used for advanced analysis of engine parameters
    }
};

/**
 * Main function - Physics Engine Comparison Tool
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);  // Info level
    
    gzmsg << "=== Gazebo Physics Engine Comparison Tool ===\n";
    gzmsg << "This tool compares the performance and behavior of different physics engines.\n\n";

    // Parse command line arguments
    std::vector<std::string> engines_to_test = {"DART", "Bullet", "TPE"};
    
    if (argc > 1)
    {
        engines_to_test.clear();
        for (int i = 1; i < argc; ++i)
        {
            engines_to_test.push_back(argv[i]);
        }
    }

    try
    {
        PhysicsComparisonAnalyzer analyzer;
        analyzer.RunComparison(engines_to_test);
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during physics comparison: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nPhysics engine comparison completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- DART: Feature-rich, best for complex robots and accurate simulation\n";
    gzmsg << "- Bullet: Fast performance, good for many objects and real-time needs\n";
    gzmsg << "- TPE: Simple and lightweight, suitable for basic simulations\n";
    gzmsg << "\nChoose the engine that best fits your simulation requirements.\n";
    
    return 0;
}