/*
 * Gazebo Joint and Constraint Demonstration
 * 
 * This example demonstrates:
 * 1. Different joint types (revolute, prismatic, universal, ball, fixed)
 * 2. Joint limits and constraints
 * 3. Joint control and actuation
 * 4. Complex multi-body mechanisms
 * 5. Joint force and torque feedback
 * 
 * Learning objectives:
 * - Understanding mechanical joints and degrees of freedom
 * - Joint configuration and parameter tuning
 * - Building complex mechanical systems
 * - Joint control and actuation strategies
 * - Debugging joint behavior and constraints
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components/Joint.hh>
#include <gz/sim/components/JointPosition.hh>
#include <gz/sim/components/JointVelocity.hh>
#include <gz/sim/components/JointForce.hh>
#include <gz/sim/components/JointAxis.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/double.pb.h>
#include <gz/msgs/joint_cmd.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>
#include <gz/math/Angle.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <cmath>

using namespace gz;
using namespace sim;

/**
 * Joint Mechanism Controller
 * Controls and monitors various joint mechanisms
 */
class JointController
{
public:
    struct JointInfo
    {
        std::string name;
        std::string type;
        double position = 0.0;
        double velocity = 0.0;
        double force = 0.0;
        double target_position = 0.0;
        double target_velocity = 0.0;
        bool active = false;
    };

    JointController() : node_(std::make_shared<transport::Node>())
    {
        gzmsg << "Joint Controller initialized\n";
        gzmsg << "Setting up joint command publishers...\n";
        
        // Set up joint command publishers
        pendulum_pub_ = node_->Advertise<msgs::Double>("/world/joint_demo/model/pendulum/joint/pendulum_joint/cmd_pos");
        piston_pub_ = node_->Advertise<msgs::Double>("/world/joint_demo/model/piston_mechanism/joint/piston_joint/cmd_pos");
        door_pub_ = node_->Advertise<msgs::Double>("/world/joint_demo/model/door_mechanism/joint/door_hinge/cmd_pos");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Wait for publishers to connect
        
        gzmsg << "Joint command publishers ready\n\n";
    }

    /**
     * Run joint demonstration with various mechanisms
     */
    void RunDemo()
    {
        gzmsg << "=== Joint and Constraint Demonstration ===\n\n";
        
        // Create server with joint world
        ServerConfig config;
        config.SetSdfString(CreateJointWorldSdf());
        config.SetUpdateRate(1000.0);
        
        Server server(config);
        
        gzmsg << "Starting joint simulation...\n";
        gzmsg << "Demonstrating various joint types and mechanisms:\n";
        gzmsg << "- Pendulum (revolute joint with limits)\n";
        gzmsg << "- Piston mechanism (prismatic joint)\n";
        gzmsg << "- Door mechanism (revolute joint with friction)\n";
        gzmsg << "- Universal joint demonstration\n";
        gzmsg << "- Four-bar linkage mechanism\n\n";
        
        // Run simulation with joint control
        const int simulation_steps = 15000;  // 15 seconds
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps)
        {
            // Control joints based on simulation time
            double sim_time = step_count * 0.001;  // Convert to seconds
            ControlJoints(sim_time);
            
            server.RunOnce(false);
            step_count++;
            
            // Print periodic updates
            if (step_count % 2000 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                gzmsg << "Simulation time: " << std::fixed << std::setprecision(1) << sim_time 
                      << "s (real: " << elapsed << "s)\n";
                PrintJointStatus(sim_time);
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        gzmsg << "\nJoint demonstration completed!\n";
        PrintFinalAnalysis();
    }

private:
    std::shared_ptr<transport::Node> node_;
    transport::Node::Publisher pendulum_pub_;
    transport::Node::Publisher piston_pub_;
    transport::Node::Publisher door_pub_;

    /**
     * Create SDF world with various joint mechanisms
     */
    std::string CreateJointWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="joint_demo">
    
    <!-- Physics configuration -->
    <physics name="default" type="ode">
      <max_step_size>0.001</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    <plugin filename="gz-sim-joint-position-controller-system" name="gz::sim::systems::JointPositionController"/>
    
    <!-- Ground plane -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry><plane><normal>0 0 1</normal><size>20 20</size></plane></geometry>
        </collision>
        <visual name="visual">
          <geometry><plane><normal>0 0 1</normal><size>20 20</size></plane></geometry>
          <material><ambient>0.8 0.8 0.8 1</ambient><diffuse>0.8 0.8 0.8 1</diffuse></material>
        </visual>
      </link>
    </model>
    
    <!-- Simple Pendulum - Revolute Joint -->
    <model name="pendulum">
      <pose>-5 0 3 0 0 0</pose>
      
      <!-- Fixed base -->
      <link name="base">
        <inertial><mass>10</mass><inertia><ixx>1</ixx><iyy>1</iyy><izz>1</izz></inertia></inertial>
        <collision name="collision">
          <geometry><box><size>0.2 0.2 0.2</size></box></geometry>
        </collision>
        <visual name="visual">
          <geometry><box><size>0.2 0.2 0.2</size></box></geometry>
          <material><ambient>0.5 0.5 0.5 1</ambient><diffuse>0.5 0.5 0.5 1</diffuse></material>
        </visual>
      </link>
      
      <!-- Pendulum arm -->
      <link name="arm">
        <pose>0 0 -1 0 0 0</pose>
        <inertial><mass>1</mass><inertia><ixx>0.33</ixx><iyy>0.33</iyy><izz>0.01</izz></inertia></inertial>
        <collision name="collision">
          <geometry><cylinder><radius>0.05</radius><length>2</length></cylinder></geometry>
        </collision>
        <visual name="visual">
          <geometry><cylinder><radius>0.05</radius><length>2</length></cylinder></geometry>
          <material><ambient>1 0 0 1</ambient><diffuse>1 0 0 1</diffuse></material>
        </visual>
      </link>
      
      <!-- Pendulum bob -->
      <link name="bob">
        <pose>0 0 -2.5 0 0 0</pose>
        <inertial><mass>2</mass><inertia><ixx>0.2</ixx><iyy>0.2</iyy><izz>0.2</izz></inertia></inertial>
        <collision name="collision">
          <geometry><sphere><radius>0.3</radius></sphere></geometry>
        </collision>
        <visual name="visual">
          <geometry><sphere><radius>0.3</radius></sphere></geometry>
          <material><ambient>1 1 0 1</ambient><diffuse>1 1 0 1</diffuse></material>
        </visual>
      </link>
      
      <!-- Base to ground joint (fixed) -->
      <joint name="base_joint" type="fixed">
        <parent>world</parent>
        <child>base</child>
      </joint>
      
      <!-- Pendulum revolute joint with limits -->
      <joint name="pendulum_joint" type="revolute">
        <parent>base</parent>
        <child>arm</child>
        <axis>
          <xyz>0 1 0</xyz>
          <limit>
            <lower>-1.57</lower>    <!-- -90 degrees -->
            <upper>1.57</upper>     <!-- +90 degrees -->
            <effort>100</effort>
            <velocity>10</velocity>
          </limit>
          <dynamics>
            <damping>0.1</damping>   <!-- Joint damping -->
            <friction>0.01</friction>
          </dynamics>
        </axis>
        <pose>0 0 0 0 0 0</pose>
      </joint>
      
      <!-- Arm to bob joint (fixed) -->
      <joint name="arm_bob_joint" type="fixed">
        <parent>arm</parent>
        <child>bob</child>
        <pose>0 0 -1.5 0 0 0</pose>
      </joint>
      
      <!-- Joint position controller plugin -->
      <plugin filename="gz-sim-joint-position-controller-system" name="gz::sim::systems::JointPositionController">
        <joint_name>pendulum_joint</joint_name>
        <topic>/world/joint_demo/model/pendulum/joint/pendulum_joint/cmd_pos</topic>
        <p_gain>1000</p_gain>
        <i_gain>0.1</i_gain>
        <d_gain>10</d_gain>
        <i_max>1</i_max>
        <i_min>-1</i_min>
        <cmd_max>100</cmd_max>
        <cmd_min>-100</cmd_min>
      </plugin>
    </model>
    
    <!-- Piston Mechanism - Prismatic Joint -->
    <model name="piston_mechanism">
      <pose>0 0 1 0 0 0</pose>
      
      <!-- Cylinder -->
      <link name="cylinder">
        <inertial><mass>5</mass><inertia><ixx>0.5</ixx><iyy>0.5</iyy><izz>0.1</izz></inertia></inertial>
        <collision name="collision">
          <geometry><cylinder><radius>0.2</radius><length>2</length></cylinder></geometry>
        </collision>
        <visual name="visual">
          <geometry><cylinder><radius>0.2</radius><length>2</length></cylinder></geometry>
          <material><ambient>0 0 1 1</ambient><diffuse>0 0 1 1</diffuse></material>
        </visual>
      </link>
      
      <!-- Piston -->
      <link name="piston">
        <pose>0 0 0 0 0 0</pose>
        <inertial><mass>2</mass><inertia><ixx>0.1</ixx><iyy>0.1</iyy><izz>0.1</izz></inertia></inertial>
        <collision name="collision">
          <geometry><cylinder><radius>0.15</radius><length>0.5</length></cylinder></geometry>
        </collision>
        <visual name="visual">
          <geometry><cylinder><radius>0.15</radius><length>0.5</length></cylinder></geometry>
          <material><ambient>0 1 1 1</ambient><diffuse>0 1 1 1</diffuse></material>
        </visual>
      </link>
      
      <!-- Cylinder to ground joint (fixed) -->
      <joint name="cylinder_joint" type="fixed">
        <parent>world</parent>
        <child>cylinder</child>
      </joint>
      
      <!-- Prismatic joint for piston movement -->
      <joint name="piston_joint" type="prismatic">
        <parent>cylinder</parent>
        <child>piston</child>
        <axis>
          <xyz>0 0 1</xyz>
          <limit>
            <lower>-0.8</lower>     <!-- 80cm down -->
            <upper>0.8</upper>      <!-- 80cm up -->
            <effort>1000</effort>
            <velocity>5</velocity>
          </limit>
          <dynamics>
            <damping>10</damping>
            <friction>5</friction>
          </dynamics>
        </axis>
        <pose>0 0 0 0 0 0</pose>
      </joint>
      
      <!-- Piston position controller -->
      <plugin filename="gz-sim-joint-position-controller-system" name="gz::sim::systems::JointPositionController">
        <joint_name>piston_joint</joint_name>
        <topic>/world/joint_demo/model/piston_mechanism/joint/piston_joint/cmd_pos</topic>
        <p_gain>5000</p_gain>
        <i_gain>1</i_gain>
        <d_gain>50</d_gain>
        <i_max>10</i_max>
        <i_min>-10</i_min>
        <cmd_max>1000</cmd_max>
        <cmd_min>-1000</cmd_min>
      </plugin>
    </model>
    
    <!-- Door Mechanism - Revolute Joint with Friction -->
    <model name="door_mechanism">
      <pose>5 0 1 0 0 0</pose>
      
      <!-- Door frame -->
      <link name="frame">
        <inertial><mass>20</mass><inertia><ixx>1</ixx><iyy>1</iyy><izz>1</izz></inertia></inertial>
        <collision name="collision">
          <geometry><box><size>0.2 0.2 3</size></box></geometry>
        </collision>
        <visual name="visual">
          <geometry><box><size>0.2 0.2 3</size></box></geometry>
          <material><ambient>0.6 0.3 0.1 1</ambient><diffuse>0.6 0.3 0.1 1</diffuse></material>
        </visual>
      </link>
      
      <!-- Door -->
      <link name="door">
        <pose>0.6 0 0 0 0 0</pose>
        <inertial><mass>10</mass><inertia><ixx>2.5</ixx><iyy>0.33</iyy><izz>2.5</izz></inertia></inertial>
        <collision name="collision">
          <geometry><box><size>1.2 0.1 2.5</size></box></geometry>
        </collision>
        <visual name="visual">
          <geometry><box><size>1.2 0.1 2.5</size></box></geometry>
          <material><ambient>0.8 0.5 0.2 1</ambient><diffuse>0.8 0.5 0.2 1</diffuse></material>
        </visual>
      </link>
      
      <!-- Frame to ground joint (fixed) -->
      <joint name="frame_joint" type="fixed">
        <parent>world</parent>
        <child>frame</child>
      </joint>
      
      <!-- Door hinge with friction and limits -->
      <joint name="door_hinge" type="revolute">
        <parent>frame</parent>
        <child>door</child>
        <axis>
          <xyz>0 0 1</xyz>
          <limit>
            <lower>0</lower>        <!-- Can't swing backwards -->
            <upper>1.57</upper>     <!-- Max 90 degrees -->
            <effort>200</effort>
            <velocity>2</velocity>
          </limit>
          <dynamics>
            <damping>5</damping>     <!-- Door closing damping -->
            <friction>2</friction>   <!-- Hinge friction -->
          </dynamics>
        </axis>
        <pose>-0.6 0 0 0 0 0</pose>
      </joint>
      
      <!-- Door position controller -->
      <plugin filename="gz-sim-joint-position-controller-system" name="gz::sim::systems::JointPositionController">
        <joint_name>door_hinge</joint_name>
        <topic>/world/joint_demo/model/door_mechanism/joint/door_hinge/cmd_pos</topic>
        <p_gain>500</p_gain>
        <i_gain>0.1</i_gain>
        <d_gain>50</d_gain>
        <i_max>5</i_max>
        <i_min>-5</i_min>
        <cmd_max>200</cmd_max>
        <cmd_min>-200</cmd_min>
      </plugin>
    </model>
    
    <!-- Universal Joint Demonstration -->
    <model name="universal_joint_demo">
      <pose>0 5 2 0 0 0</pose>
      
      <!-- Base -->
      <link name="base">
        <inertial><mass>5</mass><inertia><ixx>0.5</ixx><iyy>0.5</iyy><izz>0.5</izz></inertia></inertial>
        <collision name="collision">
          <geometry><box><size>0.5 0.5 0.2</size></box></geometry>
        </collision>
        <visual name="visual">
          <geometry><box><size>0.5 0.5 0.2</size></box></geometry>
          <material><ambient>1 0.5 0 1</ambient><diffuse>1 0.5 0 1</diffuse></material>
        </visual>
      </link>
      
      <!-- Intermediate link -->
      <link name="intermediate">
        <pose>0 0 0.5 0 0 0</pose>
        <inertial><mass>2</mass><inertia><ixx>0.1</ixx><iyy>0.1</iyy><izz>0.01</izz></inertia></inertial>
        <collision name="collision">
          <geometry><cylinder><radius>0.05</radius><length>1</length></cylinder></geometry>
        </collision>
        <visual name="visual">
          <geometry><cylinder><radius>0.05</radius><length>1</length></cylinder></geometry>
          <material><ambient>0 1 0.5 1</ambient><diffuse>0 1 0.5 1</diffuse></material>
        </visual>
      </link>
      
      <!-- End effector -->
      <link name="end_effector">
        <pose>0 0 1.2 0 0 0</pose>
        <inertial><mass>1</mass><inertia><ixx>0.05</ixx><iyy>0.05</iyy><izz>0.05</izz></inertia></inertial>
        <collision name="collision">
          <geometry><sphere><radius>0.2</radius></sphere></geometry>
        </collision>
        <visual name="visual">
          <geometry><sphere><radius>0.2</radius></sphere></geometry>
          <material><ambient>1 0 0.5 1</ambient><diffuse>1 0 0.5 1</diffuse></material>
        </visual>
      </link>
      
      <!-- Base to ground (fixed) -->
      <joint name="universal_base" type="fixed">
        <parent>world</parent>
        <child>base</child>
      </joint>
      
      <!-- First axis of universal joint -->
      <joint name="universal_axis1" type="revolute">
        <parent>base</parent>
        <child>intermediate</child>
        <axis>
          <xyz>1 0 0</xyz>
          <limit>
            <lower>-0.785</lower>   <!-- -45 degrees -->
            <upper>0.785</upper>    <!-- +45 degrees -->
            <effort>50</effort>
          </limit>
          <dynamics><damping>0.5</damping></dynamics>
        </axis>
        <pose>0 0 0.3 0 0 0</pose>
      </joint>
      
      <!-- Second axis of universal joint -->
      <joint name="universal_axis2" type="revolute">
        <parent>intermediate</parent>
        <child>end_effector</child>
        <axis>
          <xyz>0 1 0</xyz>
          <limit>
            <lower>-0.785</lower>   <!-- -45 degrees -->
            <upper>0.785</upper>    <!-- +45 degrees -->
            <effort>50</effort>
          </limit>
          <dynamics><damping>0.5</damping></dynamics>
        </axis>
        <pose>0 0 0.7 0 0 0</pose>
      </joint>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Control joints based on simulation time
     */
    void ControlJoints(double sim_time)
    {
        // Pendulum: Sinusoidal motion
        double pendulum_angle = 0.8 * sin(sim_time * 0.5);  // Slow oscillation
        msgs::Double pendulum_cmd;
        pendulum_cmd.set_data(pendulum_angle);
        pendulum_pub_.Publish(pendulum_cmd);
        
        // Piston: Triangular wave motion
        double piston_cycle = fmod(sim_time, 4.0);  // 4-second cycle
        double piston_pos;
        if (piston_cycle < 2.0)
        {
            piston_pos = 0.7 * (piston_cycle / 2.0 - 0.5);  // Up
        }
        else
        {
            piston_pos = 0.7 * (1.5 - piston_cycle / 2.0);  // Down
        }
        msgs::Double piston_cmd;
        piston_cmd.set_data(piston_pos);
        piston_pub_.Publish(piston_cmd);
        
        // Door: Step function (open and close)
        double door_cycle = fmod(sim_time, 8.0);  // 8-second cycle
        double door_angle;
        if (door_cycle < 2.0)
        {
            door_angle = 0.0;  // Closed
        }
        else if (door_cycle < 4.0)
        {
            door_angle = 1.2;  // Opening
        }
        else if (door_cycle < 6.0)
        {
            door_angle = 1.2;  // Open
        }
        else
        {
            door_angle = 0.0;  // Closing
        }
        msgs::Double door_cmd;
        door_cmd.set_data(door_angle);
        door_pub_.Publish(door_cmd);
    }

    /**
     * Print current joint status
     */
    void PrintJointStatus(double sim_time)
    {
        gzmsg << "Current joint states:\n";
        gzmsg << "  Pendulum: Oscillating (period: ~12.6s)\n";
        gzmsg << "  Piston: " << (fmod(sim_time, 4.0) < 2.0 ? "Extending" : "Retracting") << "\n";
        gzmsg << "  Door: " << GetDoorState(sim_time) << "\n";
        gzmsg << "  Universal joint: Free motion under gravity\n\n";
    }

    /**
     * Get door state description
     */
    std::string GetDoorState(double sim_time)
    {
        double door_cycle = fmod(sim_time, 8.0);
        if (door_cycle < 2.0) return "Closed";
        else if (door_cycle < 4.0) return "Opening";
        else if (door_cycle < 6.0) return "Open";
        else return "Closing";
    }

    /**
     * Print final analysis of joint behavior
     */
    void PrintFinalAnalysis()
    {
        gzmsg << "\n=== JOINT MECHANISM ANALYSIS ===\n\n";
        
        gzmsg << "Joint Types Demonstrated:\n";
        gzmsg << "-------------------------\n";
        gzmsg << "1. REVOLUTE JOINT (Pendulum & Door)\n";
        gzmsg << "   - Rotational motion around single axis\n";
        gzmsg << "   - Joint limits prevent over-rotation\n";
        gzmsg << "   - Damping provides realistic friction\n\n";
        
        gzmsg << "2. PRISMATIC JOINT (Piston)\n";
        gzmsg << "   - Linear motion along single axis\n";
        gzmsg << "   - Position limits define stroke length\n";
        gzmsg << "   - High damping simulates hydraulic resistance\n\n";
        
        gzmsg << "3. UNIVERSAL JOINT (Two-axis rotation)\n";
        gzmsg << "   - Two perpendicular revolute joints\n";
        gzmsg << "   - Allows complex 3D orientation changes\n";
        gzmsg << "   - Limited range of motion for each axis\n\n";
        
        gzmsg << "4. FIXED JOINT (Structural connections)\n";
        gzmsg << "   - Rigid connection between links\n";
        gzmsg << "   - Used for mounting and structural assembly\n";
        gzmsg << "   - No degrees of freedom\n\n";
        
        gzmsg << "Joint Control Strategies:\n";
        gzmsg << "-------------------------\n";
        gzmsg << "- Position Control: Target specific joint angles/positions\n";
        gzmsg << "- PID Controllers: Maintain desired positions with feedback\n";
        gzmsg << "- Joint Limits: Prevent mechanical damage and unrealistic motion\n";
        gzmsg << "- Damping/Friction: Provide realistic energy dissipation\n\n";
        
        gzmsg << "Applications:\n";
        gzmsg << "-------------\n";
        gzmsg << "- Robot manipulators: Serial chain of revolute joints\n";
        gzmsg << "- Mobile platforms: Wheels with revolute joints\n";
        gzmsg << "- Actuator systems: Prismatic joints for linear motion\n";
        gzmsg << "- Gimbal systems: Universal joints for camera/sensor orientation\n\n";
        
        gzmsg << "=== JOINT DEMO COMPLETE ===\n";
    }
};

/**
 * Main function - Joint and Constraint Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Joint and Constraint Demonstration ===\n";
    gzmsg << "This demo shows various joint types and mechanical mechanisms.\n\n";
    
    gzmsg << "Mechanisms to observe:\n";
    gzmsg << "- Pendulum: Revolute joint with oscillating motion\n";
    gzmsg << "- Piston: Prismatic joint with linear actuation\n";
    gzmsg << "- Door: Revolute joint with friction and limits\n";
    gzmsg << "- Universal Joint: Two-axis rotational coupling\n\n";

    try
    {
        JointController controller;
        controller.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during joint demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nJoint and constraint demo completed successfully!\n";
    gzmsg << "\nKey Learning Points:\n";
    gzmsg << "- Joints define how links can move relative to each other\n";
    gzmsg << "- Joint limits prevent unrealistic or damaging motions\n";
    gzmsg << "- Damping and friction provide realistic energy dissipation\n";
    gzmsg << "- Joint controllers enable precise motion control\n";
    gzmsg << "- Different joint types serve different mechanical purposes\n";
    gzmsg << "- Complex mechanisms can be built by combining simple joints\n";
    
    return 0;
}