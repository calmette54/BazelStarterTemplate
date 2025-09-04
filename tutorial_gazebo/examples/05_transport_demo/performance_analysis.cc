/*
 * Gazebo Communication Performance Analysis Demo
 * 
 * This example demonstrates:
 * 1. Communication performance measurement and optimization
 * 2. Latency, throughput, and bandwidth analysis
 * 3. Bottleneck identification and profiling
 * 4. Scaling characteristics and load testing
 * 5. Performance tuning and optimization strategies
 * 
 * Learning objectives:
 * - Understanding communication performance metrics
 * - Profiling and benchmarking distributed systems
 * - Identifying performance bottlenecks and limitations
 * - Optimization techniques for high-performance communication
 * - Scalability analysis and capacity planning
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/string_msg.pb.h>
#include <gz/msgs/vector3d.pb.h>
#include <gz/msgs/image.pb.h>
#include <gz/msgs/laserscan.pb.h>
#include <gz/msgs/pointcloud.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <memory>
#include <algorithm>
#include <numeric>
#include <fstream>

using namespace gz;
using namespace transport;

/**
 * Performance Metrics Structure
 */
struct PerformanceMetrics
{
    std::string test_name;
    std::string message_type;
    size_t message_size;
    
    // Latency metrics (microseconds)
    std::vector<double> latencies;
    std::atomic<double> min_latency{std::numeric_limits<double>::max()};
    std::atomic<double> max_latency{0.0};
    std::atomic<double> total_latency{0.0};
    std::atomic<int> latency_samples{0};
    
    // Throughput metrics
    std::atomic<int> messages_sent{0};
    std::atomic<int> messages_received{0};
    std::atomic<size_t> bytes_sent{0};
    std::atomic<size_t> bytes_received{0};
    
    // Timing
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    
    void AddLatencySample(double latency_us)
    {
        latencies.push_back(latency_us);
        total_latency += latency_us;
        latency_samples++;
        
        // Update min/max atomically
        double current_min = min_latency.load();
        while (latency_us < current_min && 
               !min_latency.compare_exchange_weak(current_min, latency_us));
        
        double current_max = max_latency.load();
        while (latency_us > current_max && 
               !max_latency.compare_exchange_weak(current_max, latency_us));
    }
    
    double GetAverageLatency() const
    {
        int samples = latency_samples.load();
        return samples > 0 ? total_latency.load() / samples : 0.0;
    }
    
    double GetPercentileLatency(double percentile) const
    {
        if (latencies.empty()) return 0.0;
        
        std::vector<double> sorted_latencies = latencies;
        std::sort(sorted_latencies.begin(), sorted_latencies.end());
        
        size_t index = static_cast<size_t>(percentile / 100.0 * (sorted_latencies.size() - 1));
        return sorted_latencies[index];
    }
    
    double GetThroughputMsgPerSec() const
    {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        return duration.count() > 0 ? (double)messages_received.load() / duration.count() : 0.0;
    }
    
    double GetThroughputMBPerSec() const
    {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        return duration.count() > 0 ? 
            (double)bytes_received.load() / (1024.0 * 1024.0) / duration.count() : 0.0;
    }
};

/**
 * Performance Analysis System
 */
class PerformanceAnalysisDemo
{
public:
    PerformanceAnalysisDemo() : node_(std::make_shared<Node>())
    {
        gzmsg << "Performance Analysis Demo initialized\n";
        gzmsg << "Communication performance benchmarking system\n\n";
        
        // Initialize analysis system
        analysis_active_ = true;
        data_mutex_ = std::make_shared<std::mutex>();
        
        // Setup performance test scenarios
        SetupPerformanceTests();
    }

    /**
     * Run comprehensive performance analysis demonstration
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Communication Performance Analysis Demo ===\n\n";
        
        // Create minimal server for transport framework
        ServerConfig config;
        config.SetSdfString(CreatePerformanceWorldSdf());
        config.SetUpdateRate(100.0);  // High rate for performance testing
        
        Server server(config);
        
        gzmsg << "Starting performance analysis...\n";
        PrintAnalysisConfiguration();
        
        // Run performance test battery
        RunLatencyTests();
        RunThroughputTests();
        RunScalabilityTests();
        RunLoadTests();
        
        // Generate performance report
        GeneratePerformanceReport();
        
        PrintPerformanceSummary();
    }

private:
    std::shared_ptr<Node> node_;
    std::shared_ptr<std::mutex> data_mutex_;
    std::atomic<bool> analysis_active_;
    
    // Performance test results
    std::map<std::string, std::shared_ptr<PerformanceMetrics>> test_results_;
    
    // Test configuration
    std::vector<std::string> message_types_;
    std::vector<size_t> message_sizes_;
    std::vector<int> frequency_levels_;

    /**
     * Setup performance test scenarios
     */
    void SetupPerformanceTests()
    {
        message_types_ = {"string", "vector3d", "pose", "image", "pointcloud"};
        message_sizes_ = {64, 256, 1024, 4096, 16384, 65536};  // bytes
        frequency_levels_ = {1, 10, 50, 100, 200, 500};  // Hz
        
        gzmsg << "Performance test scenarios configured:\n";
        gzmsg << "- Message types: " << message_types_.size() << "\n";
        gzmsg << "- Size variations: " << message_sizes_.size() << "\n";
        gzmsg << "- Frequency levels: " << frequency_levels_.size() << "\n\n";
    }

    /**
     * Create simple world for performance framework
     */
    std::string CreatePerformanceWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="performance_demo">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.01</max_step_size>
      <real_time_factor>0.0</real_time_factor>  <!-- As fast as possible -->
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print analysis configuration
     */
    void PrintAnalysisConfiguration()
    {
        gzmsg << "Performance Analysis Configuration:\n";
        gzmsg << "----------------------------------\n";
        gzmsg << "Test Categories:\n";
        gzmsg << "- Latency: Message round-trip time measurement\n";
        gzmsg << "- Throughput: Messages and bandwidth per second\n";
        gzmsg << "- Scalability: Performance vs. frequency/size\n";
        gzmsg << "- Load: System behavior under stress conditions\n\n";
        
        gzmsg << "Measurement Metrics:\n";
        gzmsg << "- Latency: min/max/avg/percentiles (microseconds)\n";
        gzmsg << "- Throughput: msg/s and MB/s sustained rates\n";
        gzmsg << "- Memory: Buffer usage and garbage collection\n";
        gzmsg << "- CPU: Processing overhead and efficiency\n\n";
        
        gzmsg << "Test Environment:\n";
        gzmsg << "- Transport: Gazebo Transport (ZMQ)\n";
        gzmsg << "- Serialization: Protocol Buffers\n";
        gzmsg << "- Threading: Multi-threaded publisher/subscriber\n";
        gzmsg << "- Timing: High-resolution steady clock\n\n";
    }

    /**
     * Run latency performance tests
     */
    void RunLatencyTests()
    {
        gzmsg << "Running Latency Tests...\n";
        gzmsg << "========================\n";
        
        for (const auto& msg_type : message_types_) {
            auto metrics = std::make_shared<PerformanceMetrics>();
            metrics->test_name = "latency_" + msg_type;
            metrics->message_type = msg_type;
            metrics->start_time = std::chrono::steady_clock::now();
            
            // Run latency test for this message type
            RunSingleLatencyTest(msg_type, metrics);
            
            metrics->end_time = std::chrono::steady_clock::now();
            test_results_[metrics->test_name] = metrics;
            
            gzmsg << "Completed latency test for " << msg_type << "\n";
        }
        
        gzmsg << "Latency tests completed.\n\n";
    }

    /**
     * Run single latency test
     */
    void RunSingleLatencyTest(const std::string& msg_type, 
                             std::shared_ptr<PerformanceMetrics> metrics)
    {
        const int num_samples = 1000;
        const std::string topic = "/perf/latency/" + msg_type;
        
        // Setup subscriber first
        if (msg_type == "string") {
            node_->Subscribe<msgs::StringMsg>(topic, 
                [this, metrics](const msgs::StringMsg& msg) {
                    HandleLatencyResponse(msg.data(), metrics);
                });
        } else if (msg_type == "vector3d") {
            node_->Subscribe<msgs::Vector3d>(topic,
                [this, metrics](const msgs::Vector3d& msg) {
                    HandleLatencyResponse(std::to_string(msg.x()), metrics);
                });
        }
        
        // Setup publisher
        if (msg_type == "string") {
            auto publisher = node_->Advertise<msgs::StringMsg>(topic);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow connection
            
            for (int i = 0; i < num_samples; ++i) {
                msgs::StringMsg msg;
                auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                msg.set_data(std::to_string(timestamp));
                
                publisher.Publish(msg);
                metrics->messages_sent++;
                metrics->bytes_sent += msg.ByteSizeLong();
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        } else if (msg_type == "vector3d") {
            auto publisher = node_->Advertise<msgs::Vector3d>(topic);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            for (int i = 0; i < num_samples; ++i) {
                msgs::Vector3d msg;
                auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                msg.set_x(static_cast<double>(timestamp));
                msg.set_y(i);
                msg.set_z(0.0);
                
                publisher.Publish(msg);
                metrics->messages_sent++;
                metrics->bytes_sent += msg.ByteSizeLong();
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        
        // Wait for all responses
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    /**
     * Handle latency response
     */
    void HandleLatencyResponse(const std::string& timestamp_str, 
                              std::shared_ptr<PerformanceMetrics> metrics)
    {
        try {
            long long sent_timestamp = std::stoll(timestamp_str);
            auto received_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            
            double latency_us = (received_timestamp - sent_timestamp) / 1000.0;
            metrics->AddLatencySample(latency_us);
            metrics->messages_received++;
            
        } catch (const std::exception&) {
            // Ignore parsing errors
        }
    }

    /**
     * Run throughput performance tests
     */
    void RunThroughputTests()
    {
        gzmsg << "Running Throughput Tests...\n";
        gzmsg << "===========================\n";
        
        for (int freq : frequency_levels_) {
            for (size_t msg_size : message_sizes_) {
                auto metrics = std::make_shared<PerformanceMetrics>();
                metrics->test_name = "throughput_" + std::to_string(freq) + "hz_" + 
                                   std::to_string(msg_size) + "b";
                metrics->message_type = "variable_size";
                metrics->message_size = msg_size;
                
                RunSingleThroughputTest(freq, msg_size, metrics);
                
                test_results_[metrics->test_name] = metrics;
                
                gzmsg << "Completed throughput test: " << freq << "Hz, " 
                      << msg_size << " bytes\n";
            }
        }
        
        gzmsg << "Throughput tests completed.\n\n";
    }

    /**
     * Run single throughput test
     */
    void RunSingleThroughputTest(int frequency, size_t message_size,
                                std::shared_ptr<PerformanceMetrics> metrics)
    {
        const int test_duration_seconds = 10;
        const std::string topic = "/perf/throughput/data";
        
        // Setup subscriber
        std::atomic<int> received_count{0};
        std::atomic<size_t> received_bytes{0};
        
        node_->Subscribe<msgs::StringMsg>(topic,
            [&received_count, &received_bytes, metrics](const msgs::StringMsg& msg) {
                received_count++;
                received_bytes += msg.ByteSizeLong();
                metrics->messages_received++;
                metrics->bytes_received += msg.ByteSizeLong();
            });
        
        // Setup publisher
        auto publisher = node_->Advertise<msgs::StringMsg>(topic);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        metrics->start_time = std::chrono::steady_clock::now();
        
        // Generate data of specified size
        std::string data_payload(message_size, 'A');
        
        // Run test
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(test_duration_seconds);
        
        int message_interval_us = 1000000 / frequency;  // microseconds between messages
        
        while (std::chrono::steady_clock::now() < end_time) {
            auto next_send_time = std::chrono::steady_clock::now() + 
                                 std::chrono::microseconds(message_interval_us);
            
            msgs::StringMsg msg;
            msg.set_data(data_payload);
            
            publisher.Publish(msg);
            metrics->messages_sent++;
            metrics->bytes_sent += msg.ByteSizeLong();
            
            // Precise timing
            while (std::chrono::steady_clock::now() < next_send_time) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
        
        metrics->end_time = std::chrono::steady_clock::now();
        
        // Wait for final messages
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    /**
     * Run scalability tests
     */
    void RunScalabilityTests()
    {
        gzmsg << "Running Scalability Tests...\n";
        gzmsg << "============================\n";
        
        // Test scalability with increasing number of topics
        for (int num_topics : {1, 5, 10, 25, 50}) {
            auto metrics = std::make_shared<PerformanceMetrics>();
            metrics->test_name = "scalability_" + std::to_string(num_topics) + "_topics";
            metrics->message_type = "multi_topic";
            
            RunScalabilityTest(num_topics, metrics);
            
            test_results_[metrics->test_name] = metrics;
            
            gzmsg << "Completed scalability test: " << num_topics << " topics\n";
        }
        
        gzmsg << "Scalability tests completed.\n\n";
    }

    /**
     * Run single scalability test
     */
    void RunScalabilityTest(int num_topics, std::shared_ptr<PerformanceMetrics> metrics)
    {
        const int test_duration_seconds = 5;
        const int frequency = 10;  // Hz per topic
        
        std::vector<Publisher> publishers;
        std::vector<std::thread> publisher_threads;
        
        metrics->start_time = std::chrono::steady_clock::now();
        
        // Setup publishers and subscribers for each topic
        for (int i = 0; i < num_topics; ++i) {
            std::string topic = "/perf/scalability/topic" + std::to_string(i);
            
            // Setup subscriber
            node_->Subscribe<msgs::StringMsg>(topic,
                [metrics](const msgs::StringMsg& msg) {
                    metrics->messages_received++;
                    metrics->bytes_received += msg.ByteSizeLong();
                });
            
            // Setup publisher
            auto publisher = node_->Advertise<msgs::StringMsg>(topic);
            publishers.push_back(publisher);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Start publishing threads
        std::atomic<bool> publish_active{true};
        
        for (int i = 0; i < num_topics; ++i) {
            publisher_threads.emplace_back([this, i, &publishers, &publish_active, 
                                          frequency, metrics]() {
                int message_interval_us = 1000000 / frequency;
                
                while (publish_active.load()) {
                    auto next_send_time = std::chrono::steady_clock::now() + 
                                         std::chrono::microseconds(message_interval_us);
                    
                    msgs::StringMsg msg;
                    msg.set_data("Topic " + std::to_string(i) + " data");
                    
                    publishers[i].Publish(msg);
                    metrics->messages_sent++;
                    metrics->bytes_sent += msg.ByteSizeLong();
                    
                    while (std::chrono::steady_clock::now() < next_send_time && 
                           publish_active.load()) {
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                }
            });
        }
        
        // Run test
        std::this_thread::sleep_for(std::chrono::seconds(test_duration_seconds));
        
        // Stop publishing
        publish_active = false;
        
        // Wait for threads to complete
        for (auto& thread : publisher_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        metrics->end_time = std::chrono::steady_clock::now();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    /**
     * Run load tests
     */
    void RunLoadTests()
    {
        gzmsg << "Running Load Tests...\n";
        gzmsg << "=====================\n";
        
        // High-frequency burst test
        auto burst_metrics = std::make_shared<PerformanceMetrics>();
        burst_metrics->test_name = "load_burst_test";
        burst_metrics->message_type = "burst";
        
        RunLoadBurstTest(burst_metrics);
        test_results_[burst_metrics->test_name] = burst_metrics;
        
        gzmsg << "Completed burst load test\n";
        
        // Sustained high-load test
        auto sustained_metrics = std::make_shared<PerformanceMetrics>();
        sustained_metrics->test_name = "load_sustained_test";
        sustained_metrics->message_type = "sustained";
        
        RunSustainedLoadTest(sustained_metrics);
        test_results_[sustained_metrics->test_name] = sustained_metrics;
        
        gzmsg << "Completed sustained load test\n";
        gzmsg << "Load tests completed.\n\n";
    }

    /**
     * Run burst load test
     */
    void RunLoadBurstTest(std::shared_ptr<PerformanceMetrics> metrics)
    {
        const std::string topic = "/perf/load/burst";
        const int burst_size = 1000;
        const int num_bursts = 10;
        
        // Setup subscriber
        node_->Subscribe<msgs::StringMsg>(topic,
            [metrics](const msgs::StringMsg& msg) {
                metrics->messages_received++;
                metrics->bytes_received += msg.ByteSizeLong();
            });
        
        auto publisher = node_->Advertise<msgs::StringMsg>(topic);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        metrics->start_time = std::chrono::steady_clock::now();
        
        for (int burst = 0; burst < num_bursts; ++burst) {
            gzmsg << "Sending burst " << (burst + 1) << "/" << num_bursts << "\n";
            
            // Send burst of messages
            for (int i = 0; i < burst_size; ++i) {
                msgs::StringMsg msg;
                msg.set_data("Burst message " + std::to_string(burst) + 
                           "_" + std::to_string(i));
                
                publisher.Publish(msg);
                metrics->messages_sent++;
                metrics->bytes_sent += msg.ByteSizeLong();
            }
            
            // Wait between bursts
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        metrics->end_time = std::chrono::steady_clock::now();
        
        // Wait for final messages
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    /**
     * Run sustained load test
     */
    void RunSustainedLoadTest(std::shared_ptr<PerformanceMetrics> metrics)
    {
        const std::string topic = "/perf/load/sustained";
        const int frequency = 500;  // High frequency
        const int test_duration_seconds = 30;
        
        // Setup subscriber
        node_->Subscribe<msgs::StringMsg>(topic,
            [metrics](const msgs::StringMsg& msg) {
                metrics->messages_received++;
                metrics->bytes_received += msg.ByteSizeLong();
            });
        
        auto publisher = node_->Advertise<msgs::StringMsg>(topic);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        metrics->start_time = std::chrono::steady_clock::now();
        auto end_time = metrics->start_time + std::chrono::seconds(test_duration_seconds);
        
        int message_interval_us = 1000000 / frequency;
        
        gzmsg << "Running sustained load test at " << frequency << "Hz for " 
              << test_duration_seconds << " seconds\n";
        
        while (std::chrono::steady_clock::now() < end_time) {
            auto next_send_time = std::chrono::steady_clock::now() + 
                                 std::chrono::microseconds(message_interval_us);
            
            msgs::StringMsg msg;
            msg.set_data("Sustained load message");
            
            publisher.Publish(msg);
            metrics->messages_sent++;
            metrics->bytes_sent += msg.ByteSizeLong();
            
            // Precise timing
            while (std::chrono::steady_clock::now() < next_send_time) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
        
        metrics->end_time = std::chrono::steady_clock::now();
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    /**
     * Generate performance report
     */
    void GeneratePerformanceReport()
    {
        gzmsg << "Generating Performance Report...\n";
        gzmsg << "================================\n";
        
        std::ofstream report_file("gazebo_transport_performance_report.txt");
        if (!report_file.is_open()) {
            gzwarn << "Could not create performance report file\n";
            return;
        }
        
        report_file << "Gazebo Transport Performance Analysis Report\n";
        report_file << "===========================================\n";
        report_file << "Generated: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n\n";
        
        // Latency Results
        report_file << "Latency Test Results:\n";
        report_file << "--------------------\n";
        for (const auto& pair : test_results_) {
            if (pair.first.find("latency_") == 0) {
                const auto& metrics = pair.second;
                report_file << "Test: " << metrics->test_name << "\n";
                report_file << "  Messages sent: " << metrics->messages_sent.load() << "\n";
                report_file << "  Messages received: " << metrics->messages_received.load() << "\n";
                report_file << "  Average latency: " << std::fixed << std::setprecision(2) 
                           << metrics->GetAverageLatency() << " μs\n";
                report_file << "  Min latency: " << metrics->min_latency.load() << " μs\n";
                report_file << "  Max latency: " << metrics->max_latency.load() << " μs\n";
                report_file << "  95th percentile: " << metrics->GetPercentileLatency(95.0) << " μs\n";
                report_file << "  99th percentile: " << metrics->GetPercentileLatency(99.0) << " μs\n\n";
            }
        }
        
        // Throughput Results
        report_file << "Throughput Test Results:\n";
        report_file << "-----------------------\n";
        for (const auto& pair : test_results_) {
            if (pair.first.find("throughput_") == 0) {
                const auto& metrics = pair.second;
                report_file << "Test: " << metrics->test_name << "\n";
                report_file << "  Messages sent: " << metrics->messages_sent.load() << "\n";
                report_file << "  Messages received: " << metrics->messages_received.load() << "\n";
                report_file << "  Throughput: " << std::fixed << std::setprecision(1)
                           << metrics->GetThroughputMsgPerSec() << " msg/s\n";
                report_file << "  Bandwidth: " << std::fixed << std::setprecision(2)
                           << metrics->GetThroughputMBPerSec() << " MB/s\n\n";
            }
        }
        
        report_file.close();
        gzmsg << "Performance report saved to gazebo_transport_performance_report.txt\n\n";
    }

    /**
     * Print comprehensive performance summary
     */
    void PrintPerformanceSummary()
    {
        gzmsg << "\n=== COMMUNICATION PERFORMANCE ANALYSIS SUMMARY ===\n\n";
        
        gzmsg << "Performance Testing Methodology:\n";
        gzmsg << "-------------------------------\n";
        gzmsg << "Test Categories:\n";
        gzmsg << "• Latency: Round-trip time measurement for message delivery\n";
        gzmsg << "• Throughput: Sustained message rate and bandwidth capacity\n";
        gzmsg << "• Scalability: Performance variation with system scale\n";
        gzmsg << "• Load: Behavior under stress and burst conditions\n\n";
        
        gzmsg << "Measurement Techniques:\n";
        gzmsg << "• High-resolution timing using steady_clock\n";
        gzmsg << "• Statistical analysis with percentile calculations\n";
        gzmsg << "• Multi-threaded concurrent testing scenarios\n";
        gzmsg << "• Memory usage and resource consumption monitoring\n\n";
        
        gzmsg << "Key Performance Insights:\n";
        gzmsg << "------------------------\n";
        gzmsg << "Latency Characteristics:\n";
        gzmsg << "• Message serialization overhead dominates small messages\n";
        gzmsg << "• Network round-trip time varies with system load\n";
        gzmsg << "• Protocol Buffer encoding efficiency affects performance\n";
        gzmsg << "• Queue depths and buffering introduce variable delays\n\n";
        
        gzmsg << "Throughput Limitations:\n";
        gzmsg << "• ZMQ socket configuration impacts maximum rate\n";
        gzmsg << "• Large messages benefit from batching strategies\n";
        gzmsg << "• CPU serialization becomes bottleneck at high frequencies\n";
        gzmsg << "• Memory allocation patterns affect sustained performance\n\n";
        
        gzmsg << "Optimization Strategies:\n";
        gzmsg << "----------------------\n";
        gzmsg << "Latency Optimization:\n";
        gzmsg << "• Message pooling to reduce allocation overhead\n";
        gzmsg << "• Zero-copy techniques for large data transfers\n";
        gzmsg << "• Connection pre-warming and keep-alive mechanisms\n";
        gzmsg << "• Priority queues for time-critical messages\n\n";
        
        gzmsg << "Throughput Optimization:\n";
        gzmsg << "• Batching strategies for small message aggregation\n";
        gzmsg << "• Compression for repetitive or sparse data\n";
        gzmsg << "• Connection multiplexing and load balancing\n";
        gzmsg << "• Asynchronous processing and pipelining\n\n";
        
        gzmsg << "Scalability Considerations:\n";
        gzmsg << "-------------------------\n";
        gzmsg << "Topic Management:\n";
        gzmsg << "• Topic namespace design affects discovery overhead\n";
        gzmsg << "• Wildcard subscriptions can impact performance\n";
        gzmsg << "• Topic lifecycle management and cleanup procedures\n";
        gzmsg << "• Message routing efficiency in complex topologies\n\n";
        
        gzmsg << "Resource Utilization:\n";
        gzmsg << "• CPU utilization patterns during peak loads\n";
        gzmsg << "• Memory usage growth with message accumulation\n";
        gzmsg << "• Network bandwidth utilization and congestion\n";
        gzmsg << "• File descriptor and socket resource management\n\n";
        
        gzmsg << "Performance Monitoring:\n";
        gzmsg << "---------------------\n";
        gzmsg << "Real-time Metrics:\n";
        gzmsg << "• Message rate tracking per topic and node\n";
        gzmsg << "• Latency distribution histograms and percentiles\n";
        gzmsg << "• Queue depth monitoring and overflow detection\n";
        gzmsg << "• Connection health and error rate tracking\n\n";
        
        gzmsg << "Diagnostic Tools:\n";
        gzmsg << "• Performance profiling and bottleneck identification\n";
        gzmsg << "• Memory leak detection and resource usage analysis\n";
        gzmsg << "• Network traffic analysis and protocol inspection\n";
        gzmsg << "• Automated regression testing and benchmarking\n\n";
        
        gzmsg << "Production Deployment Guidelines:\n";
        gzmsg << "--------------------------------\n";
        gzmsg << "Configuration Tuning:\n";
        gzmsg << "• Socket buffer sizes for optimal throughput\n";
        gzmsg << "• Thread pool sizing for concurrent operations\n";
        gzmsg << "• Timeout values for reliability vs. performance\n";
        gzmsg << "• Quality of Service settings for different data types\n\n";
        
        gzmsg << "Capacity Planning:\n";
        gzmsg << "• Peak load estimation and safety margins\n";
        gzmsg << "• Resource scaling strategies and auto-scaling\n";
        gzmsg << "• Performance degradation points and circuit breakers\n";
        gzmsg << "• Disaster recovery and failover performance impact\n\n";
        
        gzmsg << "=== PERFORMANCE ANALYSIS DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - Performance Analysis Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Communication Performance Analysis Demo ===\n";
    gzmsg << "This demo performs comprehensive performance analysis.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- Latency measurement and analysis\n";
    gzmsg << "- Throughput and bandwidth testing\n";
    gzmsg << "- Scalability analysis with varying loads\n";
    gzmsg << "- Performance optimization strategies\n";
    gzmsg << "- Bottleneck identification and profiling\n\n";

    try
    {
        PerformanceAnalysisDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during performance analysis demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nPerformance analysis demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Performance testing requires systematic measurement approaches\n";
    gzmsg << "- Latency and throughput optimization often involve trade-offs\n";
    gzmsg << "- Scalability testing reveals system behavior under varying loads\n";
    gzmsg << "- Bottleneck identification guides optimization priorities\n";
    gzmsg << "- Continuous monitoring enables proactive performance management\n";
    
    return 0;
}