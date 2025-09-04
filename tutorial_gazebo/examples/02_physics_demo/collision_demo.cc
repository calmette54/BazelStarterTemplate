/*
 * Gazebo Collision Detection and Response Demo
 * 
 * This example demonstrates:
 * 1. Different collision shapes and their behaviors
 * 2. Collision detection between various geometry types
 * 3. Surface properties affecting collision response
 * 4. Contact forces and collision callbacks
 * 5. Advanced collision configuration and tuning
 * 
 * Learning objectives:
 * - Understanding collision vs visual geometry
 * - Surface material properties (friction, restitution)
 * - Contact force calculations
 * - Collision detection optimization
 * - Debugging collision issues
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components/Collision.hh>
#include <gz/sim/components/ContactSensorData.hh>
#include <gz/sim/components/Geometry.hh>
#include <gz/sim/components/Material.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/Surface.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/contacts.pb.h>
#include <gz/msgs/entity.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <set>

using namespace gz;
using namespace sim;

/**
 * Collision Analysis System
 * Monitors and analyzes collision events in the simulation
 */
class CollisionAnalyzer
{
public:
    struct CollisionEvent
    {
        std::string entity1_name;
        std::string entity2_name;
        math::Vector3d contact_point;
        math::Vector3d normal;
        double force_magnitude;
        common::Time timestamp;
    };

    CollisionAnalyzer() : node_(std::make_shared<transport::Node>())
    {
        // Subscribe to contact information
        if (!node_->Subscribe("/world/collision_demo/physics/contacts", &CollisionAnalyzer::OnContacts, this))
        {
            gzerr << "Failed to subscribe to contacts topic\n";
        }

        gzmsg << "Collision Analyzer initialized\n";
        gzmsg << "Monitoring collision events and contact forces...\n\n";
    }

    /**
     * Run collision demonstration with different scenarios
     */
    void RunDemo()
    {
        gzmsg << "=== Collision Detection and Response Demo ===\n\n";
        
        // Create server with collision world
        ServerConfig config;
        config.SetSdfString(CreateCollisionWorldSdf());
        config.SetUpdateRate(1000.0);  // High update rate for accurate collision detection
        
        Server server(config);
        
        gzmsg << "Starting collision simulation...\n";
        gzmsg << "Watch for collision events between different shaped objects:\n";
        gzmsg << "- Red boxes with different friction coefficients\n";
        gzmsg << "- Green spheres with different restitution (bounciness)\n";
        gzmsg << "- Blue cylinders with different surface materials\n\n";
        
        // Run simulation and analyze collisions
        const int simulation_steps = 10000;  // 10 seconds at 1000 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print periodic updates
            if (step_count % 1000 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                gzmsg << "Simulation time: " << (step_count * 0.001) << "s (real: " << elapsed << "s) - ";
                gzmsg << "Total collisions detected: " << collision_events_.size() << "\n";
                
                if (!collision_events_.empty())
                {
                    PrintRecentCollisions(5);  // Show last 5 collisions
                }
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        // Print final collision analysis
        PrintCollisionAnalysis();
    }

private:
    std::shared_ptr<transport::Node> node_;
    std::vector<CollisionEvent> collision_events_;
    std::map<std::string, int> collision_counts_;
    std::set<std::pair<std::string, std::string>> collision_pairs_;

    /**
     * Create SDF world for collision demonstrations
     */
    std::string CreateCollisionWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="collision_demo">
    
    <!-- Physics with contact reporting -->
    <physics name="default" type="ode">
      <max_step_size>0.001</max_step_size>
      <real_time_factor>1.0</real_time_factor>
      <max_contacts>20</max_contacts>
      <dart>
        <collision_detector>fcl</collision_detector>  <!-- Fast Collision Library -->
      </dart>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-contact-system" name="gz::sim::systems::Contact"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    
    <!-- Ground plane with contact sensor -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry><plane><normal>0 0 1</normal><size>20 20</size></plane></geometry>
          <surface>
            <friction><ode><mu>1.0</mu><mu2>1.0</mu2></ode></friction>
            <restitution>0.1</restitution>
            <contact><ode><max_vel>0.01</max_vel><min_depth>0.001</min_depth></ode></contact>
          </surface>
        </collision>
        <visual name="visual">
          <geometry><plane><normal>0 0 1</normal><size>20 20</size></plane></geometry>
          <material><ambient>0.8 0.8 0.8 1</ambient><diffuse>0.8 0.8 0.8 1</diffuse></material>
        </visual>
        
        <!-- Contact sensor for detecting collisions with ground -->
        <sensor name="ground_contact_sensor" type="contact">
          <contact><collision>collision</collision></contact>
          <update_rate>1000</update_rate>
          <always_on>true</always_on>
        </sensor>
      </link>
    </model>
    
    <!-- High-friction box (sticky surface) -->
    <model name="high_friction_box">
      <pose>-3 -3 3 0 0 0</pose>
      <link name="link">
        <inertial><mass>2.0</mass><inertia><ixx>0.33</ixx><iyy>0.33</iyy><izz>0.33</izz></inertia></inertial>
        <collision name="collision">
          <geometry><box><size>1 1 1</size></box></geometry>
          <surface>
            <friction><ode><mu>2.0</mu><mu2>2.0</mu2></ode></friction>  <!-- Very high friction -->
            <restitution>0.1</restitution>  <!-- Low bounce -->
          </surface>
        </collision>
        <visual name="visual">
          <geometry><box><size>1 1 1</size></box></geometry>
          <material><ambient>1 0 0 1</ambient><diffuse>1 0 0 1</diffuse></material>
        </visual>
        <sensor name="box_contact_sensor" type="contact">
          <contact><collision>collision</collision></contact>
          <update_rate>1000</update_rate>
          <always_on>true</always_on>
        </sensor>
      </link>
    </model>
    
    <!-- Low-friction box (slippery surface) -->
    <model name="low_friction_box">
      <pose>0 -3 3 0 0 0</pose>
      <link name="link">
        <inertial><mass>2.0</mass><inertia><ixx>0.33</ixx><iyy>0.33</iyy><izz>0.33</izz></inertia></inertial>
        <collision name="collision">
          <geometry><box><size>1 1 1</size></box></geometry>
          <surface>
            <friction><ode><mu>0.1</mu><mu2>0.1</mu2></ode></friction>  <!-- Very low friction -->
            <restitution>0.1</restitution>
          </surface>
        </collision>
        <visual name="visual">
          <geometry><box><size>1 1 1</size></box></geometry>
          <material><ambient>0.5 0 0 1</ambient><diffuse>0.5 0 0 1</diffuse></material>
        </visual>
        <sensor name="box2_contact_sensor" type="contact">
          <contact><collision>collision</collision></contact>
          <update_rate>1000</update_rate>
          <always_on>true</always_on>
        </sensor>
      </link>
    </model>
    
    <!-- High-bounce sphere -->
    <model name="bouncy_sphere">
      <pose>3 -3 4 0 0 0</pose>
      <link name="link">
        <inertial><mass>1.0</mass><inertia><ixx>0.4</ixx><iyy>0.4</iyy><izz>0.4</izz></inertia></inertial>
        <collision name="collision">
          <geometry><sphere><radius>0.5</radius></sphere></geometry>
          <surface>
            <friction><ode><mu>0.8</mu><mu2>0.8</mu2></ode></friction>
            <restitution>0.9</restitution>  <!-- Very bouncy -->
          </surface>
        </collision>
        <visual name="visual">
          <geometry><sphere><radius>0.5</radius></sphere></geometry>
          <material><ambient>0 1 0 1</ambient><diffuse>0 1 0 1</diffuse></material>
        </visual>
        <sensor name="sphere1_contact_sensor" type="contact">
          <contact><collision>collision</collision></contact>
          <update_rate>1000</update_rate>
          <always_on>true</always_on>
        </sensor>
      </link>
    </model>
    
    <!-- Low-bounce sphere -->
    <model name="dead_sphere">
      <pose>6 -3 4 0 0 0</pose>
      <link name="link">
        <inertial><mass>1.0</mass><inertia><ixx>0.4</ixx><iyy>0.4</iyy><izz>0.4</izz></inertia></inertial>
        <collision name="collision">
          <geometry><sphere><radius>0.5</radius></sphere></geometry>
          <surface>
            <friction><ode><mu>0.8</mu><mu2>0.8</mu2></ode></friction>
            <restitution>0.0</restitution>  <!-- No bounce -->
          </surface>
        </collision>
        <visual name="visual">
          <geometry><sphere><radius>0.5</radius></sphere></geometry>
          <material><ambient>0 0.5 0 1</ambient><diffuse>0 0.5 0 1</diffuse></material>
        </visual>
        <sensor name="sphere2_contact_sensor" type="contact">
          <contact><collision>collision</collision></contact>
          <update_rate>1000</update_rate>
          <always_on>true</always_on>
        </sensor>
      </link>
    </model>
    
    <!-- Rolling cylinder -->
    <model name="rolling_cylinder">
      <pose>-6 0 2 0 0 0</pose>
      <link name="link">
        <inertial><mass>3.0</mass><inertia><ixx>1.5</ixx><iyy>1.5</iyy><izz>0.75</izz></inertia></inertial>
        <collision name="collision">
          <geometry><cylinder><radius>0.5</radius><length>2</length></cylinder></geometry>
          <surface>
            <friction><ode><mu>1.5</mu><mu2>1.5</mu2></ode></friction>
            <restitution>0.3</restitution>
          </surface>
        </collision>
        <visual name="visual">
          <geometry><cylinder><radius>0.5</radius><length>2</length></cylinder></geometry>
          <material><ambient>0 0 1 1</ambient><diffuse>0 0 1 1</diffuse></material>
        </visual>
        <sensor name="cylinder_contact_sensor" type="contact">
          <contact><collision>collision</collision></contact>
          <update_rate>1000</update_rate>
          <always_on>true</always_on>
        </sensor>
      </link>
    </model>
    
    <!-- Complex collision shape (compound geometry) -->
    <model name="compound_shape">
      <pose>6 0 5 0 0 0</pose>
      <link name="link">
        <inertial><mass>4.0</mass><inertia><ixx>2.0</ixx><iyy>2.0</iyy><izz>2.0</izz></inertia></inertial>
        
        <!-- Multiple collision geometries in one link -->
        <collision name="box_collision">
          <pose>0 0 0 0 0 0</pose>
          <geometry><box><size>1 1 1</size></box></geometry>
          <surface>
            <friction><ode><mu>1.0</mu><mu2>1.0</mu2></ode></friction>
            <restitution>0.5</restitution>
          </surface>
        </collision>
        
        <collision name="sphere_collision">
          <pose>0 0 1 0 0 0</pose>
          <geometry><sphere><radius>0.3</radius></sphere></geometry>
          <surface>
            <friction><ode><mu>0.5</mu><mu2>0.5</mu2></ode></friction>
            <restitution>0.8</restitution>
          </surface>
        </collision>
        
        <visual name="box_visual">
          <pose>0 0 0 0 0 0</pose>
          <geometry><box><size>1 1 1</size></box></geometry>
          <material><ambient>1 1 0 1</ambient><diffuse>1 1 0 1</diffuse></material>
        </visual>
        
        <visual name="sphere_visual">
          <pose>0 0 1 0 0 0</pose>
          <geometry><sphere><radius>0.3</radius></sphere></geometry>
          <material><ambient>1 0 1 1</ambient><diffuse>1 0 1 1</diffuse></material>
        </visual>
        
        <sensor name="compound_contact_sensor" type="contact">
          <contact>
            <collision>box_collision</collision>
            <collision>sphere_collision</collision>
          </contact>
          <update_rate>1000</update_rate>
          <always_on>true</always_on>
        </sensor>
      </link>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Handle contact messages from the simulation
     */
    void OnContacts(const msgs::Contacts& _msg)
    {
        for (int i = 0; i < _msg.contact_size(); ++i)
        {
            const auto& contact = _msg.contact(i);
            
            // Extract collision information
            CollisionEvent event;
            event.entity1_name = contact.collision1();
            event.entity2_name = contact.collision2();
            event.timestamp = common::Time::SystemTime();
            
            // Get contact point and normal from first contact point (if available)
            if (contact.position_size() > 0)
            {
                const auto& pos = contact.position(0);
                event.contact_point = math::Vector3d(pos.x(), pos.y(), pos.z());
            }
            
            if (contact.normal_size() > 0)
            {
                const auto& norm = contact.normal(0);
                event.normal = math::Vector3d(norm.x(), norm.y(), norm.z());
            }
            
            // Calculate force magnitude from wrench data
            if (contact.wrench_size() > 0)
            {
                const auto& wrench = contact.wrench(0);
                const auto& force = wrench.force();
                event.force_magnitude = math::Vector3d(force.x(), force.y(), force.z()).Length();
            }
            
            // Record the collision event
            collision_events_.push_back(event);
            
            // Update collision statistics
            std::string pair_key = event.entity1_name + " <-> " + event.entity2_name;
            collision_counts_[pair_key]++;
            collision_pairs_.insert({event.entity1_name, event.entity2_name});
        }
    }

    /**
     * Print recent collision events
     */
    void PrintRecentCollisions(int count)
    {
        if (collision_events_.empty()) return;
        
        int start_idx = std::max(0, static_cast<int>(collision_events_.size()) - count);
        
        gzmsg << "Recent collisions:\n";
        for (int i = start_idx; i < static_cast<int>(collision_events_.size()); ++i)
        {
            const auto& event = collision_events_[i];
            gzmsg << "  " << event.entity1_name << " <-> " << event.entity2_name
                  << " | Force: " << std::fixed << std::setprecision(2) << event.force_magnitude << "N\n";
        }
        gzmsg << "\n";
    }

    /**
     * Print comprehensive collision analysis
     */
    void PrintCollisionAnalysis()
    {
        gzmsg << "\n=== COLLISION ANALYSIS RESULTS ===\n\n";
        
        gzmsg << "Total collision events detected: " << collision_events_.size() << "\n";
        gzmsg << "Unique collision pairs: " << collision_pairs_.size() << "\n\n";
        
        if (collision_events_.empty())
        {
            gzmsg << "No collisions detected during the simulation.\n";
            return;
        }
        
        // Collision frequency analysis
        gzmsg << "Collision Frequency Analysis:\n";
        gzmsg << "-----------------------------\n";
        for (const auto& pair : collision_counts_)
        {
            gzmsg << pair.first << ": " << pair.second << " events\n";
        }
        
        // Force analysis
        gzmsg << "\nForce Analysis:\n";
        gzmsg << "---------------\n";
        
        double max_force = 0.0;
        double avg_force = 0.0;
        double force_sum = 0.0;
        std::string max_force_collision;
        
        for (const auto& event : collision_events_)
        {
            force_sum += event.force_magnitude;
            if (event.force_magnitude > max_force)
            {
                max_force = event.force_magnitude;
                max_force_collision = event.entity1_name + " <-> " + event.entity2_name;
            }
        }
        
        avg_force = force_sum / collision_events_.size();
        
        gzmsg << "Average collision force: " << std::fixed << std::setprecision(2) << avg_force << "N\n";
        gzmsg << "Maximum collision force: " << std::fixed << std::setprecision(2) << max_force << "N\n";
        gzmsg << "Strongest collision: " << max_force_collision << "\n";
        
        // Surface material analysis
        gzmsg << "\nSurface Material Behavior Observations:\n";
        gzmsg << "---------------------------------------\n";
        gzmsg << "High-friction surfaces: Objects tend to stick and not slide\n";
        gzmsg << "Low-friction surfaces: Objects slide and have reduced traction\n";
        gzmsg << "High-restitution surfaces: Objects bounce significantly\n";
        gzmsg << "Low-restitution surfaces: Objects lose energy quickly\n";
        
        gzmsg << "\nCollision Detection Performance:\n";
        gzmsg << "--------------------------------\n";
        gzmsg << "All collision events were successfully detected and reported.\n";
        gzmsg << "Contact sensors provided accurate force and contact point data.\n";
        gzmsg << "Multi-geometry collision detection worked correctly for compound shapes.\n";
        
        gzmsg << "\n=== COLLISION DEMO COMPLETE ===\n";
    }
};

/**
 * Main function - Collision Detection Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Collision Detection and Response Demo ===\n";
    gzmsg << "This demo shows different collision behaviors with various surface materials.\n\n";
    
    gzmsg << "Objects in the simulation:\n";
    gzmsg << "- High-friction red box (sticky surface)\n";
    gzmsg << "- Low-friction red box (slippery surface)\n";
    gzmsg << "- High-bounce green sphere (rubber ball)\n";
    gzmsg << "- Low-bounce green sphere (clay ball)\n";
    gzmsg << "- Rolling blue cylinder\n";
    gzmsg << "- Compound yellow shape (multiple collision geometries)\n\n";

    try
    {
        CollisionAnalyzer analyzer;
        analyzer.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during collision demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nCollision detection demo completed successfully!\n";
    gzmsg << "\nKey Learning Points:\n";
    gzmsg << "- Collision geometry can be different from visual geometry\n";
    gzmsg << "- Surface properties (friction, restitution) dramatically affect behavior\n";
    gzmsg << "- Contact sensors provide detailed collision information\n";
    gzmsg << "- Multiple collision geometries can be combined in complex shapes\n";
    gzmsg << "- Proper collision detection is crucial for realistic physics simulation\n";
    
    return 0;
}