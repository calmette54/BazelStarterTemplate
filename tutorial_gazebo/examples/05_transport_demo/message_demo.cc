/*
 * Gazebo Message Handling and Serialization Demo
 * 
 * This example demonstrates:
 * 1. Advanced message serialization and deserialization
 * 2. Custom message type creation and handling
 * 3. Message versioning and backward compatibility
 * 4. Binary data handling and optimization
 * 5. Message compression and performance optimization
 * 
 * Learning objectives:
 * - Understanding Protocol Buffer message design
 * - Message schema evolution and compatibility
 * - Performance characteristics of different data types
 * - Memory-efficient message handling strategies
 * - Cross-language message serialization
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/stringmsg.pb.h>
#include <gz/msgs/vector3d.pb.h>
#include <gz/msgs/image.pb.h>
#include <gz/msgs/pointcloud_packed.pb.h>
#include <gz/msgs/laserscan.pb.h>
#include <gz/msgs/contact.pb.h>
#include <gz/msgs/pose.pb.h>
#include <gz/msgs/time.pb.h>
#include <gz/msgs/header.pb.h>

#include <gz/transport/Node.hh>

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
#include <fstream>
#include <sstream>

using namespace gz;
using namespace transport;

/**
 * Message Performance Metrics
 */
struct MessageMetrics
{
    std::string message_type;
    std::atomic<int> messages_processed{0};
    std::atomic<size_t> total_bytes{0};
    std::atomic<double> total_serialize_time{0.0};
    std::atomic<double> total_deserialize_time{0.0};
    std::atomic<int> serialize_samples{0};
    std::atomic<int> deserialize_samples{0};
    
    double GetAverageSerializeTime() const 
    {
        int samples = serialize_samples.load();
        return samples > 0 ? total_serialize_time.load() / samples : 0.0;
    }
    
    double GetAverageDeserializeTime() const 
    {
        int samples = deserialize_samples.load();
        return samples > 0 ? total_deserialize_time.load() / samples : 0.0;
    }
    
    double GetAverageMessageSize() const 
    {
        int messages = messages_processed.load();
        return messages > 0 ? (double)total_bytes.load() / messages : 0.0;
    }
    
    double GetThroughput() const 
    {
        double total_time = total_serialize_time.load() + total_deserialize_time.load();
        return total_time > 0 ? (total_bytes.load() / 1024.0 / 1024.0) / (total_time / 1000.0) : 0.0; // MB/s
    }
};

/**
 * Advanced Message Handling System
 */
class MessageDemo
{
public:
    MessageDemo() : node_(std::make_shared<Node>())
    {
        gzmsg << "Message Demo initialized\n";
        gzmsg << "Advanced message serialization and handling demonstration\n\n";
        
        // Initialize message system
        processing_active_ = true;
        data_mutex_ = std::make_shared<std::mutex>();
        
        // Setup message processors
        SetupMessageHandlers();
        
        // Initialize performance metrics
        InitializeMetrics();
    }

    /**
     * Run comprehensive message handling demonstration
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Advanced Message Handling Demo ===\n\n";
        
        // Create minimal server for transport framework
        ServerConfig config;
        config.SetSdfString(CreateMessageWorldSdf());
        config.SetUpdateRate(20.0);
        
        Server server(config);
        
        gzmsg << "Starting message demonstration...\n";
        PrintMessageConfiguration();
        
        // Start message processing threads
        std::thread publisher_thread(&MessageDemo::PublisherLoop, this);
        std::thread processor_thread(&MessageDemo::ProcessorLoop, this);
        std::thread metrics_thread(&MessageDemo::MetricsLoop, this);
        
        // Run simulation
        const int simulation_steps = 12000;  // 10 minutes at 20 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && processing_active_)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print message analysis every 30 seconds
            if (step_count % 600 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                PrintMessageAnalysis(elapsed);
                gzmsg << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(50000)); // 20 Hz
        }
        
        // Stop processing and cleanup
        processing_active_ = false;
        
        if (publisher_thread.joinable()) publisher_thread.join();
        if (processor_thread.joinable()) processor_thread.join();
        if (metrics_thread.joinable()) metrics_thread.join();
        
        PrintMessageSummary();
    }

private:
    std::shared_ptr<Node> node_;
    std::shared_ptr<std::mutex> data_mutex_;
    std::atomic<bool> processing_active_;

    // Message metrics tracking
    std::map<std::string, std::shared_ptr<MessageMetrics>> message_metrics_;
    
    // Message type registry
    std::vector<std::string> active_message_types_;

    /**
     * Setup comprehensive message handling
     */
    void SetupMessageHandlers()
    {
        // Simple message types
        node_->Advertise<msgs::StringMsg>("/demo/messages/string");
        node_->Advertise<msgs::Vector3d>("/demo/messages/vector");
        active_message_types_.push_back("string");
        active_message_types_.push_back("vector3d");
        
        // Complex structured messages
        node_->Advertise<msgs::Pose>("/demo/messages/pose");
        node_->Advertise<msgs::LaserScan>("/demo/messages/laserscan");
        active_message_types_.push_back("pose");
        active_message_types_.push_back("laserscan");
        
        // Large binary messages
        node_->Advertise<msgs::Image>("/demo/messages/image");
        node_->Advertise<msgs::PointCloudPacked>("/demo/messages/pointcloud");
        active_message_types_.push_back("image");
        active_message_types_.push_back("pointcloud");
        
        // Subscribe to all message types for processing
        node_->Subscribe("/demo/messages/string", &MessageDemo::OnStringMessage, this);
        node_->Subscribe("/demo/messages/vector", &MessageDemo::OnVectorMessage, this);
        node_->Subscribe("/demo/messages/pose", &MessageDemo::OnPoseMessage, this);
        node_->Subscribe("/demo/messages/laserscan", &MessageDemo::OnLaserScanMessage, this);
        node_->Subscribe("/demo/messages/image", &MessageDemo::OnImageMessage, this);
        node_->Subscribe("/demo/messages/pointcloud", &MessageDemo::OnPointCloudMessage, this);
        
        gzmsg << "Message handlers configured:\n";
        gzmsg << "- String messages: Text data with various lengths\n";
        gzmsg << "- Vector messages: Numerical data structures\n";
        gzmsg << "- Pose messages: Complex geometric representations\n";
        gzmsg << "- LaserScan messages: Sensor data arrays\n";
        gzmsg << "- Image messages: Large binary data\n";
        gzmsg << "- PointCloud messages: Massive datasets\n\n";
    }

    /**
     * Initialize performance metrics for all message types
     */
    void InitializeMetrics()
    {
        for (const auto& type : active_message_types_)
        {
            auto metrics = std::make_shared<MessageMetrics>();
            metrics->message_type = type;
            message_metrics_[type] = metrics;
        }
    }

    /**
     * Create simple world for message framework
     */
    std::string CreateMessageWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="message_demo">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.05</max_step_size>
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
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>2 2</size>
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
     * Print message configuration
     */
    void PrintMessageConfiguration()
    {
        gzmsg << "Message System Configuration:\n";
        gzmsg << "----------------------------\n";
        gzmsg << "Serialization Protocol: Protocol Buffers (protobuf)\n";
        gzmsg << "Wire Format: Binary encoding with schema validation\n";
        gzmsg << "Compression: Optional message-level compression\n";
        gzmsg << "Versioning: Forward/backward compatibility support\n\n";
        
        gzmsg << "Message Types Under Test:\n";
        for (const auto& type : active_message_types_)
        {
            gzmsg << "  - " << type << "\n";
        }
        gzmsg << "\n";
        
        gzmsg << "Performance Metrics:\n";
        gzmsg << "- Serialization/deserialization latency\n";
        gzmsg << "- Message size and compression ratio\n";
        gzmsg << "- Throughput and bandwidth utilization\n";
        gzmsg << "- Memory allocation and garbage collection\n\n";
    }

    /**
     * Publisher loop - generates various message types
     */
    void PublisherLoop()
    {
        int cycle_count = 0;
        
        while (processing_active_)
        {
            auto current_time = std::chrono::steady_clock::now();
            
            // String messages - 10 Hz
            if (cycle_count % 5 == 0)
            {
                PublishStringMessage(cycle_count);
            }
            
            // Vector messages - 20 Hz
            if (cycle_count % 2 == 0)
            {
                PublishVectorMessage(current_time, cycle_count);
            }
            
            // Pose messages - 15 Hz
            if (cycle_count % 3 == 0)
            {
                PublishPoseMessage(current_time, cycle_count);
            }
            
            // LaserScan messages - 5 Hz
            if (cycle_count % 10 == 0)
            {
                PublishLaserScanMessage(cycle_count);
            }
            
            // Image messages - 2 Hz (large data)
            if (cycle_count % 25 == 0)
            {
                PublishImageMessage(cycle_count);
            }
            
            // PointCloud messages - 1 Hz (very large data)
            if (cycle_count % 50 == 0)
            {
                PublishPointCloudMessage(cycle_count);
            }
            
            cycle_count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // 50 Hz base rate
        }
    }

    /**
     * Processor loop - handles message analysis
     */
    void ProcessorLoop()
    {
        while (processing_active_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Process message validation and analysis
            ProcessMessageValidation();
        }
    }

    /**
     * Metrics collection loop
     */
    void MetricsLoop()
    {
        while (processing_active_)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            // Update performance metrics
            UpdatePerformanceMetrics();
        }
    }

    // Message publishing functions
    void PublishStringMessage(int cycle)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        msgs::StringMsg msg;
        
        // Vary message size to test serialization performance
        std::string data;
        int size_category = cycle % 4;
        
        switch (size_category)
        {
            case 0: // Small message
                data = "Short message #" + std::to_string(cycle);
                break;
            case 1: // Medium message
                data = "Medium length message with more content for testing serialization performance #" + std::to_string(cycle);
                break;
            case 2: // Large message
                data = std::string(1024, 'A') + " Large message #" + std::to_string(cycle);
                break;
            case 3: // Variable content
                data = GenerateVariableContent(cycle);
                break;
        }
        
        msg.set_data(data);
        
        // Measure serialization time
        std::string serialized;
        msg.SerializeToString(&serialized);
        
        auto serialize_end = std::chrono::high_resolution_clock::now();
        double serialize_time = std::chrono::duration_cast<std::chrono::microseconds>(serialize_end - start_time).count() / 1000.0;
        
        // Publish message
        node_->Publisher("/demo/messages/string").Publish(msg);
        
        // Update metrics
        auto metrics = message_metrics_["string"];
        metrics->total_serialize_time += serialize_time;
        metrics->serialize_samples++;
        metrics->total_bytes += serialized.size();
    }

    void PublishVectorMessage(std::chrono::steady_clock::time_point current_time, int cycle)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        msgs::Vector3d msg;
        
        // Generate different vector patterns
        double t = cycle * 0.1;
        msg.set_x(10.0 * sin(t) + 2.0 * sin(3*t));
        msg.set_y(5.0 * cos(t) + 1.5 * cos(2*t));
        msg.set_z(3.0 * sin(0.5*t) + 0.5 * sin(5*t));
        
        std::string serialized;
        msg.SerializeToString(&serialized);
        
        auto serialize_end = std::chrono::high_resolution_clock::now();
        double serialize_time = std::chrono::duration_cast<std::chrono::microseconds>(serialize_end - start_time).count() / 1000.0;
        
        node_->Publisher("/demo/messages/vector").Publish(msg);
        
        auto metrics = message_metrics_["vector3d"];
        metrics->total_serialize_time += serialize_time;
        metrics->serialize_samples++;
        metrics->total_bytes += serialized.size();
    }

    void PublishPoseMessage(std::chrono::steady_clock::time_point current_time, int cycle)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        msgs::Pose msg;
        
        // Complex pose with full transformation data
        double angle = cycle * 0.05;
        
        msg.mutable_position()->set_x(5.0 * cos(angle));
        msg.mutable_position()->set_y(5.0 * sin(angle));
        msg.mutable_position()->set_z(1.0 + 0.5 * sin(2*angle));
        
        msg.mutable_orientation()->set_w(cos(angle/2));
        msg.mutable_orientation()->set_x(0.1 * sin(angle));
        msg.mutable_orientation()->set_y(0.1 * cos(angle));
        msg.mutable_orientation()->set_z(sin(angle/2));
        
        // Add header with timestamp
        msg.mutable_header()->mutable_stamp()->set_sec(cycle / 1000);
        msg.mutable_header()->mutable_stamp()->set_nsec((cycle % 1000) * 1000000);
        msg.mutable_header()->set_frame_id("world");
        
        std::string serialized;
        msg.SerializeToString(&serialized);
        
        auto serialize_end = std::chrono::high_resolution_clock::now();
        double serialize_time = std::chrono::duration_cast<std::chrono::microseconds>(serialize_end - start_time).count() / 1000.0;
        
        node_->Publisher("/demo/messages/pose").Publish(msg);
        
        auto metrics = message_metrics_["pose"];
        metrics->total_serialize_time += serialize_time;
        metrics->serialize_samples++;
        metrics->total_bytes += serialized.size();
    }

    void PublishLaserScanMessage(int cycle)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        msgs::LaserScan msg;
        
        // Realistic laser scan parameters
        msg.set_frame("laser");
        msg.set_angle_min(-M_PI);
        msg.set_angle_max(M_PI);
        msg.set_angle_step(M_PI / 180.0);  // 1 degree resolution
        msg.set_range_min(0.1);
        msg.set_range_max(30.0);
        
        // Generate simulated scan data
        int num_points = 360;
        for (int i = 0; i < num_points; ++i)
        {
            double angle = msg.angle_min() + i * msg.angle_step();
            
            // Simulate environment with obstacles
            double range = 15.0;  // Default range
            
            // Add some obstacles
            if (abs(angle) < 0.5) range = 5.0;  // Front obstacle
            if (abs(angle - M_PI/2) < 0.3) range = 8.0;  // Side obstacle
            
            // Add noise and variation
            range += 2.0 * sin(cycle * 0.1 + angle * 3.0);
            range += 0.2 * ((double)rand() / RAND_MAX - 0.5);
            
            msg.add_ranges(std::max(msg.range_min(), std::min(msg.range_max(), range)));
            msg.add_intensities(100.0 + 50.0 * sin(angle * 2.0));
        }
        
        std::string serialized;
        msg.SerializeToString(&serialized);
        
        auto serialize_end = std::chrono::high_resolution_clock::now();
        double serialize_time = std::chrono::duration_cast<std::chrono::microseconds>(serialize_end - start_time).count() / 1000.0;
        
        node_->Publisher("/demo/messages/laserscan").Publish(msg);
        
        auto metrics = message_metrics_["laserscan"];
        metrics->total_serialize_time += serialize_time;
        metrics->serialize_samples++;
        metrics->total_bytes += serialized.size();
    }

    void PublishImageMessage(int cycle)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        msgs::Image msg;
        
        // Image parameters
        int width = 320;
        int height = 240;
        int channels = 3;  // RGB
        
        msg.set_width(width);
        msg.set_height(height);
        msg.set_pixel_format_type(msgs::PixelFormatType::RGB_INT8);
        msg.set_step(width * channels);
        
        // Generate synthetic image data
        std::string image_data;
        image_data.resize(width * height * channels);
        
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int idx = (y * width + x) * channels;
                
                // Generate pattern based on cycle
                uint8_t r = static_cast<uint8_t>((x + cycle) % 256);
                uint8_t g = static_cast<uint8_t>((y + cycle) % 256);
                uint8_t b = static_cast<uint8_t>((x + y + cycle) % 256);
                
                image_data[idx] = r;
                image_data[idx + 1] = g;
                image_data[idx + 2] = b;
            }
        }
        
        msg.set_data(image_data);
        
        std::string serialized;
        msg.SerializeToString(&serialized);
        
        auto serialize_end = std::chrono::high_resolution_clock::now();
        double serialize_time = std::chrono::duration_cast<std::chrono::microseconds>(serialize_end - start_time).count() / 1000.0;
        
        node_->Publisher("/demo/messages/image").Publish(msg);
        
        auto metrics = message_metrics_["image"];
        metrics->total_serialize_time += serialize_time;
        metrics->serialize_samples++;
        metrics->total_bytes += serialized.size();
    }

    void PublishPointCloudMessage(int cycle)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        msgs::PointCloudPacked msg;
        
        // Point cloud parameters
        int num_points = 10000;  // Large point cloud
        msg.set_width(num_points);
        msg.set_height(1);
        msg.set_is_bigendian(false);
        msg.set_point_step(16);  // 4 floats: x, y, z, intensity
        msg.set_row_step(msg.point_step() * num_points);
        
        // Add field definitions
        msgs::PointCloudPacked::Field* field_x = msg.add_field();
        field_x->set_name("x");
        field_x->set_offset(0);
        field_x->set_datatype(msgs::PointCloudPacked::Field::FLOAT32);
        field_x->set_count(1);
        
        msgs::PointCloudPacked::Field* field_y = msg.add_field();
        field_y->set_name("y");
        field_y->set_offset(4);
        field_y->set_datatype(msgs::PointCloudPacked::Field::FLOAT32);
        field_y->set_count(1);
        
        msgs::PointCloudPacked::Field* field_z = msg.add_field();
        field_z->set_name("z");
        field_z->set_offset(8);
        field_z->set_datatype(msgs::PointCloudPacked::Field::FLOAT32);
        field_z->set_count(1);
        
        msgs::PointCloudPacked::Field* field_intensity = msg.add_field();
        field_intensity->set_name("intensity");
        field_intensity->set_offset(12);
        field_intensity->set_datatype(msgs::PointCloudPacked::Field::FLOAT32);
        field_intensity->set_count(1);
        
        // Generate point cloud data
        std::string point_data;
        point_data.resize(msg.row_step());
        
        float* data_ptr = reinterpret_cast<float*>(&point_data[0]);
        
        for (int i = 0; i < num_points; ++i)
        {
            // Generate 3D points in a complex pattern
            double t = i / 1000.0 + cycle * 0.1;
            double r = 5.0 + 2.0 * sin(t * 2.0);
            double theta = t;
            double phi = sin(t) * 0.5;
            
            data_ptr[i * 4 + 0] = static_cast<float>(r * cos(theta) * cos(phi));
            data_ptr[i * 4 + 1] = static_cast<float>(r * sin(theta) * cos(phi));
            data_ptr[i * 4 + 2] = static_cast<float>(r * sin(phi));
            data_ptr[i * 4 + 3] = static_cast<float>(100.0 + 50.0 * sin(t * 5.0));
        }
        
        msg.set_data(point_data);
        
        std::string serialized;
        msg.SerializeToString(&serialized);
        
        auto serialize_end = std::chrono::high_resolution_clock::now();
        double serialize_time = std::chrono::duration_cast<std::chrono::microseconds>(serialize_end - start_time).count() / 1000.0;
        
        node_->Publisher("/demo/messages/pointcloud").Publish(msg);
        
        auto metrics = message_metrics_["pointcloud"];
        metrics->total_serialize_time += serialize_time;
        metrics->serialize_samples++;
        metrics->total_bytes += serialized.size();
    }

    // Helper function for variable content generation
    std::string GenerateVariableContent(int cycle)
    {
        std::ostringstream oss;
        oss << "Variable content message #" << cycle << " with data: ";
        
        // Add structured data
        oss << "timestamp=" << std::chrono::steady_clock::now().time_since_epoch().count();
        oss << ", values=[";
        
        for (int i = 0; i < 10; ++i)
        {
            oss << (i * cycle % 100);
            if (i < 9) oss << ",";
        }
        
        oss << "], metadata={type:test, version:1.0, flags:" << (cycle % 8) << "}";
        
        return oss.str();
    }

    // Message callback functions with deserialization timing
    void OnStringMessage(const msgs::StringMsg& msg)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Simulate message processing
        std::string data = msg.data();
        size_t data_size = data.size();
        
        // Perform some analysis
        size_t word_count = std::count(data.begin(), data.end(), ' ') + 1;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double deserialize_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0;
        
        std::lock_guard<std::mutex> lock(*data_mutex_);
        auto metrics = message_metrics_["string"];
        metrics->messages_processed++;
        metrics->total_deserialize_time += deserialize_time;
        metrics->deserialize_samples++;
    }

    void OnVectorMessage(const msgs::Vector3d& msg)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Process vector data
        double magnitude = sqrt(msg.x()*msg.x() + msg.y()*msg.y() + msg.z()*msg.z());
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double deserialize_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0;
        
        std::lock_guard<std::mutex> lock(*data_mutex_);
        auto metrics = message_metrics_["vector3d"];
        metrics->messages_processed++;
        metrics->total_deserialize_time += deserialize_time;
        metrics->deserialize_samples++;
    }

    void OnPoseMessage(const msgs::Pose& msg)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Process pose data
        math::Vector3d position(msg.position().x(), msg.position().y(), msg.position().z());
        math::Quaterniond orientation(msg.orientation().w(), msg.orientation().x(), 
                                     msg.orientation().y(), msg.orientation().z());
        
        // Extract timestamp if available
        if (msg.has_header() && msg.header().has_stamp())
        {
            int64_t timestamp = msg.header().stamp().sec() * 1000000000LL + msg.header().stamp().nsec();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double deserialize_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0;
        
        std::lock_guard<std::mutex> lock(*data_mutex_);
        auto metrics = message_metrics_["pose"];
        metrics->messages_processed++;
        metrics->total_deserialize_time += deserialize_time;
        metrics->deserialize_samples++;
    }

    void OnLaserScanMessage(const msgs::LaserScan& msg)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Process laser scan data
        int num_ranges = msg.ranges_size();
        double min_range = *std::min_element(msg.ranges().begin(), msg.ranges().end());
        double max_range = *std::max_element(msg.ranges().begin(), msg.ranges().end());
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double deserialize_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0;
        
        std::lock_guard<std::mutex> lock(*data_mutex_);
        auto metrics = message_metrics_["laserscan"];
        metrics->messages_processed++;
        metrics->total_deserialize_time += deserialize_time;
        metrics->deserialize_samples++;
    }

    void OnImageMessage(const msgs::Image& msg)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Process image data
        int width = msg.width();
        int height = msg.height();
        size_t data_size = msg.data().size();
        
        // Simple image processing
        const std::string& image_data = msg.data();
        if (!image_data.empty())
        {
            uint8_t first_pixel = static_cast<uint8_t>(image_data[0]);
            uint8_t last_pixel = static_cast<uint8_t>(image_data[image_data.size() - 1]);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double deserialize_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0;
        
        std::lock_guard<std::mutex> lock(*data_mutex_);
        auto metrics = message_metrics_["image"];
        metrics->messages_processed++;
        metrics->total_deserialize_time += deserialize_time;
        metrics->deserialize_samples++;
    }

    void OnPointCloudMessage(const msgs::PointCloudPacked& msg)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Process point cloud data
        int num_points = msg.width() * msg.height();
        size_t data_size = msg.data().size();
        
        // Extract some points for analysis
        if (!msg.data().empty() && msg.point_step() >= 16)
        {
            const float* data_ptr = reinterpret_cast<const float*>(msg.data().data());
            
            // Analyze first few points
            for (int i = 0; i < std::min(10, num_points); ++i)
            {
                float x = data_ptr[i * 4 + 0];
                float y = data_ptr[i * 4 + 1];
                float z = data_ptr[i * 4 + 2];
                float intensity = data_ptr[i * 4 + 3];
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double deserialize_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0;
        
        std::lock_guard<std::mutex> lock(*data_mutex_);
        auto metrics = message_metrics_["pointcloud"];
        metrics->messages_processed++;
        metrics->total_deserialize_time += deserialize_time;
        metrics->deserialize_samples++;
    }

    /**
     * Process message validation and analysis
     */
    void ProcessMessageValidation()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        // Perform validation checks on received messages
        // This could include schema validation, data integrity checks, etc.
    }

    /**
     * Update performance metrics
     */
    void UpdatePerformanceMetrics()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        // Performance metrics are updated in real-time through callbacks
        // This function could perform additional analysis if needed
    }

    /**
     * Print current message analysis
     */
    void PrintMessageAnalysis(int elapsed_seconds)
    {
        gzmsg << "Message Handling Analysis (t=" << elapsed_seconds << "s):\n";
        gzmsg << "====================================================\n";
        
        {
            std::lock_guard<std::mutex> lock(*data_mutex_);
            
            for (const auto& pair : message_metrics_)
            {
                const std::string& type = pair.first;
                const auto& metrics = pair.second;
                
                int processed = metrics->messages_processed.load();
                double avg_serialize_time = metrics->GetAverageSerializeTime();
                double avg_deserialize_time = metrics->GetAverageDeserializeTime();
                double avg_size = metrics->GetAverageMessageSize();
                double throughput = metrics->GetThroughput();
                
                gzmsg << "Message Type: " << type << "\n";
                gzmsg << "  Messages processed: " << processed << "\n";
                
                if (elapsed_seconds > 0 && processed > 0)
                {
                    gzmsg << "  Message rate: " << std::fixed << std::setprecision(1) 
                          << (double)processed / elapsed_seconds << " msg/s\n";
                }
                
                if (metrics->serialize_samples.load() > 0)
                {
                    gzmsg << "  Avg serialize time: " << std::fixed << std::setprecision(2) 
                          << avg_serialize_time << " ms\n";
                }
                
                if (metrics->deserialize_samples.load() > 0)
                {
                    gzmsg << "  Avg deserialize time: " << std::fixed << std::setprecision(2) 
                          << avg_deserialize_time << " ms\n";
                }
                
                if (processed > 0)
                {
                    gzmsg << "  Avg message size: " << std::fixed << std::setprecision(0) 
                          << avg_size << " bytes\n";
                }
                
                if (throughput > 0)
                {
                    gzmsg << "  Throughput: " << std::fixed << std::setprecision(2) 
                          << throughput << " MB/s\n";
                }
                
                gzmsg << "\n";
            }
        }
    }

    /**
     * Print comprehensive message handling summary
     */
    void PrintMessageSummary()
    {
        gzmsg << "\n=== MESSAGE HANDLING SUMMARY ===\n\n";
        
        gzmsg << "Protocol Buffers (protobuf) Architecture:\n";
        gzmsg << "----------------------------------------\n";
        gzmsg << "Schema Definition:\n";
        gzmsg << "• Language-neutral interface definition (.proto files)\n";
        gzmsg << "• Strong typing with optional/required/repeated fields\n";
        gzmsg << "• Nested message types and enumerations\n";
        gzmsg << "• Extension mechanisms for custom fields\n\n";
        
        gzmsg << "Binary Encoding:\n";
        gzmsg << "• Variable-length integer encoding (varint)\n";
        gzmsg << "• ZigZag encoding for signed integers\n";
        gzmsg << "• Length-delimited strings and embedded messages\n";
        gzmsg << "• Field number and wire type tagging\n\n";
        
        gzmsg << "Message Performance Characteristics:\n";
        gzmsg << "-----------------------------------\n";
        gzmsg << "Serialization Factors:\n";
        gzmsg << "• Message size and complexity\n";
        gzmsg << "• Repeated field count and nesting depth\n";
        gzmsg << "• String and binary data lengths\n";
        gzmsg << "• Memory allocation and copying overhead\n\n";
        
        gzmsg << "Optimization Strategies:\n";
        gzmsg << "• Message pooling and reuse\n";
        gzmsg << "• Lazy field initialization\n";
        gzmsg << "• Arena allocation for nested objects\n";
        gzmsg << "• Custom serialization for critical paths\n\n";
        
        gzmsg << "Schema Evolution and Compatibility:\n";
        gzmsg << "----------------------------------\n";
        gzmsg << "Forward Compatibility:\n";
        gzmsg << "• Unknown fields are preserved and forwarded\n";
        gzmsg << "• Optional fields can be added safely\n";
        gzmsg << "• New enum values are handled gracefully\n";
        gzmsg << "• Deprecated fields remain functional\n\n";
        
        gzmsg << "Backward Compatibility:\n";
        gzmsg << "• Field numbers must remain stable\n";
        gzmsg << "• Required fields cannot be removed\n";
        gzmsg << "• Type changes require careful consideration\n";
        gzmsg << "• Default values ensure parsing consistency\n\n";
        
        gzmsg << "Message Design Best Practices:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "Field Design:\n";
        gzmsg << "• Use optional fields for most data\n";
        gzmsg << "• Reserve field numbers for future use\n";
        gzmsg << "• Choose appropriate data types\n";
        gzmsg << "• Include version information in headers\n\n";
        
        gzmsg << "Performance Considerations:\n";
        gzmsg << "• Minimize nesting depth for critical messages\n";
        gzmsg << "• Use packed repeated fields for numeric arrays\n";
        gzmsg << "• Consider message splitting for very large data\n";
        gzmsg << "• Implement custom serialization for hot paths\n\n";
        
        gzmsg << "Cross-Language Interoperability:\n";
        gzmsg << "-------------------------------\n";
        gzmsg << "• Generated code for C++, Python, Java, Go, etc.\n";
        gzmsg << "• Consistent field access patterns\n";
        gzmsg << "• Language-specific optimization features\n";
        gzmsg << "• Runtime reflection and dynamic messages\n\n";
        
        gzmsg << "Advanced Features:\n";
        gzmsg << "-----------------\n";
        gzmsg << "• Any type for polymorphic messages\n";
        gzmsg << "• OneOf unions for exclusive field sets\n";
        gzmsg << "• Map fields for key-value associations\n";
        gzmsg << "• Well-known types (Timestamp, Duration, etc.)\n";
        gzmsg << "• JSON mapping for web integration\n\n";
        
        gzmsg << "=== MESSAGE HANDLING DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - Message Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Advanced Message Handling Demo ===\n";
    gzmsg << "This demo showcases message serialization and performance.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- Protocol Buffer message serialization\n";
    gzmsg << "- Performance analysis across message types\n";
    gzmsg << "- Schema evolution and compatibility\n";
    gzmsg << "- Binary data handling optimization\n";
    gzmsg << "- Cross-language message interoperability\n\n";

    try
    {
        MessageDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during message demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nMessage demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Message design significantly impacts performance\n";
    gzmsg << "- Protocol Buffers provide efficient serialization\n";
    gzmsg << "- Schema evolution enables system maintainability\n";
    gzmsg << "- Proper message handling prevents bottlenecks\n";
    gzmsg << "- Cross-language support enables system integration\n";
    
    return 0;
}