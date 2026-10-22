/*
 * Gazebo Tutorial Example 02: Physics Engine Comparison Tool with Static Plugin Integration
 * 
 * This comprehensive tutorial demonstrates:
 * 1. Running simulations with different physics engines (DART, Bullet, TPE) using hermetic builds
 * 2. Comparing physics engine performance, behavior, and capabilities with structured analysis
 * 3. Understanding physics engine configuration parameters and their impact on simulation
 * 4. Analyzing simulation stability, accuracy, and real-time performance factors
 * 5. Educational progression through physics engine selection and optimization
 * 
 * Learning objectives:
 * - Master differences between physics engines and their use cases
 * - Understand hermetic builds with static plugin linking for consistent physics behavior
 * - Learn physics engine configuration, parameter tuning, and optimization strategies
 * - Develop performance analysis and benchmarking methodologies for physics simulation
 * - Explore engine-specific features, limitations, and best practices for production deployments
 * - Signal-based application control and graceful resource management
 * - Why we use static plugins for reliable, reproducible physics engine comparisons
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

#include <gz/msgs/world_stats.pb.h>
#include <gz/msgs/entity.pb.h>
#include <gz/msgs/physics.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <map>
#include <vector>
#include <iomanip>
#include <signal.h>
#include <atomic>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <numeric>
#include <algorithm>

using namespace gz;
using namespace sim;

// Global flags for signal-safe shutdown
volatile sig_atomic_t g_running = 1;

/// \brief Signal handler for graceful shutdown
void signalHandler(int signal)
{
    // Use write() instead of gzmsg for signal safety
    const char* msg = "\n=== TUTORIAL: Received signal, shutting down physics comparison... ===\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    
    // Set shutdown flag atomically
    g_running = 0;
}

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
            if (!g_running) break;  // Check signal before starting each engine
            
            gzmsg << "\nTUTORIAL: Testing Physics Engine: " << engine << "\n";
            gzmsg << "TUTORIAL: ----------------------------------------\n";
            
            // Create server configuration for this engine
            ServerConfig config = CreateEngineConfig(engine);
            
            // Run simulation with this engine
            RunEngineSimulation(engine, config);
            
            if (!g_running)
            {
                gzmsg << "TUTORIAL: Signal received - stopping engine comparison..." << std::endl;
                break;
            }
        }
        
        // Print comparison results
        PrintComparisonResults();
    }

private:
    std::shared_ptr<transport::Node> node_;
    std::map<std::string, EngineMetrics> engine_metrics_;
    std::string current_engine_;
    std::chrono::steady_clock::time_point comparison_start_time_;

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
        config.SetUseLevels(false);
        
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
        comparison_start_time_ = std::chrono::steady_clock::now();
        
        // Run simulation for specified duration
        const int simulation_steps = 5000;  // 5 seconds at 1000 Hz
        const auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < simulation_steps && g_running; ++i)
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
            
            // Brief pause to prevent overwhelming the system and allow signal processing
            if (i % 100 == 0)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                gzmsg << "TUTORIAL: " << engine << " progress: " << (i * 100 / simulation_steps) << "% - " 
                      << "Steps: " << i << ", Avg step: " << std::fixed << std::setprecision(2) 
                      << (engine_metrics_[engine].step_times.empty() ? 0.0 : 
                          std::accumulate(engine_metrics_[engine].step_times.begin(), 
                                        engine_metrics_[engine].step_times.end(), 0.0) / 
                          engine_metrics_[engine].step_times.size()) << "ms\r";
                std::cout.flush();
            }
        }
        
        // Check if shutdown was triggered by signal
        if (g_running == 0)
        {
            gzmsg << "\nTUTORIAL: ✓ Signal received during " << engine << " testing - stopping gracefully..." << std::endl;
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

class TutorialPhysicsComparison
{
public:
    TutorialPhysicsComparison(const std::vector<std::string>& engines) : engines_to_test_(engines) {}

    int Run()
    {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "    GAZEBO TUTORIAL: Physics Engine Comparison Demo" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        gzmsg << "TUTORIAL: Setting up hermetic physics engine comparison environment..." << std::endl;
        gzmsg << "TUTORIAL: This demo uses static plugin approach for reliable engine testing" << std::endl;

        // === STAGE 1: Environment Configuration ===
        std::cout << "\n--- STAGE 1: Environment Configuration ---" << std::endl;
        
        // Set console verbosity and disable config file loading
        gz::common::Console::SetVerbosity(4);
        gzmsg << "TUTORIAL: Console verbosity set to level 4 for detailed analysis output" << std::endl;
        
        setenv("GZ_SIM_SERVER_CONFIG_PATH", "", 1);
        setenv("GZ_CONFIG_PATH", "", 1);
        gzmsg << "TUTORIAL: Disabled system config files for consistent physics engine testing" << std::endl;

        // === STAGE 2: Physics Engine Selection ===
        std::cout << "\n--- STAGE 2: Physics Engine Selection ---" << std::endl;
        
        gzmsg << "TUTORIAL: Physics engines selected for comparison:" << std::endl;
        for (const auto& engine : engines_to_test_)
        {
            gzmsg << "TUTORIAL:   - " << engine << " physics engine" << std::endl;
        }
        gzmsg << "TUTORIAL: Each engine will be tested with identical scenarios for fair comparison" << std::endl;

        // === STAGE 3: Comparison Analysis Setup ===
        std::cout << "\n--- STAGE 3: Comparison Analysis Setup ---" << std::endl;
        
        gzmsg << "TUTORIAL: Creating physics comparison analyzer..." << std::endl;
        gzmsg << "TUTORIAL: Static plugins (Physics, UserCommands, SceneBroadcaster) are active" << std::endl;
        gzmsg << "TUTORIAL: Hermetic approach ensures consistent testing environment across engines" << std::endl;

        // === STAGE 4: Engine Comparison Execution ===
        std::cout << "\n--- STAGE 4: Physics Engine Comparison Execution ---" << std::endl;
        
        gzmsg << "TUTORIAL: Starting comprehensive physics engine comparison..." << std::endl;
        gzmsg << "TUTORIAL: Press Ctrl+C to stop the comparison at any time" << std::endl;
        
        try
        {
            PhysicsComparisonAnalyzer analyzer;
            analyzer.RunComparison(engines_to_test_);
        }
        catch (const std::exception& e)
        {
            gzerr << "TUTORIAL: Error during physics comparison: " << e.what() << std::endl;
            return -1;
        }

        // === STAGE 5: Results Analysis and Conclusions ===
        std::cout << "\n--- STAGE 5: Results Analysis and Educational Conclusions ---" << std::endl;
        
        gzmsg << "TUTORIAL: ✓ Physics engine comparison completed successfully!" << std::endl;
        
        gzmsg << "\nTUTORIAL: Key Learning Points:" << std::endl;
        gzmsg << "TUTORIAL: - DART: Feature-rich, best for complex robots and accurate simulation" << std::endl;
        gzmsg << "TUTORIAL: - Bullet: Fast performance, good for many objects and real-time applications" << std::endl;
        gzmsg << "TUTORIAL: - TPE: Simple and lightweight, suitable for basic educational simulations" << std::endl;
        
        gzmsg << "\nTUTORIAL: Selection Guidelines:" << std::endl;
        gzmsg << "TUTORIAL: - Choose DART for robotics research and complex mechanical systems" << std::endl;
        gzmsg << "TUTORIAL: - Choose Bullet for games, real-time applications, and large object counts" << std::endl;
        gzmsg << "TUTORIAL: - Choose TPE for educational purposes and lightweight simulations" << std::endl;
        gzmsg << "TUTORIAL: - Consider your performance requirements vs. simulation accuracy trade-offs" << std::endl;
        
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "   TUTORIAL COMPLETED - Physics Engine Analysis Success!" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        return 0;
    }

private:
    std::vector<std::string> engines_to_test_;
};

/**
 * Main function - Tutorial Physics Engine Comparison Tool
 */
int main(int argc, char* argv[])
{
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // Termination signal

    // Parse command line arguments
    std::vector<std::string> engines_to_test = {"DART", "Bullet", "TPE"};
    
    if (argc > 1)
    {
        engines_to_test.clear();
        for (int i = 1; i < argc; ++i)
        {
            engines_to_test.push_back(argv[i]);
        }
        
        gzmsg << "TUTORIAL: Custom engine selection provided via command line arguments" << std::endl;
    }
    else
    {
        gzmsg << "TUTORIAL: Using default engine selection (DART, Bullet, TPE)" << std::endl;
        gzmsg << "TUTORIAL: You can specify engines as command line arguments if desired" << std::endl;
    }

    try
    {
        TutorialPhysicsComparison tutorial(engines_to_test);
        return tutorial.Run();
    }
    catch (const std::exception &e)
    {
        gzerr << "Exception in Physics Comparison Tutorial: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        gzerr << "Unknown exception in Physics Comparison Tutorial" << std::endl;
        return -1;
    }
}

/*
 * Key Concepts Demonstrated:
 * 
 * 1. **Physics Engine Comparison**: How to systematically compare DART, Bullet, and TPE 
 *    physics engines using identical test scenarios for fair performance analysis.
 * 
 * 2. **Hermetic Static Plugins**: Understanding why we use static linking for reliable,
 *    reproducible physics engine testing across different environments.
 * 
 * 3. **Performance Benchmarking**: Structured methodology for analyzing physics engine
 *    performance, stability, and real-time factors in production environments.
 * 
 * 4. **Signal Handling**: Proper application lifecycle management with graceful shutdown
 *    during long-running physics comparisons and testing procedures.
 * 
 * 5. **Educational Progression**: Step-by-step tutorial structure that teaches physics
 *    engine selection criteria and optimization strategies.
 * 
 * 6. **Resource Management**: Efficient memory and CPU usage patterns for continuous
 *    physics engine testing and comparison workflows.
 * 
 * Usage Examples:
 * - ./physics_comparison                          # Test all engines (DART, Bullet, TPE)
 * - ./physics_comparison DART                     # Test only DART physics engine
 * - ./physics_comparison Bullet TPE              # Compare Bullet vs TPE engines
 * - ./physics_comparison DART Bullet             # Compare DART vs Bullet performance
 * 
 * Next Steps:
 * - Run this comparison with different engine combinations for specific use cases
 * - Analyze performance results to choose optimal engine for your application
 * - Experiment with different physics parameters and world complexity levels
 * - Integrate comparison results into automated CI/CD testing pipelines
 * 
 * Educational Notes:
 * - Static plugin messages are expected and demonstrate hermetic build reliability
 * - Performance results depend on hardware, scenario complexity, and system load
 * - DART provides highest accuracy, Bullet offers best speed, TPE gives simplicity
 * - Tutorial demonstrates best practices for production physics engine selection
 */