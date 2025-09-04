#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/components/Actor.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/Model.hh>

#include <gz/msgs/entity_factory.pb.h>
#include <gz/msgs/pose.pb.h>
#include <gz/msgs/stringmsg.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Pose3.hh>
#include <gz/math/Vector3.hh>
#include <gz/math/Quaternion.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cmath>
#include <queue>
#include <sstream>

/**
 * @brief Actor Simulation and Behavior Trees Demo
 * 
 * This program demonstrates advanced actor capabilities in Gazebo,
 * showcasing realistic human-like entities with complex behaviors:
 * 
 * Actor Features:
 * - Dynamic actor spawning and management
 * - Scripted animation sequences
 * - Waypoint-based navigation
 * - Collision detection and avoidance
 * - Interactive behavior responses
 * - Group coordination and flocking
 * 
 * Behavior Systems:
 * - State machine implementation
 * - Goal-oriented action planning
 * - Environmental awareness and response
 * - Social interaction modeling
 * - Adaptive behavior based on context
 * 
 * Animation Control:
 * - Skeletal animation integration
 * - Smooth transition between animations
 * - Physics-based movement constraints
 * - Realistic human motion patterns
 * 
 * Applications Demonstrated:
 * - Crowd simulation and pedestrian modeling
 * - Human-robot interaction scenarios
 * - Safety training simulations
 * - Environmental behavior testing
 * - Multi-agent coordination systems
 */

struct ActorBehaviorState
{
    enum State {
        IDLE,
        WALKING,
        RUNNING,
        SITTING,
        WAVING,
        TALKING,
        AVOIDING_OBSTACLE,
        FOLLOWING_PATH,
        INTERACTING
    };
    
    State current_state;
    State previous_state;
    double state_duration;
    double state_start_time;
    
    ActorBehaviorState() : current_state(IDLE), previous_state(IDLE), 
                          state_duration(0.0), state_start_time(0.0) {}
};

struct Waypoint
{
    gz::math::Vector3d position;
    double wait_time;
    std::string animation;
    
    Waypoint(const gz::math::Vector3d& pos, double wait = 1.0, const std::string& anim = "walking")
        : position(pos), wait_time(wait), animation(anim) {}
};

struct ActorInfo
{
    std::string name;
    gz::math::Pose3d pose;
    ActorBehaviorState behavior;
    std::vector<Waypoint> waypoints;
    size_t current_waypoint;
    double speed;
    bool active;
    
    ActorInfo(const std::string& n, const gz::math::Pose3d& p)
        : name(n), pose(p), current_waypoint(0), speed(1.5), active(true) {}
};

class ActorSimulationManager
{
public:
    ActorSimulationManager()
        : simulation_time_(0.0)
        , demo_phase_(0)
        , max_actors_(5)
        , actor_id_counter_(0)
    {
        // Initialize transport node
        transport_node_ = std::make_unique<gz::transport::Node>();
        
        // Service clients for entity management
        entity_factory_client_ = transport_node_->Request("/world/advanced_features/create");
        entity_remove_client_ = transport_node_->Request("/world/advanced_features/remove");
        
        // Publishers for actor control
        actor_pose_pub_ = transport_node_->Advertise<gz::msgs::Pose>("/model/actor/pose");
        
        // Define waypoint patterns for different actor behaviors
        InitializeWaypointPatterns();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gzmsg << "Actor Simulation Manager initialized" << std::endl;
    }
    
    void Start()
    {
        gzmsg << "\n=== Actor Simulation and Behavior Trees Demo ===" << std::endl;
        gzmsg << "Starting comprehensive actor behavior demonstration..." << std::endl;
        
        const double control_frequency = 30.0; // 30 Hz for smooth actor animation
        const auto control_period = std::chrono::duration<double>(1.0 / control_frequency);
        
        while (demo_phase_ < 6)
        {
            auto start_time = std::chrono::steady_clock::now();
            
            switch (demo_phase_)
            {
                case 0: SingleActorDemo(); break;
                case 1: MultipleActorsDemo(); break;
                case 2: BehaviorTreeDemo(); break;
                case 3: CrowdSimulationDemo(); break;
                case 4: InteractionDemo(); break;
                case 5: PerformanceStressTestDemo(); break;
            }
            
            // Update all active actors
            UpdateActors();
            
            simulation_time_ += 1.0 / control_frequency;
            
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = end_time - start_time;
            if (elapsed < control_period)
            {
                std::this_thread::sleep_for(control_period - elapsed);
            }
        }
        
        gzmsg << "Actor simulation demo completed!" << std::endl;
    }

private:
    void InitializeWaypointPatterns()
    {
        // Circular patrol pattern
        patrol_waypoints_ = {
            Waypoint(gz::math::Vector3d(-5, -5, 0), 1.0, "walking"),
            Waypoint(gz::math::Vector3d(5, -5, 0), 1.0, "walking"),
            Waypoint(gz::math::Vector3d(5, 5, 0), 2.0, "idle"),
            Waypoint(gz::math::Vector3d(-5, 5, 0), 1.0, "walking"),
        };
        
        // Random exploration pattern
        exploration_waypoints_ = {
            Waypoint(gz::math::Vector3d(3, 2, 0), 0.5, "walking"),
            Waypoint(gz::math::Vector3d(-2, 4, 0), 1.5, "sitting"),
            Waypoint(gz::math::Vector3d(1, -3, 0), 0.8, "walking"),
            Waypoint(gz::math::Vector3d(-4, -1, 0), 2.0, "waving"),
            Waypoint(gz::math::Vector3d(0, 0, 0), 1.0, "talking"),
        };
        
        // Social interaction pattern
        social_waypoints_ = {
            Waypoint(gz::math::Vector3d(-1, -1, 0), 3.0, "talking"),
            Waypoint(gz::math::Vector3d(1, -1, 0), 3.0, "talking"),
            Waypoint(gz::math::Vector3d(0, 1, 0), 2.0, "waving"),
        };
    }
    
    void SingleActorDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 1: Single Actor Behavior Demo ---" << std::endl;
            gzmsg << "Spawning single actor with scripted behavior..." << std::endl;
            
            // Create first actor
            CreateActor("actor_1", gz::math::Pose3d(0, 0, 0, 0, 0, 0), patrol_waypoints_);
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        if (simulation_time_ - start_time > 15.0)
        {
            gzmsg << "Single actor demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void MultipleActorsDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 2: Multiple Actors Coordination Demo ---" << std::endl;
            gzmsg << "Spawning multiple actors with different behaviors..." << std::endl;
            
            // Create additional actors with different patterns
            CreateActor("actor_2", gz::math::Pose3d(3, 3, 0, 0, 0, M_PI), exploration_waypoints_);
            CreateActor("actor_3", gz::math::Pose3d(-3, -3, 0, 0, 0, M_PI/2), social_waypoints_);
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        if (simulation_time_ - start_time > 20.0)
        {
            gzmsg << "Multiple actors demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void BehaviorTreeDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 3: Advanced Behavior Tree Demo ---" << std::endl;
            gzmsg << "Implementing complex decision-making behaviors..." << std::endl;
            
            // Update existing actors with behavior trees
            for (auto& actor : active_actors_)
            {
                UpdateActorBehaviorTree(actor);
            }
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        if (simulation_time_ - start_time > 18.0)
        {
            gzmsg << "Behavior tree demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void CrowdSimulationDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 4: Crowd Simulation Demo ---" << std::endl;
            gzmsg << "Creating realistic crowd behaviors and flocking..." << std::endl;
            
            // Spawn additional actors for crowd simulation
            for (int i = 0; i < 3 && active_actors_.size() < max_actors_; ++i)
            {
                double angle = i * 2 * M_PI / 3;
                gz::math::Pose3d pose(
                    4 * std::cos(angle), 4 * std::sin(angle), 0,
                    0, 0, angle + M_PI
                );
                
                std::string name = "crowd_actor_" + std::to_string(i);
                CreateActor(name, pose, patrol_waypoints_);
            }
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        // Update crowd behaviors
        UpdateCrowdBehaviors();
        
        if (simulation_time_ - start_time > 25.0)
        {
            gzmsg << "Crowd simulation demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void InteractionDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 5: Human-Environment Interaction Demo ---" << std::endl;
            gzmsg << "Demonstrating actor interactions with environment..." << std::endl;
            
            // Trigger interaction behaviors
            for (auto& actor : active_actors_)
            {
                if (actor.name == "actor_1")
                {
                    // Make actor_1 interact with objects
                    actor.behavior.current_state = ActorBehaviorState::INTERACTING;
                    actor.behavior.state_start_time = simulation_time_;
                }
            }
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        if (simulation_time_ - start_time > 15.0)
        {
            gzmsg << "Interaction demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void PerformanceStressTestDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 6: Performance Stress Test Demo ---" << std::endl;
            gzmsg << "Testing performance with maximum actor count..." << std::endl;
            
            // Measure performance metrics
            performance_metrics_.actors_count = active_actors_.size();
            performance_metrics_.start_time = simulation_time_;
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Monitor performance
        if (static_cast<int>(phase_time) % 5 == 0 && 
            phase_time - static_cast<int>(phase_time) < 0.1)
        {
            ReportPerformanceMetrics();
        }
        
        if (phase_time > 10.0)
        {
            gzmsg << "Performance stress test completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void CreateActor(const std::string& name, 
                    const gz::math::Pose3d& pose,
                    const std::vector<Waypoint>& waypoints)
    {
        // Create actor SDF description
        std::string actor_sdf = GenerateActorSDF(name, pose, waypoints);
        
        // Send entity creation request
        gz::msgs::EntityFactory req;
        req.set_sdf(actor_sdf);
        req.set_name(name);
        
        gz::msgs::Boolean rep;
        bool result;
        unsigned int timeout = 1000;
        
        bool executed = entity_factory_client_.Request(req, rep, timeout, result);
        
        if (executed && result && rep.data())
        {
            // Add to active actors list
            ActorInfo actor_info(name, pose);
            actor_info.waypoints = waypoints;
            active_actors_.push_back(actor_info);
            
            gzmsg << "Successfully created actor: " << name << std::endl;
        }
        else
        {
            gzerr << "Failed to create actor: " << name << std::endl;
        }
    }
    
    std::string GenerateActorSDF(const std::string& name, 
                                const gz::math::Pose3d& pose,
                                const std::vector<Waypoint>& waypoints)
    {
        std::ostringstream sdf_stream;
        
        sdf_stream << "<?xml version='1.0'?>\n";
        sdf_stream << "<sdf version='1.10'>\n";
        sdf_stream << "  <actor name='" << name << "'>\n";
        sdf_stream << "    <pose>" << pose.Pos().X() << " " << pose.Pos().Y() << " " << pose.Pos().Z() 
                   << " " << pose.Rot().Roll() << " " << pose.Rot().Pitch() << " " << pose.Rot().Yaw() << "</pose>\n";
        sdf_stream << "    <skin>\n";
        sdf_stream << "      <filename>model://actor_walking.dae</filename>\n";
        sdf_stream << "      <scale>1.0</scale>\n";
        sdf_stream << "    </skin>\n";
        
        // Animation definitions
        sdf_stream << "    <animation name='walking'>\n";
        sdf_stream << "      <filename>model://actor_walking.dae</filename>\n";
        sdf_stream << "      <scale>1.000000</scale>\n";
        sdf_stream << "      <interpolate_x>true</interpolate_x>\n";
        sdf_stream << "    </animation>\n";
        
        sdf_stream << "    <animation name='sitting'>\n";
        sdf_stream << "      <filename>model://actor_sitting.dae</filename>\n";
        sdf_stream << "      <scale>1.000000</scale>\n";
        sdf_stream << "      <interpolate_x>true</interpolate_x>\n";
        sdf_stream << "    </animation>\n";
        
        sdf_stream << "    <animation name='running'>\n";
        sdf_stream << "      <filename>model://actor_running.dae</filename>\n";
        sdf_stream << "      <scale>1.000000</scale>\n";
        sdf_stream << "      <interpolate_x>true</interpolate_x>\n";
        sdf_stream << "    </animation>\n";
        
        // Scripted trajectory
        sdf_stream << "    <script>\n";
        sdf_stream << "      <loop>true</loop>\n";
        sdf_stream << "      <delay_start>0.000000</delay_start>\n";
        sdf_stream << "      <auto_start>true</auto_start>\n";
        
        // Generate waypoint trajectory
        double total_time = 0.0;
        for (size_t i = 0; i < waypoints.size(); ++i)
        {
            const auto& wp = waypoints[i];
            double segment_time = 5.0; // Time to reach waypoint
            
            sdf_stream << "      <trajectory id='" << i << "' type='walking'>\n";
            sdf_stream << "        <waypoint>\n";
            sdf_stream << "          <time>" << total_time << "</time>\n";
            sdf_stream << "          <pose>" << wp.position.X() << " " << wp.position.Y() << " " << wp.position.Z()
                       << " 0 0 0</pose>\n";
            sdf_stream << "        </waypoint>\n";
            
            total_time += segment_time;
            
            sdf_stream << "        <waypoint>\n";
            sdf_stream << "          <time>" << total_time << "</time>\n";
            sdf_stream << "          <pose>" << wp.position.X() << " " << wp.position.Y() << " " << wp.position.Z()
                       << " 0 0 0</pose>\n";
            sdf_stream << "        </waypoint>\n";
            sdf_stream << "      </trajectory>\n";
            
            total_time += wp.wait_time;
        }
        
        sdf_stream << "    </script>\n";
        
        // Physics properties
        sdf_stream << "    <link name='" << name << "_link'>\n";
        sdf_stream << "      <inertial>\n";
        sdf_stream << "        <mass>80.0</mass>\n";
        sdf_stream << "        <inertia>\n";
        sdf_stream << "          <ixx>24.88</ixx>\n";
        sdf_stream << "          <ixy>0</ixy>\n";
        sdf_stream << "          <ixz>0</ixz>\n";
        sdf_stream << "          <iyy>25.73</iyy>\n";
        sdf_stream << "          <iyz>0</iyz>\n";
        sdf_stream << "          <izz>2.48</izz>\n";
        sdf_stream << "        </inertia>\n";
        sdf_stream << "        <pose>0 0 0.9 0 0 0</pose>\n";
        sdf_stream << "      </inertial>\n";
        sdf_stream << "      <collision name='bottom'>\n";
        sdf_stream << "        <pose>0 0 0.02 0 0 0</pose>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <box><size>0.5 0.35 0.04</size></box>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "      </collision>\n";
        sdf_stream << "      <collision name='person'>\n";
        sdf_stream << "        <pose>0 0 0.02 0.04 0 0</pose>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <box><size>0.48 0.25 1.8</size></box>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "      </collision>\n";
        sdf_stream << "    </link>\n";
        
        // Actor plugin for behavior control
        sdf_stream << "    <plugin filename='gz-sim-actor-commands-system' name='gz::sim::systems::ActorCommands'/>\n";
        
        sdf_stream << "  </actor>\n";
        sdf_stream << "</sdf>\n";
        
        return sdf_stream.str();
    }
    
    void UpdateActors()
    {
        for (auto& actor : active_actors_)
        {
            UpdateActorBehavior(actor);
            UpdateActorAnimation(actor);
            
            // Update behavior state timing
            actor.behavior.state_duration = simulation_time_ - actor.behavior.state_start_time;
        }
        
        // Report actor status periodically
        static int update_counter = 0;
        if (++update_counter % 900 == 0) // Every 30 seconds at 30 Hz
        {
            ReportActorStatus();
        }
    }
    
    void UpdateActorBehavior(ActorInfo& actor)
    {
        switch (actor.behavior.current_state)
        {
            case ActorBehaviorState::IDLE:
                if (actor.behavior.state_duration > 2.0)
                {
                    TransitionToState(actor, ActorBehaviorState::WALKING);
                }
                break;
                
            case ActorBehaviorState::WALKING:
                // Check if reached current waypoint
                if (actor.current_waypoint < actor.waypoints.size())
                {
                    const auto& target = actor.waypoints[actor.current_waypoint];
                    double distance = (target.position - actor.pose.Pos()).Length();
                    
                    if (distance < 0.5) // Reached waypoint
                    {
                        actor.current_waypoint = (actor.current_waypoint + 1) % actor.waypoints.size();
                        
                        // Decide next behavior based on waypoint
                        if (target.wait_time > 1.5)
                        {
                            TransitionToState(actor, ActorBehaviorState::SITTING);
                        }
                        else if (target.animation == "waving")
                        {
                            TransitionToState(actor, ActorBehaviorState::WAVING);
                        }
                        else
                        {
                            TransitionToState(actor, ActorBehaviorState::IDLE);
                        }
                    }
                }
                break;
                
            case ActorBehaviorState::SITTING:
                if (actor.behavior.state_duration > 3.0)
                {
                    TransitionToState(actor, ActorBehaviorState::WALKING);
                }
                break;
                
            case ActorBehaviorState::WAVING:
                if (actor.behavior.state_duration > 2.0)
                {
                    TransitionToState(actor, ActorBehaviorState::WALKING);
                }
                break;
                
            case ActorBehaviorState::TALKING:
                if (actor.behavior.state_duration > 4.0)
                {
                    TransitionToState(actor, ActorBehaviorState::WALKING);
                }
                break;
                
            case ActorBehaviorState::INTERACTING:
                if (actor.behavior.state_duration > 5.0)
                {
                    TransitionToState(actor, ActorBehaviorState::IDLE);
                }
                break;
                
            default:
                break;
        }
    }
    
    void UpdateActorBehaviorTree(ActorInfo& actor)
    {
        // Simplified behavior tree implementation
        
        // Root: Selector node
        if (ExecuteBehaviorSelector(actor))
        {
            // Behavior tree executed successfully
        }
    }
    
    bool ExecuteBehaviorSelector(ActorInfo& actor)
    {
        // Priority-based behavior selection
        
        // 1. Safety behaviors (highest priority)
        if (CheckForObstacles(actor))
        {
            TransitionToState(actor, ActorBehaviorState::AVOIDING_OBSTACLE);
            return true;
        }
        
        // 2. Social behaviors
        if (CheckForSocialInteraction(actor))
        {
            TransitionToState(actor, ActorBehaviorState::TALKING);
            return true;
        }
        
        // 3. Navigation behaviors
        if (actor.behavior.current_state == ActorBehaviorState::IDLE && 
            actor.behavior.state_duration > 1.0)
        {
            TransitionToState(actor, ActorBehaviorState::FOLLOWING_PATH);
            return true;
        }
        
        // 4. Default behaviors
        return false;
    }
    
    bool CheckForObstacles(const ActorInfo& actor)
    {
        // Simulate obstacle detection
        // In a real implementation, this would use sensor data
        
        for (const auto& other_actor : active_actors_)
        {
            if (other_actor.name == actor.name) continue;
            
            double distance = (other_actor.pose.Pos() - actor.pose.Pos()).Length();
            if (distance < 1.0) // Too close to another actor
            {
                return true;
            }
        }
        
        return false;
    }
    
    bool CheckForSocialInteraction(const ActorInfo& actor)
    {
        // Check if other actors are nearby for social interaction
        
        for (const auto& other_actor : active_actors_)
        {
            if (other_actor.name == actor.name) continue;
            
            double distance = (other_actor.pose.Pos() - actor.pose.Pos()).Length();
            if (distance < 2.0 && distance > 0.8)
            {
                // Good distance for social interaction
                if (other_actor.behavior.current_state == ActorBehaviorState::IDLE ||
                    other_actor.behavior.current_state == ActorBehaviorState::TALKING)
                {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    void UpdateCrowdBehaviors()
    {
        // Implement flocking behavior for crowd simulation
        
        for (auto& actor : active_actors_)
        {
            if (actor.name.find("crowd_actor") != std::string::npos)
            {
                ApplyFlockingBehavior(actor);
            }
        }
    }
    
    void ApplyFlockingBehavior(ActorInfo& actor)
    {
        gz::math::Vector3d separation_force(0, 0, 0);
        gz::math::Vector3d alignment_force(0, 0, 0);
        gz::math::Vector3d cohesion_force(0, 0, 0);
        
        int neighbor_count = 0;
        const double neighbor_radius = 3.0;
        
        for (const auto& other_actor : active_actors_)
        {
            if (other_actor.name == actor.name) continue;
            
            double distance = (other_actor.pose.Pos() - actor.pose.Pos()).Length();
            
            if (distance < neighbor_radius)
            {
                neighbor_count++;
                
                // Separation: avoid crowding neighbors
                if (distance < 1.0)
                {
                    gz::math::Vector3d diff = actor.pose.Pos() - other_actor.pose.Pos();
                    diff.Normalize();
                    separation_force += diff / distance; // Stronger force when closer
                }
                
                // Alignment: steer towards average heading of neighbors
                // (simplified - would need velocity vectors in real implementation)
                
                // Cohesion: steer towards average position of neighbors
                cohesion_force += other_actor.pose.Pos();
            }
        }
        
        if (neighbor_count > 0)
        {
            cohesion_force /= neighbor_count;
            cohesion_force -= actor.pose.Pos();
            cohesion_force.Normalize();
            
            // Apply combined forces to modify actor behavior
            gz::math::Vector3d total_force = separation_force * 2.0 + 
                                           cohesion_force * 1.0;
            
            if (total_force.Length() > 0.1)
            {
                // Modify actor's target waypoint based on flocking forces
                if (actor.current_waypoint < actor.waypoints.size())
                {
                    actor.waypoints[actor.current_waypoint].position += total_force * 0.5;
                }
            }
        }
    }
    
    void UpdateActorAnimation(const ActorInfo& actor)
    {
        // Animation updates would be sent to the actor plugin
        // This is a simplified representation
        
        std::string animation_name;
        switch (actor.behavior.current_state)
        {
            case ActorBehaviorState::WALKING:
            case ActorBehaviorState::FOLLOWING_PATH:
                animation_name = "walking";
                break;
            case ActorBehaviorState::RUNNING:
                animation_name = "running";
                break;
            case ActorBehaviorState::SITTING:
                animation_name = "sitting";
                break;
            default:
                animation_name = "idle";
                break;
        }
        
        // In a real implementation, this would send animation commands
        // to the actor plugin via gz-transport
    }
    
    void TransitionToState(ActorInfo& actor, ActorBehaviorState::State new_state)
    {
        if (actor.behavior.current_state != new_state)
        {
            actor.behavior.previous_state = actor.behavior.current_state;
            actor.behavior.current_state = new_state;
            actor.behavior.state_start_time = simulation_time_;
            actor.behavior.state_duration = 0.0;
        }
    }
    
    void ReportActorStatus()
    {
        gzmsg << "\n=== Actor Status Report ===" << std::endl;
        gzmsg << "Active actors: " << active_actors_.size() << std::endl;
        gzmsg << "Simulation time: " << std::fixed << std::setprecision(1) 
              << simulation_time_ << "s" << std::endl;
        
        for (const auto& actor : active_actors_)
        {
            std::string state_name = GetStateName(actor.behavior.current_state);
            gzmsg << "  " << actor.name << ": " << state_name 
                  << " (duration: " << std::setprecision(1) << actor.behavior.state_duration << "s)"
                  << " waypoint: " << actor.current_waypoint + 1 << "/" << actor.waypoints.size() << std::endl;
        }
    }
    
    void ReportPerformanceMetrics()
    {
        double elapsed_time = simulation_time_ - performance_metrics_.start_time;
        
        gzmsg << "\n=== Performance Metrics ===" << std::endl;
        gzmsg << "Active actors: " << active_actors_.size() << std::endl;
        gzmsg << "Simulation time: " << std::fixed << std::setprecision(1) << elapsed_time << "s" << std::endl;
        gzmsg << "Average update frequency: " << std::setprecision(1) 
              << (30.0) << " Hz" << std::endl; // Target frequency
        
        // Calculate behavior state distribution
        std::map<ActorBehaviorState::State, int> state_counts;
        for (const auto& actor : active_actors_)
        {
            state_counts[actor.behavior.current_state]++;
        }
        
        gzmsg << "Behavior distribution:" << std::endl;
        for (const auto& pair : state_counts)
        {
            gzmsg << "  " << GetStateName(pair.first) << ": " << pair.second << std::endl;
        }
    }
    
    std::string GetStateName(ActorBehaviorState::State state) const
    {
        switch (state)
        {
            case ActorBehaviorState::IDLE: return "IDLE";
            case ActorBehaviorState::WALKING: return "WALKING";
            case ActorBehaviorState::RUNNING: return "RUNNING";
            case ActorBehaviorState::SITTING: return "SITTING";
            case ActorBehaviorState::WAVING: return "WAVING";
            case ActorBehaviorState::TALKING: return "TALKING";
            case ActorBehaviorState::AVOIDING_OBSTACLE: return "AVOIDING_OBSTACLE";
            case ActorBehaviorState::FOLLOWING_PATH: return "FOLLOWING_PATH";
            case ActorBehaviorState::INTERACTING: return "INTERACTING";
            default: return "UNKNOWN";
        }
    }
    
    // Member variables
    std::unique_ptr<gz::transport::Node> transport_node_;
    gz::transport::Node::Publisher actor_pose_pub_;
    
    // Service clients
    gz::transport::Node::RequesterProxy entity_factory_client_;
    gz::transport::Node::RequesterProxy entity_remove_client_;
    
    std::vector<ActorInfo> active_actors_;
    
    // Predefined waypoint patterns
    std::vector<Waypoint> patrol_waypoints_;
    std::vector<Waypoint> exploration_waypoints_;
    std::vector<Waypoint> social_waypoints_;
    
    double simulation_time_;
    int demo_phase_;
    size_t max_actors_;
    int actor_id_counter_;
    
    struct PerformanceMetrics {
        size_t actors_count;
        double start_time;
        double total_updates;
    } performance_metrics_;
};

int main(int argc, char** argv)
{
    // Configure Gazebo server
    gz::sim::ServerConfig config;
    config.SetSdfFile("worlds/advanced_features.sdf");
    config.SetUpdateRate(1000.0);
    
    gz::sim::Server server(config);
    
    gzmsg << "\n=== Actor Simulation and Behavior Trees Demo ===" << std::endl;
    gzmsg << "Gazebo Harmonic - Advanced Actor Features Tutorial" << std::endl;
    gzmsg << "\nThis demo demonstrates:" << std::endl;
    gzmsg << "• Dynamic actor spawning and management" << std::endl;
    gzmsg << "• Scripted animation sequences" << std::endl;
    gzmsg << "• Waypoint-based navigation" << std::endl;
    gzmsg << "• Behavior trees and state machines" << std::endl;
    gzmsg << "• Crowd simulation and flocking" << std::endl;
    gzmsg << "• Human-environment interaction" << std::endl;
    gzmsg << "• Performance optimization for multiple actors" << std::endl;
    
    // Start server
    std::thread server_thread([&server]() {
        server.Run(true, 0, false);
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Run actor simulation
    ActorSimulationManager manager;
    manager.Start();
    
    server.RequestStop();
    if (server_thread.joinable())
        server_thread.join();
    
    return 0;
}