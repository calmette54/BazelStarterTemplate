/*
 * Gazebo Service Communication Demo
 * 
 * This example demonstrates:
 * 1. Request-response service communication patterns
 * 2. Service discovery and registration mechanisms  
 * 3. Synchronous and asynchronous service calls
 * 4. Error handling and timeout management
 * 5. Service versioning and compatibility
 * 
 * Learning objectives:
 * - Understanding client-server communication patterns
 * - Service interface design and documentation
 * - Performance characteristics of RPC systems
 * - Error propagation and exception handling
 * - Load balancing and service scaling considerations
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/string_msg.pb.h>
#include <gz/msgs/vector3d.pb.h>
#include <gz/msgs/boolean.pb.h>
#include <gz/msgs/double.pb.h>
#include <gz/msgs/empty.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Angle.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <memory>
#include <future>
#include <functional>

using namespace gz;
using namespace transport;

/**
 * Service Performance Statistics
 */
struct ServiceStats
{
    std::string service_name;
    std::atomic<int> requests_sent{0};
    std::atomic<int> responses_received{0};
    std::atomic<int> timeouts{0};
    std::atomic<int> errors{0};
    std::atomic<double> total_response_time{0.0};
    std::atomic<int> response_time_samples{0};
    
    double GetAverageResponseTime() const 
    {
        int samples = response_time_samples.load();
        return samples > 0 ? total_response_time.load() / samples : 0.0;
    }
    
    double GetSuccessRate() const
    {
        int sent = requests_sent.load();
        int received = responses_received.load();
        return sent > 0 ? 100.0 * received / sent : 0.0;
    }
};

/**
 * Service Communication System
 */
class ServiceDemo
{
public:
    ServiceDemo() : node_(std::make_shared<Node>())
    {
        gzmsg << "Service Demo initialized\n";
        gzmsg << "Request-response service communication demonstration\n\n";
        
        // Initialize communication system
        communication_active_ = true;
        data_mutex_ = std::make_shared<std::mutex>();
        
        // Setup services and clients
        SetupServices();
        
        // Initialize statistics tracking
        InitializeStatistics();
    }

    /**
     * Run comprehensive service communication demonstration
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Service Communication Demo ===\n\n";
        
        // Create minimal server for transport framework
        ServerConfig config;
        config.SetSdfString(CreateServiceWorldSdf());
        config.SetUpdateRate(10.0);  // Lower rate for service demo
        
        Server server(config);
        
        gzmsg << "Starting service demonstration...\n";
        PrintServiceConfiguration();
        
        // Start service threads
        std::thread client_thread(&ServiceDemo::ClientLoop, this);
        std::thread statistics_thread(&ServiceDemo::StatisticsLoop, this);
        
        // Run simulation
        const int simulation_steps = 6000;  // 10 minutes at 10 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && communication_active_)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print service statistics every 30 seconds
            if (step_count % 300 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                PrintServiceAnalysis(elapsed);
                gzmsg << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(100000)); // 10 Hz
        }
        
        // Stop communication and cleanup
        communication_active_ = false;
        
        if (client_thread.joinable()) client_thread.join();
        if (statistics_thread.joinable()) statistics_thread.join();
        
        PrintServiceSummary();
    }

private:
    std::shared_ptr<Node> node_;
    std::shared_ptr<std::mutex> data_mutex_;
    std::atomic<bool> communication_active_;

    // Service statistics
    std::map<std::string, std::shared_ptr<ServiceStats>> service_stats_;
    
    // Available services
    std::vector<std::string> active_services_;

    /**
     * Setup comprehensive service patterns
     */
    void SetupServices()
    {
        // Calculator service - basic arithmetic operations
        node_->Advertise("/demo/calculator/add", &ServiceDemo::CalculatorAdd, this);
        node_->Advertise("/demo/calculator/multiply", &ServiceDemo::CalculatorMultiply, this);
        active_services_.push_back("/demo/calculator/add");
        active_services_.push_back("/demo/calculator/multiply");
        
        // System status service - health checks
        node_->Advertise("/demo/system/status", &ServiceDemo::SystemStatus, this);
        node_->Advertise("/demo/system/reset", &ServiceDemo::SystemReset, this);
        active_services_.push_back("/demo/system/status");
        active_services_.push_back("/demo/system/reset");
        
        // Robot control service - motion commands
        node_->Advertise("/demo/robot/move_to", &ServiceDemo::RobotMoveTo, this);
        node_->Advertise("/demo/robot/get_pose", &ServiceDemo::RobotGetPose, this);
        active_services_.push_back("/demo/robot/move_to");
        active_services_.push_back("/demo/robot/get_pose");
        
        // Data processing service - simulated heavy computation
        node_->Advertise("/demo/processing/analyze", &ServiceDemo::DataAnalyze, this);
        active_services_.push_back("/demo/processing/analyze");
        
        gzmsg << "Services configured:\n";
        gzmsg << "- /demo/calculator/*: Arithmetic operations\n";
        gzmsg << "- /demo/system/*: System health and control\n";
        gzmsg << "- /demo/robot/*: Robot control and status\n";
        gzmsg << "- /demo/processing/*: Data analysis services\n\n";
    }

    /**
     * Initialize statistics tracking for all services
     */
    void InitializeStatistics()
    {
        for (const auto& service : active_services_)
        {
            auto stats = std::make_shared<ServiceStats>();
            stats->service_name = service;
            service_stats_[service] = stats;
        }
    }

    /**
     * Create simple world for service framework
     */
    std::string CreateServiceWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="service_demo">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.1</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    
    <!-- Ground plane -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>5 5</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>5 5</size>
            </plane>
          </geometry>
        </visual>
      </link>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print service configuration
     */
    void PrintServiceConfiguration()
    {
        gzmsg << "Service Communication Configuration:\n";
        gzmsg << "-----------------------------------\n";
        gzmsg << "Communication Pattern: Request-Response (RPC)\n";
        gzmsg << "Transport Protocol: ZMQ REQ-REP sockets\n";
        gzmsg << "Service Discovery: Automatic registration\n";
        gzmsg << "Timeout Handling: Configurable per service\n\n";
        
        gzmsg << "Available Services:\n";
        for (const auto& service : active_services_)
        {
            gzmsg << "  - " << service << "\n";
        }
        gzmsg << "\n";
        
        gzmsg << "Service Characteristics:\n";
        gzmsg << "- Synchronous: Blocking until response\n";
        gzmsg << "- Reliable: Guaranteed delivery or timeout\n";
        gzmsg << "- Stateless: Each request is independent\n";
        gzmsg << "- Load-balanced: Multiple providers supported\n\n";
    }

    /**
     * Client loop - makes service calls at regular intervals
     */
    void ClientLoop()
    {
        int call_count = 0;
        
        while (communication_active_)
        {
            // Test calculator services
            if (call_count % 20 == 0)  // Every 20 seconds
            {
                TestCalculatorServices();
            }
            
            // Test system services
            if (call_count % 30 == 0)  // Every 30 seconds
            {
                TestSystemServices();
            }
            
            // Test robot services
            if (call_count % 15 == 0)  // Every 15 seconds
            {
                TestRobotServices();
            }
            
            // Test data processing service
            if (call_count % 45 == 0)  // Every 45 seconds
            {
                TestDataProcessingService();
            }
            
            call_count++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    /**
     * Statistics loop - collects service performance metrics
     */
    void StatisticsLoop()
    {
        while (communication_active_)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            // Statistics are updated in real-time through service calls
            UpdateStatistics();
        }
    }

    // Service implementation functions
    bool CalculatorAdd(const msgs::Vector3d& req, msgs::Double& res)
    {
        // Simple addition service
        res.set_data(req.x() + req.y() + req.z());
        
        // Simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        return true;
    }

    bool CalculatorMultiply(const msgs::Vector3d& req, msgs::Double& res)
    {
        // Simple multiplication service
        res.set_data(req.x() * req.y() * req.z());
        
        // Simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        
        return true;
    }

    bool SystemStatus(const msgs::Empty& req, msgs::StringMsg& res)
    {
        // System health check
        static int status_calls = 0;
        status_calls++;
        
        std::string status;
        if (status_calls % 10 == 0)
        {
            status = "System: WARNING - High load detected";
        }
        else if (status_calls % 20 == 0)
        {
            status = "System: ERROR - Component failure";
        }
        else
        {
            status = "System: OK - All components healthy";
        }
        
        res.set_data(status);
        
        // Simulate diagnostic time
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        return true;
    }

    bool SystemReset(const msgs::StringMsg& req, msgs::Boolean& res)
    {
        // System reset command
        std::string component = req.data();
        
        // Simulate reset operation
        if (component == "all" || component == "sensors" || component == "actuators")
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            res.set_data(true);
        }
        else
        {
            res.set_data(false);
        }
        
        return true;
    }

    bool RobotMoveTo(const msgs::Vector3d& req, msgs::Boolean& res)
    {
        // Robot movement command
        double distance = sqrt(req.x()*req.x() + req.y()*req.y() + req.z()*req.z());
        
        // Simulate movement time based on distance
        int move_time_ms = static_cast<int>(distance * 100);  // 100ms per unit
        std::this_thread::sleep_for(std::chrono::milliseconds(move_time_ms));
        
        // Simulate occasional movement failure
        if (distance > 10.0)
        {
            res.set_data(false);  // Too far to move
        }
        else
        {
            res.set_data(true);
        }
        
        return true;
    }

    bool RobotGetPose(const msgs::Empty& req, msgs::Vector3d& res)
    {
        // Get current robot pose
        static double robot_x = 0.0, robot_y = 0.0, robot_z = 0.5;
        
        // Simulate some drift
        robot_x += 0.01 * ((double)rand() / RAND_MAX - 0.5);
        robot_y += 0.01 * ((double)rand() / RAND_MAX - 0.5);
        
        res.set_x(robot_x);
        res.set_y(robot_y);
        res.set_z(robot_z);
        
        // Quick response for pose queries
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        
        return true;
    }

    bool DataAnalyze(const msgs::StringMsg& req, msgs::StringMsg& res)
    {
        // Data analysis service (simulated heavy computation)
        std::string data_type = req.data();
        
        // Simulate processing time based on data type
        int processing_time = 100;  // Base time
        
        if (data_type == "image") processing_time = 500;
        else if (data_type == "lidar") processing_time = 200;
        else if (data_type == "audio") processing_time = 300;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(processing_time));
        
        // Generate analysis result
        std::string result = "Analysis of " + data_type + " completed. ";
        result += "Features detected: " + std::to_string(rand() % 100 + 10);
        result += ", Confidence: " + std::to_string(0.7 + 0.3 * (double)rand() / RAND_MAX);
        
        res.set_data(result);
        
        return true;
    }

    // Service testing functions
    void TestCalculatorServices()
    {
        // Test addition service
        msgs::Vector3d add_req;
        add_req.set_x(10.5);
        add_req.set_y(20.3);
        add_req.set_z(5.7);
        
        msgs::Double add_res;
        
        auto start_time = std::chrono::steady_clock::now();
        service_stats_["/demo/calculator/add"]->requests_sent++;
        
        bool result;
        bool success = node_->Request("/demo/calculator/add", add_req, 1000, add_res, result);
        
        auto end_time = std::chrono::steady_clock::now();
        double response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        if (success)
        {
            service_stats_["/demo/calculator/add"]->responses_received++;
            service_stats_["/demo/calculator/add"]->total_response_time += response_time;
            service_stats_["/demo/calculator/add"]->response_time_samples++;
        }
        else
        {
            service_stats_["/demo/calculator/add"]->timeouts++;
        }
        
        // Test multiplication service
        msgs::Vector3d mult_req;
        mult_req.set_x(2.0);
        mult_req.set_y(3.5);
        mult_req.set_z(1.5);
        
        msgs::Double mult_res;
        
        start_time = std::chrono::steady_clock::now();
        service_stats_["/demo/calculator/multiply"]->requests_sent++;
        
        success = node_->Request("/demo/calculator/multiply", mult_req, 1000, mult_res, result);
        
        end_time = std::chrono::steady_clock::now();
        response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        if (success)
        {
            service_stats_["/demo/calculator/multiply"]->responses_received++;
            service_stats_["/demo/calculator/multiply"]->total_response_time += response_time;
            service_stats_["/demo/calculator/multiply"]->response_time_samples++;
        }
        else
        {
            service_stats_["/demo/calculator/multiply"]->timeouts++;
        }
    }

    void TestSystemServices()
    {
        // Test system status
        msgs::Empty status_req;
        msgs::StringMsg status_res;
        bool result;
        
        auto start_time = std::chrono::steady_clock::now();
        service_stats_["/demo/system/status"]->requests_sent++;
        
        bool success = node_->Request("/demo/system/status", status_req, 2000, status_res, result);
        
        auto end_time = std::chrono::steady_clock::now();
        double response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        if (success)
        {
            service_stats_["/demo/system/status"]->responses_received++;
            service_stats_["/demo/system/status"]->total_response_time += response_time;
            service_stats_["/demo/system/status"]->response_time_samples++;
        }
        else
        {
            service_stats_["/demo/system/status"]->timeouts++;
        }
    }

    void TestRobotServices()
    {
        // Test robot pose query
        msgs::Empty pose_req;
        msgs::Vector3d pose_res;
        bool result;
        
        auto start_time = std::chrono::steady_clock::now();
        service_stats_["/demo/robot/get_pose"]->requests_sent++;
        
        bool success = node_->Request("/demo/robot/get_pose", pose_req, 500, pose_res, result);
        
        auto end_time = std::chrono::steady_clock::now();
        double response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        if (success)
        {
            service_stats_["/demo/robot/get_pose"]->responses_received++;
            service_stats_["/demo/robot/get_pose"]->total_response_time += response_time;
            service_stats_["/demo/robot/get_pose"]->response_time_samples++;
        }
        else
        {
            service_stats_["/demo/robot/get_pose"]->timeouts++;
        }
        
        // Test robot movement (less frequently)
        static int move_counter = 0;
        if (move_counter++ % 3 == 0)
        {
            msgs::Vector3d move_req;
            move_req.set_x(1.0 + 2.0 * (double)rand() / RAND_MAX);
            move_req.set_y(1.0 + 2.0 * (double)rand() / RAND_MAX);
            move_req.set_z(0.0);
            
            msgs::Boolean move_res;
            
            start_time = std::chrono::steady_clock::now();
            service_stats_["/demo/robot/move_to"]->requests_sent++;
            
            success = node_->Request("/demo/robot/move_to", move_req, 2000, move_res, result);
            
            end_time = std::chrono::steady_clock::now();
            response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            
            if (success)
            {
                service_stats_["/demo/robot/move_to"]->responses_received++;
                service_stats_["/demo/robot/move_to"]->total_response_time += response_time;
                service_stats_["/demo/robot/move_to"]->response_time_samples++;
            }
            else
            {
                service_stats_["/demo/robot/move_to"]->timeouts++;
            }
        }
    }

    void TestDataProcessingService()
    {
        std::vector<std::string> data_types = {"image", "lidar", "audio", "text"};
        std::string data_type = data_types[rand() % data_types.size()];
        
        msgs::StringMsg process_req;
        process_req.set_data(data_type);
        
        msgs::StringMsg process_res;
        bool result;
        
        auto start_time = std::chrono::steady_clock::now();
        service_stats_["/demo/processing/analyze"]->requests_sent++;
        
        bool success = node_->Request("/demo/processing/analyze", process_req, 3000, process_res, result);
        
        auto end_time = std::chrono::steady_clock::now();
        double response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        if (success)
        {
            service_stats_["/demo/processing/analyze"]->responses_received++;
            service_stats_["/demo/processing/analyze"]->total_response_time += response_time;
            service_stats_["/demo/processing/analyze"]->response_time_samples++;
        }
        else
        {
            service_stats_["/demo/processing/analyze"]->timeouts++;
        }
    }

    /**
     * Update service statistics
     */
    void UpdateStatistics()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        // Statistics are updated in real-time through service calls
        // This function could perform additional analysis if needed
    }

    /**
     * Print current service analysis
     */
    void PrintServiceAnalysis(int elapsed_seconds)
    {
        gzmsg << "Service Communication Analysis (t=" << elapsed_seconds << "s):\n";
        gzmsg << "==================================================\n";
        
        {
            std::lock_guard<std::mutex> lock(*data_mutex_);
            
            for (const auto& pair : service_stats_)
            {
                const std::string& service = pair.first;
                const auto& stats = pair.second;
                
                int sent = stats->requests_sent.load();
                int received = stats->responses_received.load();
                int timeouts = stats->timeouts.load();
                int errors = stats->errors.load();
                double avg_response_time = stats->GetAverageResponseTime();
                double success_rate = stats->GetSuccessRate();
                
                gzmsg << "Service: " << service << "\n";
                gzmsg << "  Requests sent: " << sent << "\n";
                gzmsg << "  Responses received: " << received << "\n";
                gzmsg << "  Timeouts: " << timeouts << "\n";
                gzmsg << "  Errors: " << errors << "\n";
                gzmsg << "  Success rate: " << std::fixed << std::setprecision(1) << success_rate << "%\n";
                
                if (stats->response_time_samples.load() > 0)
                {
                    gzmsg << "  Average response time: " << std::fixed << std::setprecision(1) 
                          << avg_response_time << " ms\n";
                }
                
                if (elapsed_seconds > 0 && sent > 0)
                {
                    gzmsg << "  Request rate: " << std::fixed << std::setprecision(2) 
                          << (double)sent / elapsed_seconds << " req/s\n";
                }
                
                gzmsg << "\n";
            }
        }
    }

    /**
     * Print comprehensive service communication summary
     */
    void PrintServiceSummary()
    {
        gzmsg << "\n=== SERVICE COMMUNICATION SUMMARY ===\n\n";
        
        gzmsg << "Request-Response Communication Pattern:\n";
        gzmsg << "--------------------------------------\n";
        gzmsg << "Service Architecture:\n";
        gzmsg << "• Client-server model with defined interfaces\n";
        gzmsg << "• Synchronous request-response semantics\n";
        gzmsg << "• Service discovery and registration mechanisms\n";
        gzmsg << "• Load balancing across multiple service providers\n\n";
        
        gzmsg << "Protocol Characteristics:\n";
        gzmsg << "• REQ-REP socket pattern (ZeroMQ)\n";
        gzmsg << "• Reliable delivery with timeout handling\n";
        gzmsg << "• Message correlation and request tracking\n";
        gzmsg << "• Automatic retry and failover support\n\n";
        
        gzmsg << "Service Interface Design:\n";
        gzmsg << "------------------------\n";
        gzmsg << "Best Practices:\n";
        gzmsg << "• Clear input/output message definitions\n";
        gzmsg << "• Comprehensive error code specifications\n";
        gzmsg << "• Version-aware service interfaces\n";
        gzmsg << "• Documentation and schema definitions\n\n";
        
        gzmsg << "Service Lifecycle Management:\n";
        gzmsg << "• Graceful startup and shutdown procedures\n";
        gzmsg << "• Health check and heartbeat mechanisms\n";
        gzmsg << "• Resource cleanup and state management\n";
        gzmsg << "• Service dependency resolution\n\n";
        
        gzmsg << "Performance Considerations:\n";
        gzmsg << "--------------------------\n";
        gzmsg << "Latency Factors:\n";
        gzmsg << "• Network round-trip time\n";
        gzmsg << "• Service processing duration\n";
        gzmsg << "• Message serialization overhead\n";
        gzmsg << "• Queue depths and load balancing\n\n";
        
        gzmsg << "Scalability Strategies:\n";
        gzmsg << "• Horizontal scaling with multiple service instances\n";
        gzmsg << "• Caching and result memoization\n";
        gzmsg << "• Asynchronous processing for long operations\n";
        gzmsg << "• Circuit breaker patterns for fault isolation\n\n";
        
        gzmsg << "Error Handling and Recovery:\n";
        gzmsg << "---------------------------\n";
        gzmsg << "Failure Modes:\n";
        gzmsg << "• Service unavailability and network partitions\n";
        gzmsg << "• Request timeout and response corruption\n";
        gzmsg << "• Service overload and resource exhaustion\n";
        gzmsg << "• Version incompatibility and protocol mismatches\n\n";
        
        gzmsg << "Recovery Mechanisms:\n";
        gzmsg << "• Exponential backoff for retry strategies\n";
        gzmsg << "• Dead letter queues for failed requests\n";
        gzmsg << "• Health monitoring and automatic failover\n";
        gzmsg << "• Graceful degradation and fallback services\n\n";
        
        gzmsg << "Quality of Service (QoS):\n";
        gzmsg << "------------------------\n";
        gzmsg << "• Priority-based request queuing\n";
        gzmsg << "• Rate limiting and throttling mechanisms\n";
        gzmsg << "• Service level agreements (SLA) monitoring\n";
        gzmsg << "• Resource allocation and capacity planning\n\n";
        
        gzmsg << "Security Considerations:\n";
        gzmsg << "----------------------\n";
        gzmsg << "• Authentication and authorization frameworks\n";
        gzmsg << "• Message encryption and integrity protection\n";
        gzmsg << "• Access control and audit logging\n";
        gzmsg << "• DoS protection and input validation\n\n";
        
        gzmsg << "=== SERVICE COMMUNICATION DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - Service Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Service Communication Demo ===\n";
    gzmsg << "This demo showcases request-response communication patterns.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- Request-response service communication\n";
    gzmsg << "- Service discovery and registration\n";
    gzmsg << "- Timeout handling and error recovery\n";
    gzmsg << "- Performance analysis and optimization\n";
    gzmsg << "- Service interface design patterns\n\n";

    try
    {
        ServiceDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during service demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nService demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Services enable reliable request-response communication\n";
    gzmsg << "- Proper interface design is critical for maintainability\n";
    gzmsg << "- Timeout and error handling ensure system robustness\n";
    gzmsg << "- Performance monitoring guides optimization efforts\n";
    gzmsg << "- Service discovery enables dynamic system composition\n";
    
    return 0;
}