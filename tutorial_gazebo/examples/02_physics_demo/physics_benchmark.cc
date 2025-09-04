/*
 * Gazebo Physics Performance Benchmark Tool
 * 
 * This example demonstrates:
 * 1. Physics engine performance comparison under load
 * 2. Scalability testing with increasing object counts
 * 3. Complex scenario benchmarking
 * 4. Memory usage and computational efficiency analysis
 * 5. Real-time performance optimization techniques
 * 
 * Learning objectives:
 * - Understanding physics engine performance characteristics
 * - Benchmarking methodology for simulation performance
 * - Performance optimization strategies
 * - Resource usage monitoring and analysis
 * - Choosing appropriate physics settings for different scenarios
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>
#include <gz/common/Profiler.hh>

#include <gz/msgs/world_stats.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>
#include <gz/math/Rand.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef __linux__
#include <sys/resource.h>
#include <unistd.h>
#endif

using namespace gz;
using namespace sim;

/**
 * Physics Performance Benchmark System
 * Comprehensive benchmarking suite for physics engine performance
 */
class PhysicsBenchmark
{
public:
    struct BenchmarkResult
    {
        std::string test_name;
        std::string physics_engine;
        int object_count = 0;
        double avg_step_time = 0.0;
        double min_step_time = 0.0;
        double max_step_time = 0.0;
        double real_time_factor = 0.0;
        double cpu_usage = 0.0;
        size_t memory_usage = 0;
        bool completed_successfully = false;
        std::vector<double> step_times;
    };

    PhysicsBenchmark() : node_(std::make_shared<transport::Node>())
    {
        // Subscribe to world statistics
        if (!node_->Subscribe("/world/benchmark/stats", &PhysicsBenchmark::OnWorldStats, this))
        {
            gzerr << "Failed to subscribe to world stats topic\n";
        }

        gzmsg << "Physics Performance Benchmark Tool initialized\n";
        gzmsg << "Preparing comprehensive performance analysis...\n\n";
    }

    /**
     * Run comprehensive physics benchmarks
     */
    void RunBenchmarks()
    {
        gzmsg << "=== Physics Performance Benchmark Suite ===\n\n";
        
        // Define benchmark scenarios
        std::vector<int> object_counts = {10, 50, 100, 250, 500};
        std::vector<std::string> engines = {"DART", "Bullet", "TPE"};
        std::vector<std::string> scenarios = {"falling_objects", "collision_test", "joint_stress"};
        
        // Run benchmarks for each combination
        for (const auto& engine : engines)
        {
            gzmsg << "\n" << std::string(60, '=') << "\n";
            gzmsg << "BENCHMARKING: " << engine << " PHYSICS ENGINE\n";
            gzmsg << std::string(60, '=') << "\n";
            
            for (const auto& scenario : scenarios)
            {
                gzmsg << "\nScenario: " << scenario << "\n";
                gzmsg << std::string(40, '-') << "\n";
                
                for (int count : object_counts)
                {
                    gzmsg << "Testing with " << count << " objects...\n";
                    
                    BenchmarkResult result = RunSingleBenchmark(engine, scenario, count);
                    results_.push_back(result);
                    
                    PrintBenchmarkResult(result);
                    
                    // Brief pause between tests
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }
            }
        }
        
        // Print comprehensive analysis
        PrintComprehensiveAnalysis();
    }

private:
    std::shared_ptr<transport::Node> node_;
    std::vector<BenchmarkResult> results_;
    common::Time last_stats_time_;

    /**
     * Run a single benchmark test
     */
    BenchmarkResult RunSingleBenchmark(const std::string& engine, const std::string& scenario, int object_count)
    {
        BenchmarkResult result;
        result.test_name = scenario;
        result.physics_engine = engine;
        result.object_count = object_count;
        
        try
        {
            // Create server configuration
            ServerConfig config = CreateBenchmarkConfig(engine, scenario, object_count);
            Server server(config);
            
            // Initialize timing
            auto start_time = std::chrono::high_resolution_clock::now();
            const int benchmark_steps = 3000;  // 3 seconds at 1000 Hz
            
            // Record initial memory usage
            result.memory_usage = GetCurrentMemoryUsage();
            
            // Run benchmark
            for (int i = 0; i < benchmark_steps; ++i)
            {
                auto step_start = std::chrono::high_resolution_clock::now();
                
                server.RunOnce(false);
                
                auto step_end = std::chrono::high_resolution_clock::now();
                double step_time = std::chrono::duration<double, std::milli>(step_end - step_start).count();
                
                result.step_times.push_back(step_time);
                
                // Brief yield to prevent system overload
                if (i % 100 == 0)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            double total_time = std::chrono::duration<double>(end_time - start_time).count();
            
            // Calculate metrics
            CalculateBenchmarkMetrics(result, total_time, benchmark_steps);
            result.completed_successfully = true;
        }
        catch (const std::exception& e)
        {
            gzerr << "Benchmark failed: " << e.what() << "\n";
            result.completed_successfully = false;
        }
        
        return result;
    }

    /**
     * Create benchmark configuration for specific test
     */
    ServerConfig CreateBenchmarkConfig(const std::string& engine, const std::string& scenario, int object_count)
    {
        ServerConfig config;
        config.SetSdfString(CreateBenchmarkWorldSdf(engine, scenario, object_count));
        config.SetUpdateRate(1000.0);
        config.SetUseLevelsExtension(false);
        
        return config;
    }

    /**
     * Generate SDF for benchmark scenarios
     */
    std::string CreateBenchmarkWorldSdf(const std::string& engine, const std::string& scenario, int object_count)
    {
        std::string physics_plugin;
        if (engine == "DART")
            physics_plugin = "gz-physics-dartsim-plugin";
        else if (engine == "Bullet")
            physics_plugin = "gz-physics-bullet-plugin";
        else if (engine == "TPE")
            physics_plugin = "gz-physics-tpe-plugin";
        else
            physics_plugin = "gz-physics-dartsim-plugin";

        std::ostringstream sdf_stream;
        
        sdf_stream << R"(<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="benchmark">
    
    <physics name="default" type="ode">
      <max_step_size>0.001</max_step_size>
      <real_time_factor>1.0</real_time_factor>
      <real_time_update_rate>1000</real_time_update_rate>
    </physics>
    
    <plugin filename=")" << physics_plugin << R"(" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    
    <!-- Ground plane -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry><plane><normal>0 0 1</normal><size>50 50</size></plane></geometry>
        </collision>
        <visual name="visual">
          <geometry><plane><normal>0 0 1</normal><size>50 50</size></plane></geometry>
          <material><ambient>0.8 0.8 0.8 1</ambient></material>
        </visual>
      </link>
    </model>
)";

        // Add objects based on scenario
        if (scenario == "falling_objects")
        {
            AddFallingObjectsScenario(sdf_stream, object_count);
        }
        else if (scenario == "collision_test")
        {
            AddCollisionTestScenario(sdf_stream, object_count);
        }
        else if (scenario == "joint_stress")
        {
            AddJointStressScenario(sdf_stream, object_count);
        }

        sdf_stream << "  </world>\n</sdf>\n";
        return sdf_stream.str();
    }

    /**
     * Add falling objects scenario
     */
    void AddFallingObjectsScenario(std::ostringstream& sdf, int object_count)
    {
        math::Rand::Seed(12345);  // Reproducible random numbers
        
        for (int i = 0; i < object_count; ++i)
        {
            double x = math::Rand::DblUniform(-10, 10);
            double y = math::Rand::DblUniform(-10, 10);
            double z = math::Rand::DblUniform(5, 15);
            
            // Vary object types
            std::string geometry;
            std::string inertia;
            if (i % 3 == 0)  // Box
            {
                geometry = "<box><size>0.5 0.5 0.5</size></box>";
                inertia = "<ixx>0.042</ixx><iyy>0.042</iyy><izz>0.042</izz>";
            }
            else if (i % 3 == 1)  // Sphere
            {
                geometry = "<sphere><radius>0.25</radius></sphere>";
                inertia = "<ixx>0.025</ixx><iyy>0.025</iyy><izz>0.025</izz>";
            }
            else  // Cylinder
            {
                geometry = "<cylinder><radius>0.25</radius><length>0.5</length></cylinder>";
                inertia = "<ixx>0.042</ixx><iyy>0.042</iyy><izz>0.0125</izz>";
            }

            sdf << "    <model name=\"object_" << i << "\">\n";
            sdf << "      <pose>" << x << " " << y << " " << z << " 0 0 0</pose>\n";
            sdf << "      <link name=\"link\">\n";
            sdf << "        <inertial><mass>1.0</mass><inertia>" << inertia << "</inertia></inertial>\n";
            sdf << "        <collision name=\"collision\">\n";
            sdf << "          <geometry>" << geometry << "</geometry>\n";
            sdf << "          <surface><friction><ode><mu>0.8</mu></ode></friction><restitution>0.3</restitution></surface>\n";
            sdf << "        </collision>\n";
            sdf << "        <visual name=\"visual\">\n";
            sdf << "          <geometry>" << geometry << "</geometry>\n";
            sdf << "          <material><ambient>" << (i%3)*0.5 << " " << ((i+1)%3)*0.5 << " " << ((i+2)%3)*0.5 << " 1</ambient></material>\n";
            sdf << "        </visual>\n";
            sdf << "      </link>\n";
            sdf << "    </model>\n";
        }
    }

    /**
     * Add collision test scenario
     */
    void AddCollisionTestScenario(std::ostringstream& sdf, int object_count)
    {
        // Create a wall of objects that will collide
        int objects_per_row = static_cast<int>(std::sqrt(object_count));
        
        for (int i = 0; i < object_count; ++i)
        {
            int row = i / objects_per_row;
            int col = i % objects_per_row;
            
            double x = (col - objects_per_row/2) * 0.6;
            double y = (row - objects_per_row/2) * 0.6;
            double z = 2 + row * 0.6;

            sdf << "    <model name=\"collision_object_" << i << "\">\n";
            sdf << "      <pose>" << x << " " << y << " " << z << " 0 0 0</pose>\n";
            sdf << "      <link name=\"link\">\n";
            sdf << "        <inertial><mass>2.0</mass><inertia><ixx>0.167</ixx><iyy>0.167</iyy><izz>0.167</izz></inertia></inertial>\n";
            sdf << "        <collision name=\"collision\">\n";
            sdf << "          <geometry><box><size>0.5 0.5 0.5</size></box></geometry>\n";
            sdf << "          <surface><friction><ode><mu>1.0</mu></ode></friction><restitution>0.2</restitution></surface>\n";
            sdf << "        </collision>\n";
            sdf << "        <visual name=\"visual\">\n";
            sdf << "          <geometry><box><size>0.5 0.5 0.5</size></box></geometry>\n";
            sdf << "          <material><ambient>1 0 0 1</ambient></material>\n";
            sdf << "        </visual>\n";
            sdf << "      </link>\n";
            sdf << "    </model>\n";
        }
    }

    /**
     * Add joint stress test scenario
     */
    void AddJointStressScenario(std::ostringstream& sdf, int object_count)
    {
        // Create chains of connected objects to stress joint calculations
        int chains = object_count / 10;  // 10 objects per chain
        int objects_per_chain = object_count / chains;
        
        for (int chain = 0; chain < chains; ++chain)
        {
            for (int link = 0; link < objects_per_chain; ++link)
            {
                int object_id = chain * objects_per_chain + link;
                double x = chain * 2 - chains;
                double y = 0;
                double z = 5 + link * 0.6;

                sdf << "    <model name=\"chain_" << chain << "_link_" << link << "\">\n";
                sdf << "      <pose>" << x << " " << y << " " << z << " 0 0 0</pose>\n";
                sdf << "      <link name=\"link\">\n";
                sdf << "        <inertial><mass>0.5</mass><inertia><ixx>0.02</ixx><iyy>0.02</iyy><izz>0.02</izz></inertia></inertial>\n";
                sdf << "        <collision name=\"collision\">\n";
                sdf << "          <geometry><box><size>0.2 0.2 0.5</size></box></geometry>\n";
                sdf << "        </collision>\n";
                sdf << "        <visual name=\"visual\">\n";
                sdf << "          <geometry><box><size>0.2 0.2 0.5</size></box></geometry>\n";
                sdf << "          <material><ambient>0 1 0 1</ambient></material>\n";
                sdf << "        </visual>\n";
                sdf << "      </link>\n";
                sdf << "    </model>\n";

                // Add joint to previous link or world
                if (link > 0)
                {
                    int prev_id = chain * objects_per_chain + (link - 1);
                    sdf << "    <joint name=\"chain_" << chain << "_joint_" << link << "\" type=\"revolute\">\n";
                    sdf << "      <parent>chain_" << chain << "_link_" << (link-1) << "::link</parent>\n";
                    sdf << "      <child>chain_" << chain << "_link_" << link << "::link</child>\n";
                    sdf << "      <axis><xyz>1 0 0</xyz><dynamics><damping>0.1</damping></dynamics></axis>\n";
                    sdf << "    </joint>\n";
                }
                else
                {
                    // First link connects to world
                    sdf << "    <joint name=\"chain_" << chain << "_base\" type=\"revolute\">\n";
                    sdf << "      <parent>world</parent>\n";
                    sdf << "      <child>chain_" << chain << "_link_" << link << "::link</child>\n";
                    sdf << "      <axis><xyz>0 1 0</xyz><dynamics><damping>0.5</damping></dynamics></axis>\n";
                    sdf << "    </joint>\n";
                }
            }
        }
    }

    /**
     * Calculate benchmark metrics from collected data
     */
    void CalculateBenchmarkMetrics(BenchmarkResult& result, double total_time, int steps)
    {
        if (result.step_times.empty()) return;

        // Calculate timing statistics
        double sum = 0.0;
        result.min_step_time = *std::min_element(result.step_times.begin(), result.step_times.end());
        result.max_step_time = *std::max_element(result.step_times.begin(), result.step_times.end());
        
        for (double time : result.step_times)
        {
            sum += time;
        }
        result.avg_step_time = sum / result.step_times.size();
        
        // Calculate real-time factor
        double sim_time = steps * 0.001;  // seconds
        result.real_time_factor = sim_time / total_time;
        
        // Get final memory usage
        result.memory_usage = GetCurrentMemoryUsage();
        
        // CPU usage estimation (simplified)
        result.cpu_usage = std::max(0.0, std::min(100.0, 100.0 / result.real_time_factor - 100.0));
    }

    /**
     * Get current memory usage in bytes
     */
    size_t GetCurrentMemoryUsage()
    {
#ifdef __linux__
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        return usage.ru_maxrss * 1024;  // Convert from KB to bytes
#else
        return 0;  // Not implemented for other platforms
#endif
    }

    /**
     * Print result of a single benchmark
     */
    void PrintBenchmarkResult(const BenchmarkResult& result)
    {
        if (!result.completed_successfully)
        {
            gzmsg << "  FAILED - Could not complete benchmark\n\n";
            return;
        }

        gzmsg << "  Results:\n";
        gzmsg << "    Average step time: " << std::fixed << std::setprecision(3) << result.avg_step_time << " ms\n";
        gzmsg << "    Step time range: " << std::fixed << std::setprecision(3) 
              << result.min_step_time << " - " << result.max_step_time << " ms\n";
        gzmsg << "    Real-time factor: " << std::fixed << std::setprecision(2) << result.real_time_factor << "x\n";
        gzmsg << "    Memory usage: " << (result.memory_usage / 1024 / 1024) << " MB\n";
        gzmsg << "    Performance: " << GetPerformanceRating(result.real_time_factor) << "\n\n";
    }

    /**
     * Get performance rating based on real-time factor
     */
    std::string GetPerformanceRating(double rtf)
    {
        if (rtf >= 1.0) return "Excellent (Real-time or faster)";
        else if (rtf >= 0.8) return "Good (Near real-time)";
        else if (rtf >= 0.5) return "Fair (Moderate performance)";
        else if (rtf >= 0.2) return "Poor (Slow performance)";
        else return "Very Poor (Much slower than real-time)";
    }

    /**
     * Print comprehensive analysis of all benchmark results
     */
    void PrintComprehensiveAnalysis()
    {
        gzmsg << "\n" << std::string(80, '=') << "\n";
        gzmsg << "COMPREHENSIVE BENCHMARK ANALYSIS\n";
        gzmsg << std::string(80, '=') << "\n\n";

        // Performance comparison by engine
        PrintEngineComparison();
        
        // Scalability analysis
        PrintScalabilityAnalysis();
        
        // Scenario analysis
        PrintScenarioAnalysis();
        
        // Recommendations
        PrintRecommendations();
    }

    /**
     * Print engine performance comparison
     */
    void PrintEngineComparison()
    {
        gzmsg << "Engine Performance Comparison:\n";
        gzmsg << std::string(50, '-') << "\n";
        
        std::map<std::string, std::vector<BenchmarkResult*>> engine_results;
        for (auto& result : results_)
        {
            if (result.completed_successfully)
            {
                engine_results[result.physics_engine].push_back(&result);
            }
        }

        for (const auto& pair : engine_results)
        {
            const std::string& engine = pair.first;
            const std::vector<BenchmarkResult*>& results = pair.second;
            
            double avg_rtf = 0.0;
            double avg_memory = 0.0;
            
            for (const auto* result : results)
            {
                avg_rtf += result->real_time_factor;
                avg_memory += result->memory_usage;
            }
            
            avg_rtf /= results.size();
            avg_memory /= results.size();
            
            gzmsg << engine << " Engine:\n";
            gzmsg << "  Average Real-Time Factor: " << std::fixed << std::setprecision(2) << avg_rtf << "x\n";
            gzmsg << "  Average Memory Usage: " << std::fixed << std::setprecision(1) << (avg_memory / 1024 / 1024) << " MB\n";
            gzmsg << "  Overall Rating: " << GetPerformanceRating(avg_rtf) << "\n\n";
        }
    }

    /**
     * Print scalability analysis
     */
    void PrintScalabilityAnalysis()
    {
        gzmsg << "Scalability Analysis:\n";
        gzmsg << std::string(50, '-') << "\n";
        
        // Group results by object count
        std::map<int, std::vector<BenchmarkResult*>> count_results;
        for (auto& result : results_)
        {
            if (result.completed_successfully)
            {
                count_results[result.object_count].push_back(&result);
            }
        }

        gzmsg << "Performance vs Object Count:\n";
        for (const auto& pair : count_results)
        {
            int count = pair.first;
            const std::vector<BenchmarkResult*>& results = pair.second;
            
            double avg_rtf = 0.0;
            for (const auto* result : results)
            {
                avg_rtf += result->real_time_factor;
            }
            avg_rtf /= results.size();
            
            gzmsg << "  " << std::setw(3) << count << " objects: " 
                  << std::fixed << std::setprecision(2) << avg_rtf << "x RTF\n";
        }
        gzmsg << "\n";
    }

    /**
     * Print scenario analysis
     */
    void PrintScenarioAnalysis()
    {
        gzmsg << "Scenario Performance Analysis:\n";
        gzmsg << std::string(50, '-') << "\n";
        
        // Group results by scenario
        std::map<std::string, std::vector<BenchmarkResult*>> scenario_results;
        for (auto& result : results_)
        {
            if (result.completed_successfully)
            {
                scenario_results[result.test_name].push_back(&result);
            }
        }

        for (const auto& pair : scenario_results)
        {
            const std::string& scenario = pair.first;
            const std::vector<BenchmarkResult*>& results = pair.second;
            
            double avg_rtf = 0.0;
            for (const auto* result : results)
            {
                avg_rtf += result->real_time_factor;
            }
            avg_rtf /= results.size();
            
            gzmsg << scenario << ": " << std::fixed << std::setprecision(2) << avg_rtf << "x RTF average\n";
        }
        gzmsg << "\n";
    }

    /**
     * Print recommendations based on benchmark results
     */
    void PrintRecommendations()
    {
        gzmsg << "Performance Recommendations:\n";
        gzmsg << std::string(50, '-') << "\n";
        gzmsg << "Based on benchmark results:\n\n";
        
        gzmsg << "For Real-Time Applications:\n";
        gzmsg << "- Use Bullet engine for many objects (>100)\n";
        gzmsg << "- Use TPE engine for simple scenarios\n";
        gzmsg << "- Reduce physics update rate if needed\n\n";
        
        gzmsg << "For Accuracy-Critical Simulations:\n";
        gzmsg << "- Use DART engine despite lower performance\n";
        gzmsg << "- Accept lower real-time factors for precision\n";
        gzmsg << "- Use smaller time steps for stability\n\n";
        
        gzmsg << "For Large-Scale Simulations:\n";
        gzmsg << "- Prefer Bullet engine for scalability\n";
        gzmsg << "- Consider object culling and LOD techniques\n";
        gzmsg << "- Monitor memory usage carefully\n\n";
        
        gzmsg << "General Optimization Tips:\n";
        gzmsg << "- Use simple collision shapes where possible\n";
        gzmsg << "- Adjust physics parameters for your use case\n";
        gzmsg << "- Balance accuracy vs performance needs\n";
        gzmsg << "- Profile your specific scenarios regularly\n\n";
    }

    /**
     * Callback for world statistics (currently unused but available)
     */
    void OnWorldStats(const msgs::WorldStatistics& _msg)
    {
        // Could be used for additional real-time performance monitoring
    }
};

/**
 * Main function - Physics Performance Benchmark
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Physics Performance Benchmark Tool ===\n";
    gzmsg << "This tool provides comprehensive performance analysis of physics engines.\n\n";
    
    gzmsg << "Benchmark scenarios:\n";
    gzmsg << "- Falling Objects: Basic gravity and collision performance\n";
    gzmsg << "- Collision Test: Intensive collision detection stress test\n";
    gzmsg << "- Joint Stress: Complex multi-body system performance\n\n";
    
    gzmsg << "Testing object counts: 10, 50, 100, 250, 500\n";
    gzmsg << "Physics engines: DART, Bullet, TPE\n\n";
    
    gzmsg << "Note: This benchmark may take several minutes to complete.\n\n";

    try
    {
        PhysicsBenchmark benchmark;
        benchmark.RunBenchmarks();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during physics benchmark: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nPhysics performance benchmark completed successfully!\n";
    gzmsg << "\nUse these results to choose the optimal physics engine and settings\n";
    gzmsg << "for your specific simulation requirements.\n";
    
    return 0;
}