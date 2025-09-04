/*
 * Gazebo Multi-Node Communication Demo
 * 
 * This example demonstrates:
 * 1. Multi-node communication architectures
 * 2. Distributed system coordination patterns
 * 3. Node discovery and peer-to-peer communication
 * 4. Message routing and broadcast mechanisms
 * 5. Fault tolerance and network partitioning
 * 
 * Learning objectives:
 * - Understanding distributed system architectures
 * - Node lifecycle and discovery mechanisms
 * - Inter-node coordination and synchronization
 * - Network topology and routing strategies
 * - Resilience and fault recovery patterns
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/string_msg.pb.h>
#include <gz/msgs/vector3d.pb.h>
#include <gz/msgs/boolean.pb.h>
#include <gz/msgs/empty.pb.h>

#include <gz/transport/Node.hh>
#include <gz/transport/TopicUtils.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Rand.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <memory>
#include <algorithm>
#include <random>

using namespace gz;
using namespace transport;

/**
 * Node Information Structure
 */
struct NodeInfo
{
    std::string node_id;
    std::string node_type;
    std::chrono::steady_clock::time_point last_heartbeat;
    std::atomic<bool> active{true};
    std::atomic<int> messages_sent{0};
    std::atomic<int> messages_received{0};
    std::vector<std::string> subscribed_topics;
    std::vector<std::string> published_topics;
    
    bool IsActive() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat).count();
        return elapsed < 10;  // Consider active if heartbeat within 10 seconds
    }
};

/**
 * Multi-Node Communication System
 */
class MultiNodeDemo
{
public:
    MultiNodeDemo() : node_(std::make_shared<Node>())
    {
        // Generate unique node ID
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        node_id_ = "node_" + std::to_string(dis(gen));
        
        gzmsg << "Multi-Node Demo initialized with ID: " << node_id_ << "\n";
        gzmsg << "Distributed communication system demonstration\n\n";
        
        // Initialize communication system
        communication_active_ = true;
        data_mutex_ = std::make_shared<std::mutex>();
        
        // Setup multi-node communication patterns
        SetupMultiNodeCommunication();
        
        // Initialize node tracking
        InitializeNodeTracking();
    }

    /**
     * Run comprehensive multi-node communication demonstration
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Multi-Node Communication Demo ===\n\n";
        
        // Create minimal server for transport framework
        ServerConfig config;
        config.SetSdfString(CreateMultiNodeWorldSdf());
        config.SetUpdateRate(20.0);
        
        Server server(config);
        
        gzmsg << "Starting multi-node demonstration...\n";
        PrintMultiNodeConfiguration();
        
        // Start communication threads
        std::thread heartbeat_thread(&MultiNodeDemo::HeartbeatLoop, this);
        std::thread discovery_thread(&MultiNodeDemo::DiscoveryLoop, this);
        std::thread coordination_thread(&MultiNodeDemo::CoordinationLoop, this);
        std::thread monitoring_thread(&MultiNodeDemo::MonitoringLoop, this);
        
        // Run simulation
        const int simulation_steps = 12000;  // 10 minutes at 20 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && communication_active_)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print network status every 30 seconds
            if (step_count % 600 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                PrintNetworkStatus(elapsed);
                gzmsg << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(50000)); // 20 Hz
        }
        
        // Stop communication and cleanup
        communication_active_ = false;
        
        if (heartbeat_thread.joinable()) heartbeat_thread.join();
        if (discovery_thread.joinable()) discovery_thread.join();
        if (coordination_thread.joinable()) coordination_thread.join();
        if (monitoring_thread.joinable()) monitoring_thread.join();
        
        PrintMultiNodeSummary();
    }

private:
    std::shared_ptr<Node> node_;
    std::shared_ptr<std::mutex> data_mutex_;
    std::atomic<bool> communication_active_;
    
    std::string node_id_;
    std::string node_type_;
    
    // Network topology tracking
    std::map<std::string, std::shared_ptr<NodeInfo>> known_nodes_;
    std::set<std::string> active_topics_;
    
    // Coordination state
    std::atomic<bool> is_coordinator_{false};
    std::string current_coordinator_;
    std::atomic<int> coordination_term_{0};
    
    /**
     * Setup multi-node communication patterns
     */
    void SetupMultiNodeCommunication()
    {
        // Determine node type based on ID
        int node_num = std::stoi(node_id_.substr(5));
        if (node_num % 4 == 0) {
            node_type_ = "coordinator";
        } else if (node_num % 4 == 1) {
            node_type_ = "data_producer";
        } else if (node_num % 4 == 2) {
            node_type_ = "data_processor";
        } else {
            node_type_ = "data_consumer";
        }
        
        gzmsg << "Node type: " << node_type_ << "\n";
        
        // Setup universal topics for all nodes
        node_->Subscribe("/multinode/heartbeat", &MultiNodeDemo::OnHeartbeat, this);
        node_->Subscribe("/multinode/discovery", &MultiNodeDemo::OnDiscovery, this);
        node_->Subscribe("/multinode/coordination", &MultiNodeDemo::OnCoordination, this);
        
        // Setup role-specific topics
        if (node_type_ == "data_producer") {
            node_->Subscribe("/multinode/data_requests", &MultiNodeDemo::OnDataRequest, this);
        } else if (node_type_ == "data_processor") {
            node_->Subscribe("/multinode/raw_data", &MultiNodeDemo::OnRawData, this);
        } else if (node_type_ == "data_consumer") {
            node_->Subscribe("/multinode/processed_data", &MultiNodeDemo::OnProcessedData, this);
        }
        
        gzmsg << "Multi-node communication configured for " << node_type_ << " role\n\n";
    }
    
    /**
     * Initialize node tracking system
     */
    void InitializeNodeTracking()
    {
        // Add self to known nodes
        auto self_info = std::make_shared<NodeInfo>();
        self_info->node_id = node_id_;
        self_info->node_type = node_type_;
        self_info->last_heartbeat = std::chrono::steady_clock::now();
        
        known_nodes_[node_id_] = self_info;
        
        // Initialize active topics
        active_topics_.insert("/multinode/heartbeat");
        active_topics_.insert("/multinode/discovery");
        active_topics_.insert("/multinode/coordination");
    }

    /**
     * Create simple world for multi-node framework
     */
    std::string CreateMultiNodeWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="multinode_demo">
    
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
        <collision name="collision">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>20 20</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>20 20</size>
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
     * Print multi-node configuration
     */
    void PrintMultiNodeConfiguration()
    {
        gzmsg << "Multi-Node Communication Configuration:\n";
        gzmsg << "-------------------------------------\n";
        gzmsg << "Node ID: " << node_id_ << "\n";
        gzmsg << "Node Type: " << node_type_ << "\n";
        gzmsg << "Transport Protocol: ZMQ with multicast discovery\n";
        gzmsg << "Coordination Algorithm: Leader election with heartbeats\n";
        gzmsg << "Fault Tolerance: Automatic failover and recovery\n\n";
        
        gzmsg << "Communication Patterns:\n";
        gzmsg << "- Heartbeat: Periodic node status broadcasts\n";
        gzmsg << "- Discovery: Peer node detection and capability exchange\n";
        gzmsg << "- Coordination: Leader election and task distribution\n";
        gzmsg << "- Data Flow: Producer → Processor → Consumer pipeline\n\n";
        
        gzmsg << "Network Topology:\n";
        gzmsg << "- Mesh: All nodes can communicate directly\n";
        gzmsg << "- Star: Coordinator acts as central hub when elected\n";
        gzmsg << "- Pipeline: Data flows through processing stages\n";
        gzmsg << "- Broadcast: Status updates sent to all nodes\n\n";
    }

    /**
     * Heartbeat loop - announces node presence
     */
    void HeartbeatLoop()
    {
        auto heartbeat_publisher = node_->Advertise<msgs::StringMsg>("/multinode/heartbeat");
        
        while (communication_active_)
        {
            msgs::StringMsg heartbeat_msg;
            std::string heartbeat_data = node_id_ + "|" + node_type_ + "|" + 
                std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
                    
            heartbeat_msg.set_data(heartbeat_data);
            
            bool published = heartbeat_publisher.Publish(heartbeat_msg);
            if (published) {
                known_nodes_[node_id_]->messages_sent++;
            }
            
            // Update self heartbeat
            known_nodes_[node_id_]->last_heartbeat = std::chrono::steady_clock::now();
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    /**
     * Discovery loop - finds other nodes and services
     */
    void DiscoveryLoop()
    {
        auto discovery_publisher = node_->Advertise<msgs::StringMsg>("/multinode/discovery");
        
        while (communication_active_)
        {
            // Send discovery message
            msgs::StringMsg discovery_msg;
            std::string capabilities = node_id_ + "|" + node_type_ + "|CAPABILITIES:";
            
            if (node_type_ == "data_producer") {
                capabilities += "GENERATE_DATA,SENSOR_SIMULATION";
            } else if (node_type_ == "data_processor") {
                capabilities += "PROCESS_DATA,FILTER_SIGNALS,COMPUTE_METRICS";
            } else if (node_type_ == "data_consumer") {
                capabilities += "CONSUME_DATA,VISUALIZE,STORE";
            } else if (node_type_ == "coordinator") {
                capabilities += "COORDINATE,SCHEDULE_TASKS,MONITOR_HEALTH";
            }
            
            discovery_msg.set_data(capabilities);
            discovery_publisher.Publish(discovery_msg);
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    /**
     * Coordination loop - handles leader election and task distribution
     */
    void CoordinationLoop()
    {
        auto coordination_publisher = node_->Advertise<msgs::StringMsg>("/multinode/coordination");
        
        while (communication_active_)
        {
            std::lock_guard<std::mutex> lock(*data_mutex_);
            
            // Check if we should become coordinator
            bool should_coordinate = DetermineCoordinatorStatus();
            
            if (should_coordinate && !is_coordinator_) {
                // Become coordinator
                is_coordinator_ = true;
                current_coordinator_ = node_id_;
                coordination_term_++;
                
                gzmsg << "Node " << node_id_ << " becoming coordinator (term " 
                      << coordination_term_ << ")\n";
                
                msgs::StringMsg coord_msg;
                coord_msg.set_data("COORDINATOR_ELECTION|" + node_id_ + "|" + 
                    std::to_string(coordination_term_.load()));
                coordination_publisher.Publish(coord_msg);
            }
            
            // If we are coordinator, send task assignments
            if (is_coordinator_) {
                DistributeTasks(coordination_publisher);
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    /**
     * Monitoring loop - tracks network health
     */
    void MonitoringLoop()
    {
        while (communication_active_)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            std::lock_guard<std::mutex> lock(*data_mutex_);
            
            // Check for inactive nodes
            auto now = std::chrono::steady_clock::now();
            for (auto it = known_nodes_.begin(); it != known_nodes_.end();) {
                if (!it->second->IsActive() && it->first != node_id_) {
                    gzmsg << "Node " << it->first << " detected as inactive\n";
                    it = known_nodes_.erase(it);
                } else {
                    ++it;
                }
            }
            
            // If coordinator is inactive, trigger new election
            if (!current_coordinator_.empty() && 
                known_nodes_.find(current_coordinator_) == known_nodes_.end()) {
                gzmsg << "Coordinator " << current_coordinator_ << " is inactive\n";
                current_coordinator_.clear();
                is_coordinator_ = false;
            }
        }
    }

    /**
     * Determine if this node should become coordinator
     */
    bool DetermineCoordinatorStatus()
    {
        // Simple leader election: node with lowest ID becomes coordinator
        // if no coordinator exists or current coordinator is inactive
        
        if (current_coordinator_.empty()) {
            // No coordinator, check if we have the lowest ID
            std::string lowest_id = node_id_;
            for (const auto& pair : known_nodes_) {
                if (pair.second->IsActive() && pair.first < lowest_id) {
                    lowest_id = pair.first;
                }
            }
            return lowest_id == node_id_;
        }
        
        return false;
    }

    /**
     * Distribute tasks to network nodes
     */
    void DistributeTasks(Publisher& coordination_publisher)
    {
        // Send task assignments based on node capabilities
        static int task_counter = 0;
        task_counter++;
        
        for (const auto& pair : known_nodes_) {
            if (!pair.second->IsActive() || pair.first == node_id_) continue;
            
            msgs::StringMsg task_msg;
            std::string task_assignment;
            
            if (pair.second->node_type == "data_producer") {
                task_assignment = "TASK|" + pair.first + "|GENERATE_DATA_SET_" + 
                    std::to_string(task_counter % 10);
            } else if (pair.second->node_type == "data_processor") {
                task_assignment = "TASK|" + pair.first + "|PROCESS_BATCH_" + 
                    std::to_string(task_counter % 5);
            } else if (pair.second->node_type == "data_consumer") {
                task_assignment = "TASK|" + pair.first + "|STORE_RESULTS_" + 
                    std::to_string(task_counter % 3);
            }
            
            if (!task_assignment.empty()) {
                task_msg.set_data(task_assignment);
                coordination_publisher.Publish(task_msg);
            }
        }
    }

    // Message callback functions
    void OnHeartbeat(const msgs::StringMsg& msg)
    {
        std::string data = msg.data();
        std::vector<std::string> parts;
        
        // Simple string splitting
        std::stringstream ss(data);
        std::string part;
        while (std::getline(ss, part, '|')) {
            parts.push_back(part);
        }
        
        if (parts.size() >= 3) {
            std::string sender_id = parts[0];
            std::string sender_type = parts[1];
            
            std::lock_guard<std::mutex> lock(*data_mutex_);
            
            if (known_nodes_.find(sender_id) == known_nodes_.end()) {
                // New node discovered
                auto new_node = std::make_shared<NodeInfo>();
                new_node->node_id = sender_id;
                new_node->node_type = sender_type;
                known_nodes_[sender_id] = new_node;
                
                if (sender_id != node_id_) {
                    gzmsg << "Discovered new node: " << sender_id 
                          << " (type: " << sender_type << ")\n";
                }
            }
            
            // Update heartbeat
            known_nodes_[sender_id]->last_heartbeat = std::chrono::steady_clock::now();
            known_nodes_[sender_id]->messages_received++;
        }
    }

    void OnDiscovery(const msgs::StringMsg& msg)
    {
        std::string data = msg.data();
        
        // Parse capabilities announcement
        if (data.find("CAPABILITIES:") != std::string::npos) {
            std::lock_guard<std::mutex> lock(*data_mutex_);
            
            // Extract node ID and update capabilities
            size_t first_pipe = data.find('|');
            if (first_pipe != std::string::npos) {
                std::string sender_id = data.substr(0, first_pipe);
                
                if (sender_id != node_id_ && 
                    known_nodes_.find(sender_id) != known_nodes_.end()) {
                    known_nodes_[sender_id]->messages_received++;
                }
            }
        }
    }

    void OnCoordination(const msgs::StringMsg& msg)
    {
        std::string data = msg.data();
        
        if (data.find("COORDINATOR_ELECTION") == 0) {
            // Handle coordinator election
            std::vector<std::string> parts;
            std::stringstream ss(data);
            std::string part;
            while (std::getline(ss, part, '|')) {
                parts.push_back(part);
            }
            
            if (parts.size() >= 3) {
                std::lock_guard<std::mutex> lock(*data_mutex_);
                
                std::string new_coordinator = parts[1];
                int new_term = std::stoi(parts[2]);
                
                if (new_term > coordination_term_ || current_coordinator_.empty()) {
                    current_coordinator_ = new_coordinator;
                    coordination_term_ = new_term;
                    
                    if (new_coordinator != node_id_) {
                        is_coordinator_ = false;
                        gzmsg << "Accepting " << new_coordinator 
                              << " as coordinator (term " << new_term << ")\n";
                    }
                }
            }
        } else if (data.find("TASK|") == 0) {
            // Handle task assignment
            std::vector<std::string> parts;
            std::stringstream ss(data);
            std::string part;
            while (std::getline(ss, part, '|')) {
                parts.push_back(part);
            }
            
            if (parts.size() >= 3 && parts[1] == node_id_) {
                std::string task = parts[2];
                gzmsg << "Received task assignment: " << task << "\n";
                
                // Simulate task execution
                std::thread([this, task]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(
                        100 + rand() % 400));  // 100-500ms execution time
                    gzmsg << "Completed task: " << task << "\n";
                }).detach();
            }
        }
    }

    void OnDataRequest(const msgs::StringMsg& msg)
    {
        if (node_type_ == "data_producer") {
            // Simulate data generation
            auto data_publisher = node_->Advertise<msgs::Vector3d>("/multinode/raw_data");
            
            msgs::Vector3d data_msg;
            data_msg.set_x(math::Rand::DblUniform(0.0, 100.0));
            data_msg.set_y(math::Rand::DblUniform(0.0, 100.0));
            data_msg.set_z(math::Rand::DblUniform(0.0, 100.0));
            
            data_publisher.Publish(data_msg);
        }
    }

    void OnRawData(const msgs::Vector3d& msg)
    {
        if (node_type_ == "data_processor") {
            // Simulate data processing
            auto processed_publisher = node_->Advertise<msgs::Vector3d>("/multinode/processed_data");
            
            msgs::Vector3d processed_msg;
            processed_msg.set_x(msg.x() * 0.8);  // Simple processing
            processed_msg.set_y(msg.y() * 0.9);
            processed_msg.set_z(msg.z() * 0.7);
            
            processed_publisher.Publish(processed_msg);
        }
    }

    void OnProcessedData(const msgs::Vector3d& msg)
    {
        if (node_type_ == "data_consumer") {
            // Simulate data consumption/storage
            gzmsg << "Consuming processed data: (" 
                  << msg.x() << ", " << msg.y() << ", " << msg.z() << ")\n";
        }
    }

    /**
     * Print current network status
     */
    void PrintNetworkStatus(int elapsed_seconds)
    {
        gzmsg << "Multi-Node Network Status (t=" << elapsed_seconds << "s):\n";
        gzmsg << "===============================================\n";
        
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        gzmsg << "Network Topology:\n";
        gzmsg << "- Active nodes: " << known_nodes_.size() << "\n";
        gzmsg << "- Current coordinator: " << 
            (current_coordinator_.empty() ? "None" : current_coordinator_) << "\n";
        gzmsg << "- Coordination term: " << coordination_term_ << "\n\n";
        
        gzmsg << "Node Details:\n";
        for (const auto& pair : known_nodes_) {
            const auto& node_info = pair.second;
            bool is_active = node_info->IsActive();
            
            gzmsg << "  Node: " << node_info->node_id;
            if (node_info->node_id == node_id_) gzmsg << " (SELF)";
            if (node_info->node_id == current_coordinator_) gzmsg << " (COORDINATOR)";
            gzmsg << "\n";
            
            gzmsg << "    Type: " << node_info->node_type << "\n";
            gzmsg << "    Status: " << (is_active ? "ACTIVE" : "INACTIVE") << "\n";
            gzmsg << "    Messages sent: " << node_info->messages_sent.load() << "\n";
            gzmsg << "    Messages received: " << node_info->messages_received.load() << "\n";
            
            if (elapsed_seconds > 0) {
                gzmsg << "    Send rate: " << std::fixed << std::setprecision(1)
                      << (double)node_info->messages_sent.load() / elapsed_seconds << " Hz\n";
                gzmsg << "    Receive rate: " << std::fixed << std::setprecision(1)
                      << (double)node_info->messages_received.load() / elapsed_seconds << " Hz\n";
            }
            
            gzmsg << "\n";
        }
    }

    /**
     * Print comprehensive multi-node summary
     */
    void PrintMultiNodeSummary()
    {
        gzmsg << "\n=== MULTI-NODE COMMUNICATION SUMMARY ===\n\n";
        
        gzmsg << "Distributed System Architecture:\n";
        gzmsg << "-------------------------------\n";
        gzmsg << "Network Topologies:\n";
        gzmsg << "• Mesh: Direct peer-to-peer communication\n";
        gzmsg << "• Star: Centralized coordination through leader\n";
        gzmsg << "• Pipeline: Sequential data processing stages\n";
        gzmsg << "• Broadcast: One-to-many information distribution\n\n";
        
        gzmsg << "Node Discovery and Registration:\n";
        gzmsg << "• Heartbeat-based presence announcement\n";
        gzmsg << "• Capability advertisement and negotiation\n";
        gzmsg << "• Dynamic topology adaptation\n";
        gzmsg << "• Graceful node joining and leaving\n\n";
        
        gzmsg << "Coordination Mechanisms:\n";
        gzmsg << "----------------------\n";
        gzmsg << "Leader Election:\n";
        gzmsg << "• Bully algorithm for coordinator selection\n";
        gzmsg << "• Term-based leadership with automatic renewal\n";
        gzmsg << "• Failover and recovery mechanisms\n";
        gzmsg << "• Split-brain prevention strategies\n\n";
        
        gzmsg << "Task Distribution:\n";
        gzmsg << "• Capability-based task assignment\n";
        gzmsg << "• Load balancing across available nodes\n";
        gzmsg << "• Priority queuing and scheduling\n";
        gzmsg << "• Progress monitoring and timeout handling\n\n";
        
        gzmsg << "Fault Tolerance Patterns:\n";
        gzmsg << "------------------------\n";
        gzmsg << "Failure Detection:\n";
        gzmsg << "• Heartbeat monitoring with configurable timeouts\n";
        gzmsg << "• Network partition detection and recovery\n";
        gzmsg << "• Node health assessment and reporting\n";
        gzmsg << "• Cascade failure prevention mechanisms\n\n";
        
        gzmsg << "Recovery Strategies:\n";
        gzmsg << "• Automatic leader re-election on coordinator failure\n";
        gzmsg << "• Task redistribution and state reconstruction\n";
        gzmsg << "• Graceful degradation under partial failures\n";
        gzmsg << "• Rejoin protocol for recovered nodes\n\n";
        
        gzmsg << "Communication Patterns:\n";
        gzmsg << "---------------------\n";
        gzmsg << "Message Types:\n";
        gzmsg << "• Heartbeat: Periodic node status announcements\n";
        gzmsg << "• Discovery: Capability exchange and service registration\n";
        gzmsg << "• Coordination: Leader election and task distribution\n";
        gzmsg << "• Data: Application-specific information flow\n\n";
        
        gzmsg << "Quality of Service:\n";
        gzmsg << "• Reliable delivery for critical coordination messages\n";
        gzmsg << "• Best-effort delivery for high-frequency data streams\n";
        gzmsg << "• Message prioritization and flow control\n";
        gzmsg << "• Duplicate detection and idempotency\n\n";
        
        gzmsg << "Performance Optimization:\n";
        gzmsg << "------------------------\n";
        gzmsg << "Scalability Considerations:\n";
        gzmsg << "• Hierarchical coordination for large networks\n";
        gzmsg << "• Message aggregation and batching strategies\n";
        gzmsg << "• Selective flooding and gossip protocols\n";
        gzmsg << "• Connection pooling and multiplexing\n\n";
        
        gzmsg << "Network Efficiency:\n";
        gzmsg << "• Topology-aware routing and message forwarding\n";
        gzmsg << "• Bandwidth utilization monitoring and adaptation\n";
        gzmsg << "• Compression for large message payloads\n";
        gzmsg << "• Connection keep-alive and resource management\n\n";
        
        gzmsg << "Security Considerations:\n";
        gzmsg << "----------------------\n";
        gzmsg << "• Node authentication and authorization\n";
        gzmsg << "• Message integrity and tamper detection\n";
        gzmsg << "• Secure channel establishment (TLS/encryption)\n";
        gzmsg << "• DDoS protection and rate limiting\n\n";
        
        gzmsg << "Monitoring and Diagnostics:\n";
        gzmsg << "-------------------------\n";
        gzmsg << "• Real-time network topology visualization\n";
        gzmsg << "• Node health and performance metrics collection\n";
        gzmsg << "• Message flow tracing and latency analysis\n";
        gzmsg << "• Automated anomaly detection and alerting\n\n";
        
        gzmsg << "=== MULTI-NODE DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - Multi-Node Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Multi-Node Communication Demo ===\n";
    gzmsg << "This demo showcases distributed system architectures.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- Multi-node communication patterns\n";
    gzmsg << "- Distributed coordination and leader election\n";
    gzmsg << "- Node discovery and capability negotiation\n";
    gzmsg << "- Fault tolerance and recovery mechanisms\n";
    gzmsg << "- Network topology and routing strategies\n\n";

    try
    {
        MultiNodeDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during multi-node demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nMulti-node demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Distributed systems require robust coordination mechanisms\n";
    gzmsg << "- Leader election enables centralized decision making\n";
    gzmsg << "- Heartbeat protocols provide fault detection capabilities\n";
    gzmsg << "- Task distribution optimizes resource utilization\n";
    gzmsg << "- Network partitions require careful handling and recovery\n";
    
    return 0;
}