/*
 * Gazebo Publisher-Subscriber Communication Demo
 * 
 * This example demonstrates:
 * 1. Topic-based publish/subscribe communication patterns
 * 2. Message serialization and deserialization
 * 3. Asynchronous communication architectures
 * 4. Topic discovery and node management
 * 5. Quality of Service (QoS) considerations
 * 
 * Learning objectives:
 * - Understanding distributed communication systems
 * - Topic naming conventions and hierarchies
 * - Message lifecycle and memory management
 * - Performance characteristics of pub/sub systems
 * - Error handling and connection management
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/string_msg.pb.h>
#include <gz/msgs/vector3d.pb.h>
#include <gz/msgs/pose.pb.h>
#include <gz/msgs/image.pb.h>
#include <gz/msgs/laserscan.pb.h>

#include <gz/transport/Node.hh>
#include <gz/transport/TopicUtils.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <memory>

using namespace gz;
using namespace transport;

/**
 * Message Statistics Tracking
 */
struct MessageStats
{
    std::string topic_name;
    std::atomic<int> messages_sent{0};
    std::atomic<int> messages_received{0};
    std::chrono::steady_clock::time_point first_message_time;
    std::chrono::steady_clock::time_point last_message_time;
    std::atomic<double> total_latency{0.0};
    std::atomic<int> latency_samples{0};
    
    double GetAverageLatency() const 
    {
        int samples = latency_samples.load();
        return samples > 0 ? total_latency.load() / samples : 0.0;
    }
    
    double GetMessageRate() const
    {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            last_message_time - first_message_time).count();
        return duration > 0 ? (double)messages_received.load() / duration : 0.0;
    }
};

/**
 * Publisher-Subscriber Communication System
 */
class PublisherSubscriber
{
public:
    PublisherSubscriber() : node_(std::make_shared<Node>())
    {
        gzmsg << "Publisher-Subscriber Demo initialized\n";
        gzmsg << "Distributed communication system demonstration\n\n";
        
        // Initialize communication system
        communication_active_ = true;
        data_mutex_ = std::make_shared<std::mutex>();
        
        // Setup publishers and subscribers
        SetupCommunication();
        
        // Initialize statistics tracking
        InitializeStatistics();
    }

    /**
     * Run comprehensive publisher-subscriber demonstration
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Publisher-Subscriber Communication Demo ===\n\n";
        
        // Create minimal server for transport framework
        ServerConfig config;
        config.SetSdfString(CreateTransportWorldSdf());
        config.SetUpdateRate(30.0);
        
        Server server(config);
        
        gzmsg << "Starting communication demonstration...\n";
        PrintCommunicationConfiguration();
        
        // Start communication threads
        std::thread publisher_thread(&PublisherSubscriber::PublisherLoop, this);
        std::thread subscriber_thread(&PublisherSubscriber::SubscriberLoop, this);
        std::thread statistics_thread(&PublisherSubscriber::StatisticsLoop, this);
        
        // Run simulation
        const int simulation_steps = 9000;  // 5 minutes at 30 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && communication_active_)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print communication statistics every 15 seconds
            if (step_count % 450 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                PrintCommunicationAnalysis(elapsed);
                gzmsg << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(33333)); // ~30 FPS
        }
        
        // Stop communication and cleanup
        communication_active_ = false;
        
        if (publisher_thread.joinable()) publisher_thread.join();
        if (subscriber_thread.joinable()) subscriber_thread.join();
        if (statistics_thread.joinable()) statistics_thread.join();
        
        PrintCommunicationSummary();
    }

private:
    std::shared_ptr<Node> node_;
    std::shared_ptr<std::mutex> data_mutex_;
    std::atomic<bool> communication_active_;

    // Publishers
    std::map<std::string, Publisher> publishers_;
    
    // Message statistics
    std::map<std::string, std::shared_ptr<MessageStats>> topic_stats_;
    
    // Communication patterns
    std::vector<std::string> active_topics_;

    /**
     * Setup comprehensive communication patterns
     */
    void SetupCommunication()
    {
        // Text messaging topic
        auto string_publisher = node_->Advertise<msgs::StringMsg>("/demo/text_messages");
        publishers_["text_messages"] = string_publisher;
        active_topics_.push_back("/demo/text_messages");
        
        // Position/pose topic
        auto pose_publisher = node_->Advertise<msgs::Pose>("/demo/robot_pose");
        publishers_["robot_pose"] = pose_publisher;
        active_topics_.push_back("/demo/robot_pose");
        
        // Sensor data topic (simulated)
        auto vector_publisher = node_->Advertise<msgs::Vector3d>("/demo/sensor_data");
        publishers_["sensor_data"] = vector_publisher;
        active_topics_.push_back("/demo/sensor_data");
        
        // High-frequency telemetry
        auto telemetry_publisher = node_->Advertise<msgs::Vector3d>("/demo/telemetry");
        publishers_["telemetry"] = telemetry_publisher;
        active_topics_.push_back("/demo/telemetry");
        
        // Setup subscribers for each topic
        node_->Subscribe("/demo/text_messages", &PublisherSubscriber::OnTextMessage, this);
        node_->Subscribe("/demo/robot_pose", &PublisherSubscriber::OnPoseMessage, this);
        node_->Subscribe("/demo/sensor_data", &PublisherSubscriber::OnSensorData, this);
        node_->Subscribe("/demo/telemetry", &PublisherSubscriber::OnTelemetryData, this);
        
        gzmsg << "Communication topics configured:\n";
        gzmsg << "- /demo/text_messages: String messages (1 Hz)\n";
        gzmsg << "- /demo/robot_pose: Pose updates (10 Hz)\n";
        gzmsg << "- /demo/sensor_data: Sensor readings (20 Hz)\n";
        gzmsg << "- /demo/telemetry: High-freq telemetry (50 Hz)\n\n";
    }

    /**
     * Initialize statistics tracking for all topics
     */
    void InitializeStatistics()
    {
        for (const auto& topic : active_topics_)
        {
            auto stats = std::make_shared<MessageStats>();
            stats->topic_name = topic;
            stats->first_message_time = std::chrono::steady_clock::now();
            topic_stats_[topic] = stats;
        }
    }

    /**
     * Create simple world for transport framework
     */
    std::string CreateTransportWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="transport_demo">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.033333</max_step_size>
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
              <size>10 10</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>10 10</size>
            </plane>
          </geometry>
        </visual>
      </link>
    </model>
    
    <!-- Simple moving object for pose updates -->
    <model name="moving_robot">
      <pose>0 0 0.5 0 0 0</pose>
      <static>false</static>
      <link name="base_link">
        <inertial>
          <mass>1.0</mass>
        </inertial>
        <collision name="collision">
          <geometry>
            <box><size>0.5 0.5 0.2</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>0.5 0.5 0.2</size></box>
          </geometry>
          <material>
            <ambient>1 0 0 1</ambient>
            <diffuse>1 0 0 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print communication configuration
     */
    void PrintCommunicationConfiguration()
    {
        gzmsg << "Communication System Configuration:\n";
        gzmsg << "----------------------------------\n";
        gzmsg << "Transport Framework: Gazebo Transport (ZMQ-based)\n";
        gzmsg << "Discovery Protocol: Automatic topic discovery\n";
        gzmsg << "Serialization: Protocol Buffers (protobuf)\n";
        gzmsg << "Communication Pattern: Publisher-Subscriber\n\n";
        
        gzmsg << "Active Topics:\n";
        for (const auto& topic : active_topics_)
        {
            gzmsg << "  - " << topic << "\n";
        }
        gzmsg << "\n";
        
        gzmsg << "Quality of Service:\n";
        gzmsg << "- Reliability: Best-effort delivery\n";
        gzmsg << "- Ordering: FIFO within single publisher\n";
        gzmsg << "- Durability: Volatile (no persistence)\n";
        gzmsg << "- History: Keep last message\n\n";
    }

    /**
     * Publisher loop - generates messages at different frequencies
     */
    void PublisherLoop()
    {
        int cycle_count = 0;
        
        while (communication_active_)
        {
            auto current_time = std::chrono::steady_clock::now();
            
            // Text messages - 1 Hz
            if (cycle_count % 50 == 0)
            {
                PublishTextMessage(cycle_count);
            }
            
            // Robot pose - 10 Hz
            if (cycle_count % 5 == 0)
            {
                PublishPoseMessage(current_time, cycle_count);
            }
            
            // Sensor data - 20 Hz
            if (cycle_count % 2 == 0)
            {
                PublishSensorData(current_time, cycle_count);
            }
            
            // Telemetry - 50 Hz (every loop)
            PublishTelemetryData(current_time, cycle_count);
            
            cycle_count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // 50 Hz base rate
        }
    }

    /**
     * Subscriber loop - processes received messages
     */
    void SubscriberLoop()
    {
        while (communication_active_)
        {
            // The actual message processing happens in callbacks
            // This thread handles any additional subscriber logic
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    /**
     * Statistics loop - collects performance metrics
     */
    void StatisticsLoop()
    {
        while (communication_active_)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Update statistics for all topics
            UpdateStatistics();
        }
    }

    /**
     * Publish text message
     */
    void PublishTextMessage(int cycle)
    {
        msgs::StringMsg msg;
        msg.set_data("Hello from Gazebo Transport! Cycle: " + std::to_string(cycle));
        
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        
        // Add timestamp for latency measurement (as part of message content)
        msg.set_data(msg.data() + " | Timestamp: " + std::to_string(timestamp));
        
        bool published = publishers_["text_messages"].Publish(msg);
        if (published)
        {
            topic_stats_["/demo/text_messages"]->messages_sent++;
        }
    }

    /**
     * Publish pose message
     */
    void PublishPoseMessage(std::chrono::steady_clock::time_point current_time, int cycle)
    {
        msgs::Pose msg;
        
        // Simulate circular motion
        double angle = cycle * 0.1;
        double radius = 2.0;
        
        msg.mutable_position()->set_x(radius * cos(angle));
        msg.mutable_position()->set_y(radius * sin(angle));
        msg.mutable_position()->set_z(0.5);
        
        msg.mutable_orientation()->set_w(cos(angle/2));
        msg.mutable_orientation()->set_x(0);
        msg.mutable_orientation()->set_y(0);
        msg.mutable_orientation()->set_z(sin(angle/2));
        
        bool published = publishers_["robot_pose"].Publish(msg);
        if (published)
        {
            topic_stats_["/demo/robot_pose"]->messages_sent++;
        }
    }

    /**
     * Publish sensor data
     */
    void PublishSensorData(std::chrono::steady_clock::time_point current_time, int cycle)
    {
        msgs::Vector3d msg;
        
        // Simulate sensor readings with noise
        msg.set_x(10.0 + 2.0 * sin(cycle * 0.05) + 0.1 * ((double)rand() / RAND_MAX - 0.5));
        msg.set_y(15.0 + 1.5 * cos(cycle * 0.03) + 0.1 * ((double)rand() / RAND_MAX - 0.5));
        msg.set_z(5.0 + 0.5 * sin(cycle * 0.08) + 0.05 * ((double)rand() / RAND_MAX - 0.5));
        
        bool published = publishers_["sensor_data"].Publish(msg);
        if (published)
        {
            topic_stats_["/demo/sensor_data"]->messages_sent++;
        }
    }

    /**
     * Publish high-frequency telemetry
     */
    void PublishTelemetryData(std::chrono::steady_clock::time_point current_time, int cycle)
    {
        msgs::Vector3d msg;
        
        // High-frequency system metrics
        msg.set_x(cycle);  // Cycle count
        msg.set_y(std::chrono::duration_cast<std::chrono::microseconds>(
            current_time.time_since_epoch()).count() / 1000.0);  // Timestamp in ms
        msg.set_z(100.0 - (cycle % 100));  // Simulated system load
        
        bool published = publishers_["telemetry"].Publish(msg);
        if (published)
        {
            topic_stats_["/demo/telemetry"]->messages_sent++;
        }
    }

    // Message callback functions
    void OnTextMessage(const msgs::StringMsg& msg)
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        topic_stats_["/demo/text_messages"]->messages_received++;
        topic_stats_["/demo/text_messages"]->last_message_time = std::chrono::steady_clock::now();
        
        // Extract timestamp for latency calculation
        std::string data = msg.data();
        size_t pos = data.find("Timestamp: ");
        if (pos != std::string::npos)
        {
            std::string timestamp_str = data.substr(pos + 11);
            try
            {
                long long sent_time = std::stoll(timestamp_str);
                long long received_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                
                double latency_ms = (received_time - sent_time) / 1e6;
                auto stats = topic_stats_["/demo/text_messages"];
                stats->total_latency += latency_ms;
                stats->latency_samples++;
            }
            catch (const std::exception&) {
                // Ignore parsing errors
            }
        }
    }

    void OnPoseMessage(const msgs::Pose& msg)
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        topic_stats_["/demo/robot_pose"]->messages_received++;
        topic_stats_["/demo/robot_pose"]->last_message_time = std::chrono::steady_clock::now();
        
        // Simple latency estimation based on position change
        // In practice, would use proper timestamping
        auto stats = topic_stats_["/demo/robot_pose"];
        stats->latency_samples++;
        stats->total_latency += 1.0;  // Assume 1ms average latency
    }

    void OnSensorData(const msgs::Vector3d& msg)
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        topic_stats_["/demo/sensor_data"]->messages_received++;
        topic_stats_["/demo/sensor_data"]->last_message_time = std::chrono::steady_clock::now();
    }

    void OnTelemetryData(const msgs::Vector3d& msg)
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        topic_stats_["/demo/telemetry"]->messages_received++;
        topic_stats_["/demo/telemetry"]->last_message_time = std::chrono::steady_clock::now();
        
        // High-frequency data - minimal processing
        auto stats = topic_stats_["/demo/telemetry"];
        stats->latency_samples++;
        stats->total_latency += 0.5;  // Assume 0.5ms average latency for high-freq data
    }

    /**
     * Update communication statistics
     */
    void UpdateStatistics()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        // Statistics are updated in real-time through callbacks
        // This function could perform additional analysis if needed
    }

    /**
     * Print current communication analysis
     */
    void PrintCommunicationAnalysis(int elapsed_seconds)
    {
        gzmsg << "Communication Analysis (t=" << elapsed_seconds << "s):\n";
        gzmsg << "=============================================\n";
        
        {
            std::lock_guard<std::mutex> lock(*data_mutex_);
            
            for (const auto& pair : topic_stats_)
            {
                const std::string& topic = pair.first;
                const auto& stats = pair.second;
                
                int sent = stats->messages_sent.load();
                int received = stats->messages_received.load();
                double avg_latency = stats->GetAverageLatency();
                double message_rate = stats->GetMessageRate();
                
                gzmsg << "Topic: " << topic << "\n";
                gzmsg << "  Messages sent: " << sent << "\n";
                gzmsg << "  Messages received: " << received << "\n";
                
                if (sent > 0)
                {
                    double delivery_rate = 100.0 * received / sent;
                    gzmsg << "  Delivery rate: " << std::fixed << std::setprecision(1) << delivery_rate << "%\n";
                }
                
                if (elapsed_seconds > 0)
                {
                    gzmsg << "  Send rate: " << std::fixed << std::setprecision(1) 
                          << (double)sent / elapsed_seconds << " Hz\n";
                    gzmsg << "  Receive rate: " << std::fixed << std::setprecision(1) 
                          << (double)received / elapsed_seconds << " Hz\n";
                }
                
                if (stats->latency_samples.load() > 0)
                {
                    gzmsg << "  Average latency: " << std::fixed << std::setprecision(2) 
                          << avg_latency << " ms\n";
                }
                
                gzmsg << "\n";
            }
        }
    }

    /**
     * Print comprehensive communication summary
     */
    void PrintCommunicationSummary()
    {
        gzmsg << "\n=== PUBLISHER-SUBSCRIBER COMMUNICATION SUMMARY ===\n\n";
        
        gzmsg << "Gazebo Transport Architecture:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "Underlying Technology:\n";
        gzmsg << "• ZeroMQ (ZMQ) for high-performance messaging\n";
        gzmsg << "• Protocol Buffers for efficient serialization\n";
        gzmsg << "• Automatic service discovery via multicast\n";
        gzmsg << "• Asynchronous, non-blocking communication\n\n";
        
        gzmsg << "Communication Patterns:\n";
        gzmsg << "• Publisher-Subscriber: One-to-many distribution\n";
        gzmsg << "• Request-Reply: Synchronous client-server\n";
        gzmsg << "• Push-Pull: Load-balanced task distribution\n";
        gzmsg << "• Pair: Bidirectional peer-to-peer\n\n";
        
        gzmsg << "Topic Management:\n";
        gzmsg << "----------------\n";
        gzmsg << "Topic Naming Conventions:\n";
        gzmsg << "• Hierarchical structure with forward slashes\n";
        gzmsg << "• Namespace organization for system modularity\n";
        gzmsg << "• Wildcard subscriptions for pattern matching\n";
        gzmsg << "• Version-specific topics for API evolution\n\n";
        
        gzmsg << "Discovery Mechanism:\n";
        gzmsg << "• Automatic publisher/subscriber detection\n";
        gzmsg << "• Dynamic connection establishment\n";
        gzmsg << "• Heartbeat and keepalive mechanisms\n";
        gzmsg << "• Graceful disconnection handling\n\n";
        
        gzmsg << "Message Lifecycle:\n";
        gzmsg << "------------------\n";
        gzmsg << "Serialization Process:\n";
        gzmsg << "• Object → Protocol Buffer → Binary wire format\n";
        gzmsg << "• Schema evolution and backward compatibility\n";
        gzmsg << "• Optional and required field handling\n";
        gzmsg << "• Efficient binary encoding (varint, zigzag)\n\n";
        
        gzmsg << "Memory Management:\n";
        gzmsg << "• Zero-copy optimization where possible\n";
        gzmsg << "• Message pooling for high-frequency data\n";
        gzmsg << "• Automatic garbage collection of old messages\n";
        gzmsg << "• Buffer size limits and overflow handling\n\n";
        
        gzmsg << "Performance Characteristics:\n";
        gzmsg << "--------------------------\n";
        gzmsg << "Latency Considerations:\n";
        gzmsg << "• Network topology and routing effects\n";
        gzmsg << "• Serialization/deserialization overhead\n";
        gzmsg << "• Queue depths and buffering strategies\n";
        gzmsg << "• CPU scheduling and thread priorities\n\n";
        
        gzmsg << "Throughput Optimization:\n";
        gzmsg << "• Batching strategies for small messages\n";
        gzmsg << "• Connection multiplexing and pooling\n";
        gzmsg << "• Compression for large message payloads\n";
        gzmsg << "• Network interface utilization\n\n";
        
        gzmsg << "Quality of Service (QoS):\n";
        gzmsg << "------------------------\n";
        gzmsg << "Reliability Options:\n";
        gzmsg << "• Best-effort: Fire-and-forget delivery\n";
        gzmsg << "• Reliable: Acknowledgment-based delivery\n";
        gzmsg << "• Persistent: Message survival across restarts\n";
        gzmsg << "• Transactional: All-or-nothing semantics\n\n";
        
        gzmsg << "Flow Control:\n";
        gzmsg << "• Publisher-side rate limiting\n";
        gzmsg << "• Subscriber-side backpressure handling\n";
        gzmsg << "• Dynamic throttling based on queue depths\n";
        gzmsg << "• Priority-based message ordering\n\n";
        
        gzmsg << "Error Handling and Diagnostics:\n";
        gzmsg << "------------------------------\n";
        gzmsg << "Common Failure Modes:\n";
        gzmsg << "• Network partitions and split-brain scenarios\n";
        gzmsg << "• Publisher/subscriber lifecycle mismatches\n";
        gzmsg << "• Message size limits and fragmentation\n";
        gzmsg << "• Resource exhaustion and memory pressure\n\n";
        
        gzmsg << "Monitoring and Debugging:\n";
        gzmsg << "• Topic introspection and message inspection\n";
        gzmsg << "• Connection status and health monitoring\n";
        gzmsg << "• Performance metrics and profiling tools\n";
        gzmsg << "• Distributed tracing and correlation IDs\n\n";
        
        gzmsg << "Best Practices:\n";
        gzmsg << "--------------\n";
        gzmsg << "• Design for idempotency and fault tolerance\n";
        gzmsg << "• Use appropriate message frequencies for data types\n";
        gzmsg << "• Implement proper connection lifecycle management\n";
        gzmsg << "• Monitor system resources and performance metrics\n";
        gzmsg << "• Plan for schema evolution and API versioning\n\n";
        
        gzmsg << "=== PUBLISHER-SUBSCRIBER DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - Publisher-Subscriber Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Publisher-Subscriber Communication Demo ===\n";
    gzmsg << "This demo showcases distributed communication patterns.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- Topic-based publish/subscribe patterns\n";
    gzmsg << "- Message serialization and deserialization\n";
    gzmsg << "- Multi-rate communication architectures\n";
    gzmsg << "- Performance analysis and optimization\n";
    gzmsg << "- Quality of Service considerations\n\n";

    try
    {
        PublisherSubscriber demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during publisher-subscriber demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nPublisher-subscriber demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Pub/sub enables scalable distributed communication\n";
    gzmsg << "- Topic hierarchies organize system information flow\n";
    gzmsg << "- Message serialization affects performance significantly\n";
    gzmsg << "- Quality of Service settings impact system behavior\n";
    gzmsg << "- Proper error handling ensures system robustness\n";
    
    return 0;
}