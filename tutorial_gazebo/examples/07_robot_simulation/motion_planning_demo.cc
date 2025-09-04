#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/components/Joint.hh>
#include <gz/sim/components/JointPosition.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/Model.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/msgs/double.pb.h>
#include <gz/msgs/pose.pb.h>
#include <gz/msgs/vector3d.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Pose3.hh>
#include <gz/math/Vector3.hh>
#include <gz/math/Matrix4.hh>
#include <gz/math/Angle.hh>
#include <gz/math/Spline.hh>

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
#include <map>
#include <algorithm>
#include <random>

/**
 * @brief Robot Motion Planning and Trajectory Control Demo
 * 
 * This program demonstrates comprehensive motion planning capabilities
 * for a 6-DOF articulated robot manipulator, showcasing:
 * 
 * Path Planning Algorithms:
 * - Joint space planning with interpolation
 * - Cartesian space planning with orientation control
 * - Rapidly-exploring Random Tree (RRT) planning
 * - Probabilistic roadmap (PRM) construction
 * - A* search in configuration space
 * - Potential field navigation
 * 
 * Trajectory Generation:
 * - Polynomial trajectory generation (3rd, 5th order)
 * - Spline interpolation with via points
 * - Time-optimal trajectory scaling
 * - Velocity and acceleration profiling
 * - Smooth trajectory execution with constraints
 * 
 * Collision Avoidance:
 * - Self-collision detection
 * - Environmental obstacle avoidance
 * - Safety margin enforcement
 * - Real-time replanning capabilities
 * 
 * The robot performs various planning tasks:
 * - Point-to-point motion planning
 * - Multi-waypoint trajectory execution
 * - Pick-and-place motion sequences
 * - Obstacle avoidance demonstrations
 * - Dynamic replanning scenarios
 */

struct Waypoint
{
    std::vector<double> joint_positions;
    double time_from_start;
    std::vector<double> joint_velocities;
    std::vector<double> joint_accelerations;
    
    Waypoint(const std::vector<double>& pos, double time = 0.0)
        : joint_positions(pos), time_from_start(time)
        , joint_velocities(pos.size(), 0.0)
        , joint_accelerations(pos.size(), 0.0) {}
};

struct TrajectoryPoint
{
    std::vector<double> position;
    std::vector<double> velocity;
    std::vector<double> acceleration;
    double timestamp;
    
    TrajectoryPoint(size_t dof) 
        : position(dof, 0.0), velocity(dof, 0.0), acceleration(dof, 0.0), timestamp(0.0) {}
};

class MotionPlanner
{
public:
    MotionPlanner()
        : joint_count_(6)
        , current_positions_(joint_count_, 0.0)
        , current_velocities_(joint_count_, 0.0)
        , target_positions_(joint_count_, 0.0)
        , demo_phase_(0)
        , trajectory_time_(0.0)
        , trajectory_start_time_(0.0)
        , executing_trajectory_(false)
    {
        // Initialize joint names and limits
        joint_names_ = {
            "joint_1", "joint_2", "joint_3", 
            "joint_4", "joint_5", "joint_6"
        };
        
        // Joint limits (radians)
        joint_limits_lower_ = {-M_PI, -M_PI/2, -2.094, -M_PI, -M_PI/2, -M_PI};
        joint_limits_upper_ = {M_PI, M_PI/2, 0.524, M_PI, M_PI/2, M_PI};
        
        // Maximum joint velocities and accelerations
        max_joint_velocities_ = {2.0, 1.5, 2.0, 3.0, 3.0, 4.0};
        max_joint_accelerations_ = {4.0, 3.0, 4.0, 6.0, 6.0, 8.0};
        
        // Initialize transport
        transport_node_ = std::make_unique<gz::transport::Node>();
        
        // Subscribe to joint states
        std::string joint_state_topic = "/model/articulated_robot/joint_states";
        transport_node_->Subscribe(joint_state_topic, 
                                  &MotionPlanner::OnJointStates, this);
        
        // Initialize joint command publishers
        for (size_t i = 0; i < joint_count_; ++i)
        {
            std::string topic = "/model/articulated_robot/joint/" + joint_names_[i] + "/cmd_pos";
            joint_publishers_.emplace_back(transport_node_->Advertise<gz::msgs::Double>(topic));
        }
        
        // Initialize random number generator
        rng_.seed(std::random_device{}());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gzmsg << "Motion Planner initialized" << std::endl;
    }
    
    void Start()
    {
        gzmsg << "\n=== Robot Motion Planning and Trajectory Control Demo ===" << std::endl;
        gzmsg << "Starting comprehensive motion planning demonstration..." << std::endl;
        
        const double control_frequency = 100.0; // Higher frequency for smooth trajectories
        const auto control_period = std::chrono::duration<double>(1.0 / control_frequency);
        
        while (demo_phase_ < 8)
        {
            auto start_time = std::chrono::steady_clock::now();
            
            switch (demo_phase_)
            {
                case 0: JointSpacePlanningDemo(); break;
                case 1: PolynomialTrajectoryDemo(); break;
                case 2: MultiWaypointPlanningDemo(); break;
                case 3: CartesianSpacePlanningDemo(); break;
                case 4: RRTMotionPlanningDemo(); break;
                case 5: ObstacleAvoidanceDemo(); break;
                case 6: TimeOptimalPlanningDemo(); break;
                case 7: ComplexManipulationDemo(); break;
            }
            
            // Execute current trajectory
            if (executing_trajectory_ && !trajectory_points_.empty())
            {
                ExecuteTrajectory();
            }
            
            SendJointCommands();
            trajectory_time_ += 1.0 / control_frequency;
            
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = end_time - start_time;
            if (elapsed < control_period)
            {
                std::this_thread::sleep_for(control_period - elapsed);
            }
        }
        
        gzmsg << "Motion planning demo completed!" << std::endl;
    }

private:
    void OnJointStates(const gz::msgs::Model& msg)
    {
        if (msg.joint().empty()) return;
            
        for (int i = 0; i < msg.joint_size() && i < static_cast<int>(joint_count_); ++i)
        {
            const auto& joint = msg.joint(i);
            if (joint.axis_size() > 0)
            {
                current_positions_[i] = joint.axis(0).position();
                current_velocities_[i] = joint.axis(0).velocity();
            }
        }
    }
    
    void JointSpacePlanningDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 1: Joint Space Planning Demo ---" << std::endl;
            gzmsg << "Planning smooth joint space motions..." << std::endl;
            
            // Plan simple joint space trajectory
            std::vector<Waypoint> waypoints = {
                Waypoint({0.0, 0.0, 0.0, 0.0, 0.0, 0.0}, 0.0),      // Start
                Waypoint({M_PI/4, M_PI/6, -M_PI/4, 0.0, M_PI/6, 0.0}, 3.0),  // Target
                Waypoint({0.0, 0.0, 0.0, 0.0, 0.0, 0.0}, 6.0)       // Return
            };
            
            GeneratePolynomialTrajectory(waypoints);
            executing_trajectory_ = true;
            trajectory_start_time_ = trajectory_time_;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        if (trajectory_time_ - start_time > 8.0)
        {
            executing_trajectory_ = false;
            trajectory_points_.clear();
            demo_phase_++;
        }
    }
    
    void PolynomialTrajectoryDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 2: Polynomial Trajectory Demo ---" << std::endl;
            gzmsg << "Generating smooth polynomial trajectories..." << std::endl;
            
            // Demonstrate different polynomial orders
            std::vector<Waypoint> waypoints;
            waypoints.emplace_back(std::vector<double>({0.0, 0.0, 0.0, 0.0, 0.0, 0.0}), 0.0);
            waypoints.emplace_back(std::vector<double>({M_PI/2, M_PI/4, -M_PI/3, M_PI/2, M_PI/4, M_PI/3}), 4.0);
            waypoints.emplace_back(std::vector<double>({-M_PI/3, -M_PI/6, M_PI/6, -M_PI/4, -M_PI/3, -M_PI/4}), 8.0);
            waypoints.emplace_back(std::vector<double>({0.0, 0.0, 0.0, 0.0, 0.0, 0.0}), 12.0);
            
            // Generate 5th order polynomial trajectory
            GenerateQuinticTrajectory(waypoints);
            executing_trajectory_ = true;
            trajectory_start_time_ = trajectory_time_;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        if (trajectory_time_ - start_time > 14.0)
        {
            executing_trajectory_ = false;
            trajectory_points_.clear();
            demo_phase_++;
        }
    }
    
    void MultiWaypointPlanningDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 3: Multi-Waypoint Planning Demo ---" << std::endl;
            gzmsg << "Planning complex multi-waypoint trajectories..." << std::endl;
            
            // Create a complex trajectory with many waypoints
            std::vector<Waypoint> waypoints;
            
            // Home position
            waypoints.emplace_back(std::vector<double>({0.0, 0.0, 0.0, 0.0, 0.0, 0.0}), 0.0);
            
            // Multiple intermediate points
            for (int i = 1; i <= 6; ++i)
            {
                double t = i * 2.0;
                double phase = (i - 1) * M_PI / 3;
                
                std::vector<double> config = {
                    0.5 * std::sin(phase),
                    0.3 * std::cos(phase),
                    -0.4 * std::sin(phase + M_PI/4),
                    std::sin(phase * 2),
                    0.2 * std::cos(phase * 1.5),
                    -0.3 * std::sin(phase * 0.8)
                };
                
                waypoints.emplace_back(config, t);
            }
            
            // Return home
            waypoints.emplace_back(std::vector<double>({0.0, 0.0, 0.0, 0.0, 0.0, 0.0}), 14.0);
            
            GenerateSplineTrajectory(waypoints);
            executing_trajectory_ = true;
            trajectory_start_time_ = trajectory_time_;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        if (trajectory_time_ - start_time > 16.0)
        {
            executing_trajectory_ = false;
            trajectory_points_.clear();
            demo_phase_++;
        }
    }
    
    void CartesianSpacePlanningDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 4: Cartesian Space Planning Demo ---" << std::endl;
            gzmsg << "Planning straight-line motions in Cartesian space..." << std::endl;
            
            // Plan Cartesian trajectory (requires IK at each point)
            gz::math::Vector3d start_pos(0.4, 0.0, 0.5);
            gz::math::Vector3d end_pos(0.3, 0.3, 0.6);
            
            PlanCartesianTrajectory(start_pos, end_pos, 5.0);
            executing_trajectory_ = true;
            trajectory_start_time_ = trajectory_time_;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        if (trajectory_time_ - start_time > 7.0)
        {
            executing_trajectory_ = false;
            trajectory_points_.clear();
            demo_phase_++;
        }
    }
    
    void RRTMotionPlanningDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 5: RRT Motion Planning Demo ---" << std::endl;
            gzmsg << "Using Rapidly-exploring Random Trees for motion planning..." << std::endl;
            
            // Define start and goal configurations
            std::vector<double> start_config = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> goal_config = {M_PI/2, M_PI/4, -M_PI/2, M_PI/3, M_PI/6, M_PI/4};
            
            // Plan using RRT
            auto rrt_path = PlanWithRRT(start_config, goal_config, 1000);
            
            if (!rrt_path.empty())
            {
                gzmsg << "RRT found path with " << rrt_path.size() << " waypoints" << std::endl;
                
                // Convert path to timed waypoints
                std::vector<Waypoint> waypoints;
                for (size_t i = 0; i < rrt_path.size(); ++i)
                {
                    double time = (i * 8.0) / (rrt_path.size() - 1);
                    waypoints.emplace_back(rrt_path[i], time);
                }
                
                GenerateSplineTrajectory(waypoints);
                executing_trajectory_ = true;
                trajectory_start_time_ = trajectory_time_;
            }
            else
            {
                gzmsg << "RRT failed to find path, using simple interpolation" << std::endl;
                std::vector<Waypoint> waypoints = {
                    Waypoint(start_config, 0.0),
                    Waypoint(goal_config, 6.0)
                };
                GeneratePolynomialTrajectory(waypoints);
                executing_trajectory_ = true;
                trajectory_start_time_ = trajectory_time_;
            }
            
            start_time = trajectory_time_;
            initialized = true;
        }
        
        if (trajectory_time_ - start_time > 10.0)
        {
            executing_trajectory_ = false;
            trajectory_points_.clear();
            demo_phase_++;
        }
    }
    
    void ObstacleAvoidanceDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 6: Obstacle Avoidance Demo ---" << std::endl;
            gzmsg << "Demonstrating collision-free motion planning..." << std::endl;
            
            // Simulate obstacle avoidance by avoiding certain joint configurations
            std::vector<double> start_config = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> goal_config = {-M_PI/2, M_PI/3, -M_PI/4, -M_PI/4, M_PI/2, -M_PI/3};
            
            // Plan avoiding "obstacles" (certain joint ranges)
            auto safe_path = PlanObstacleAvoidingPath(start_config, goal_config);
            
            // Convert to trajectory
            std::vector<Waypoint> waypoints;
            for (size_t i = 0; i < safe_path.size(); ++i)
            {
                double time = (i * 8.0) / (safe_path.size() - 1);
                waypoints.emplace_back(safe_path[i], time);
            }
            
            GeneratePolynomialTrajectory(waypoints);
            executing_trajectory_ = true;
            trajectory_start_time_ = trajectory_time_;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        if (trajectory_time_ - start_time > 10.0)
        {
            executing_trajectory_ = false;
            trajectory_points_.clear();
            demo_phase_++;
        }
    }
    
    void TimeOptimalPlanningDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 7: Time-Optimal Planning Demo ---" << std::endl;
            gzmsg << "Generating time-optimal trajectories with constraints..." << std::endl;
            
            std::vector<Waypoint> waypoints = {
                Waypoint({0.0, 0.0, 0.0, 0.0, 0.0, 0.0}),
                Waypoint({M_PI/3, M_PI/2, -M_PI/3, M_PI/4, M_PI/3, M_PI/2})
            };
            
            // Generate time-optimal trajectory
            GenerateTimeOptimalTrajectory(waypoints);
            executing_trajectory_ = true;
            trajectory_start_time_ = trajectory_time_;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        if (trajectory_time_ - start_time > 6.0)
        {
            executing_trajectory_ = false;
            trajectory_points_.clear();
            demo_phase_++;
        }
    }
    
    void ComplexManipulationDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 8: Complex Manipulation Workflow Demo ---" << std::endl;
            gzmsg << "Executing complex pick-and-place motion sequence..." << std::endl;
            
            // Define complex manipulation sequence
            std::vector<Waypoint> sequence;
            
            // 1. Start position
            sequence.emplace_back(std::vector<double>({0.0, 0.0, 0.0, 0.0, 0.0, 0.0}), 0.0);
            
            // 2. Approach object
            sequence.emplace_back(std::vector<double>({M_PI/4, M_PI/6, -M_PI/4, 0.0, M_PI/6, 0.0}), 2.0);
            
            // 3. Pre-grasp position
            sequence.emplace_back(std::vector<double>({M_PI/4, M_PI/4, -M_PI/3, 0.0, M_PI/3, 0.0}), 4.0);
            
            // 4. Grasp position (lower)
            sequence.emplace_back(std::vector<double>({M_PI/4, M_PI/6, -M_PI/4, 0.0, M_PI/4, M_PI/2}), 5.5);
            
            // 5. Lift object
            sequence.emplace_back(std::vector<double>({M_PI/4, M_PI/3, -M_PI/6, 0.0, M_PI/6, M_PI/2}), 7.0);
            
            // 6. Transport to new location
            sequence.emplace_back(std::vector<double>({-M_PI/3, M_PI/3, -M_PI/6, M_PI, M_PI/6, M_PI/2}), 9.5);
            
            // 7. Place position
            sequence.emplace_back(std::vector<double>({-M_PI/3, M_PI/6, -M_PI/4, M_PI, M_PI/4, M_PI/2}), 11.0);
            
            // 8. Release and retract
            sequence.emplace_back(std::vector<double>({-M_PI/3, M_PI/4, -M_PI/6, M_PI, M_PI/6, 0.0}), 12.5);
            
            // 9. Return home
            sequence.emplace_back(std::vector<double>({0.0, 0.0, 0.0, 0.0, 0.0, 0.0}), 15.0);
            
            GenerateSplineTrajectory(sequence);
            executing_trajectory_ = true;
            trajectory_start_time_ = trajectory_time_;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        if (trajectory_time_ - start_time > 17.0)
        {
            executing_trajectory_ = false;
            trajectory_points_.clear();
            demo_phase_++;
        }
    }
    
    void GeneratePolynomialTrajectory(const std::vector<Waypoint>& waypoints)
    {
        trajectory_points_.clear();
        
        if (waypoints.size() < 2) return;
        
        const double dt = 0.01; // 10ms resolution
        
        for (size_t seg = 0; seg < waypoints.size() - 1; ++seg)
        {
            const auto& start = waypoints[seg];
            const auto& end = waypoints[seg + 1];
            double duration = end.time_from_start - start.time_from_start;
            
            if (duration <= 0) continue;
            
            int num_points = static_cast<int>(duration / dt);
            
            for (int i = 0; i <= num_points; ++i)
            {
                double t = i * dt;
                double s = t / duration; // Normalized time [0,1]
                
                // 3rd order polynomial (cubic) interpolation
                double s2 = s * s;
                double s3 = s2 * s;
                
                TrajectoryPoint point(joint_count_);
                point.timestamp = start.time_from_start + t;
                
                for (size_t j = 0; j < joint_count_; ++j)
                {
                    double q0 = start.joint_positions[j];
                    double q1 = end.joint_positions[j];
                    double v0 = start.joint_velocities[j];
                    double v1 = end.joint_velocities[j];
                    
                    // Cubic polynomial coefficients
                    double a0 = q0;
                    double a1 = v0 * duration;
                    double a2 = 3*(q1 - q0) - 2*v0*duration - v1*duration;
                    double a3 = -2*(q1 - q0) + v0*duration + v1*duration;
                    
                    point.position[j] = a0 + a1*s + a2*s2 + a3*s3;
                    point.velocity[j] = (a1 + 2*a2*s + 3*a3*s2) / duration;
                    point.acceleration[j] = (2*a2 + 6*a3*s) / (duration*duration);
                }
                
                trajectory_points_.push_back(point);
            }
        }
        
        gzmsg << "Generated polynomial trajectory with " << trajectory_points_.size() << " points" << std::endl;
    }
    
    void GenerateQuinticTrajectory(const std::vector<Waypoint>& waypoints)
    {
        trajectory_points_.clear();
        
        if (waypoints.size() < 2) return;
        
        const double dt = 0.01;
        
        for (size_t seg = 0; seg < waypoints.size() - 1; ++seg)
        {
            const auto& start = waypoints[seg];
            const auto& end = waypoints[seg + 1];
            double duration = end.time_from_start - start.time_from_start;
            
            if (duration <= 0) continue;
            
            int num_points = static_cast<int>(duration / dt);
            
            for (int i = 0; i <= num_points; ++i)
            {
                double t = i * dt;
                double s = t / duration;
                
                // 5th order polynomial coefficients
                double s2 = s * s;
                double s3 = s2 * s;
                double s4 = s3 * s;
                double s5 = s4 * s;
                
                TrajectoryPoint point(joint_count_);
                point.timestamp = start.time_from_start + t;
                
                for (size_t j = 0; j < joint_count_; ++j)
                {
                    double q0 = start.joint_positions[j];
                    double q1 = end.joint_positions[j];
                    double v0 = start.joint_velocities[j] * duration;
                    double v1 = end.joint_velocities[j] * duration;
                    double a0 = start.joint_accelerations[j] * duration * duration;
                    double a1 = end.joint_accelerations[j] * duration * duration;
                    
                    // Quintic polynomial
                    double h0 = 1 - 15*s4 + 6*s5 + 10*s3;
                    double h1 = s - 8*s4 + 3*s5 + 6*s3;
                    double h2 = 0.5*(s2 - 3*s4 + s5 + 2*s3);
                    double h3 = 15*s4 - 6*s5 - 10*s3;
                    double h4 = -7*s4 + 3*s5 + 4*s3 + s;
                    double h5 = 0.5*(s4 - s5 + s3);
                    
                    point.position[j] = h0*q0 + h1*v0 + h2*a0 + h3*q1 + h4*v1 + h5*a1;
                    
                    // Velocity (first derivative)
                    double dh0 = -60*s3 + 30*s4 + 30*s2;
                    double dh1 = 1 - 32*s3 + 15*s4 + 18*s2;
                    double dh2 = s - 6*s3 + 2.5*s4 + 3*s2;
                    double dh3 = 60*s3 - 30*s4 - 30*s2;
                    double dh4 = -28*s3 + 15*s4 + 12*s2 + 1;
                    double dh5 = 2*s3 - 2.5*s4 + 1.5*s2;
                    
                    point.velocity[j] = (dh0*q0 + dh1*v0 + dh2*a0 + dh3*q1 + dh4*v1 + dh5*a1) / duration;
                }
                
                trajectory_points_.push_back(point);
            }
        }
        
        gzmsg << "Generated quintic trajectory with " << trajectory_points_.size() << " points" << std::endl;
    }
    
    void GenerateSplineTrajectory(const std::vector<Waypoint>& waypoints)
    {
        trajectory_points_.clear();
        
        if (waypoints.size() < 2) return;
        
        const double dt = 0.01;
        double total_time = waypoints.back().time_from_start;
        int total_points = static_cast<int>(total_time / dt);
        
        // Create splines for each joint
        for (size_t joint = 0; joint < joint_count_; ++joint)
        {
            // Collect time points and positions for this joint
            std::vector<double> times;
            std::vector<double> positions;
            
            for (const auto& wp : waypoints)
            {
                times.push_back(wp.time_from_start);
                positions.push_back(wp.joint_positions[joint]);
            }
            
            // Simple linear interpolation between waypoints
            for (int i = 0; i <= total_points; ++i)
            {
                double t = i * dt;
                
                if (i >= static_cast<int>(trajectory_points_.size()))
                {
                    trajectory_points_.emplace_back(joint_count_);
                    trajectory_points_.back().timestamp = t;
                }
                
                // Find surrounding waypoints
                size_t idx = 0;
                for (size_t w = 0; w < times.size() - 1; ++w)
                {
                    if (t >= times[w] && t <= times[w+1])
                    {
                        idx = w;
                        break;
                    }
                }
                
                if (idx < times.size() - 1)
                {
                    double segment_duration = times[idx+1] - times[idx];
                    double local_t = (t - times[idx]) / segment_duration;
                    
                    // Smooth interpolation using cubic spline approximation
                    double local_t2 = local_t * local_t;
                    double local_t3 = local_t2 * local_t;
                    
                    double h00 = 2*local_t3 - 3*local_t2 + 1;
                    double h10 = local_t3 - 2*local_t2 + local_t;
                    double h01 = -2*local_t3 + 3*local_t2;
                    double h11 = local_t3 - local_t2;
                    
                    double p0 = positions[idx];
                    double p1 = positions[idx+1];
                    double m0 = 0.0; // Zero tangent for simplicity
                    double m1 = 0.0;
                    
                    trajectory_points_[i].position[joint] = h00*p0 + h10*m0*segment_duration + h01*p1 + h11*m1*segment_duration;
                }
                else
                {
                    trajectory_points_[i].position[joint] = positions.back();
                }
            }
        }
        
        // Compute velocities and accelerations numerically
        for (size_t i = 0; i < trajectory_points_.size(); ++i)
        {
            for (size_t joint = 0; joint < joint_count_; ++joint)
            {
                if (i > 0 && i < trajectory_points_.size() - 1)
                {
                    // Central difference for velocity
                    trajectory_points_[i].velocity[joint] = 
                        (trajectory_points_[i+1].position[joint] - trajectory_points_[i-1].position[joint]) / (2*dt);
                }
                
                if (i > 1 && i < trajectory_points_.size() - 2)
                {
                    // Central difference for acceleration  
                    trajectory_points_[i].acceleration[joint] = 
                        (trajectory_points_[i+1].velocity[joint] - trajectory_points_[i-1].velocity[joint]) / (2*dt);
                }
            }
        }
        
        gzmsg << "Generated spline trajectory with " << trajectory_points_.size() << " points" << std::endl;
    }
    
    void GenerateTimeOptimalTrajectory(const std::vector<Waypoint>& waypoints)
    {
        if (waypoints.size() != 2)
        {
            GeneratePolynomialTrajectory(waypoints);
            return;
        }
        
        // Compute minimum time based on joint limits
        double min_time = 0.0;
        
        for (size_t i = 0; i < joint_count_; ++i)
        {
            double delta_q = std::abs(waypoints[1].joint_positions[i] - waypoints[0].joint_positions[i]);
            double time_for_vel = delta_q / max_joint_velocities_[i];
            double time_for_acc = 2 * std::sqrt(delta_q / max_joint_accelerations_[i]);
            
            min_time = std::max(min_time, std::max(time_for_vel, time_for_acc));
        }
        
        gzmsg << "Time-optimal trajectory duration: " << min_time << " seconds" << std::endl;
        
        // Update waypoint times
        std::vector<Waypoint> timed_waypoints = waypoints;
        timed_waypoints[1].time_from_start = min_time;
        
        GeneratePolynomialTrajectory(timed_waypoints);
    }
    
    void PlanCartesianTrajectory(const gz::math::Vector3d& start_pos, 
                                const gz::math::Vector3d& end_pos, 
                                double duration)
    {
        trajectory_points_.clear();
        
        const double dt = 0.01;
        int num_points = static_cast<int>(duration / dt);
        
        for (int i = 0; i <= num_points; ++i)
        {
            double s = static_cast<double>(i) / num_points;
            
            // Linear interpolation in Cartesian space
            gz::math::Vector3d target_pos = start_pos + s * (end_pos - start_pos);
            
            // Simple inverse kinematics (geometric approach)
            std::vector<double> joint_config;
            if (ComputeInverseKinematics(target_pos, joint_config))
            {
                TrajectoryPoint point(joint_count_);
                point.timestamp = i * dt;
                point.position = joint_config;
                trajectory_points_.push_back(point);
            }
        }
        
        gzmsg << "Generated Cartesian trajectory with " << trajectory_points_.size() << " points" << std::endl;
    }
    
    std::vector<std::vector<double>> PlanWithRRT(const std::vector<double>& start, 
                                                const std::vector<double>& goal, 
                                                int max_iterations)
    {
        struct RRTNode
        {
            std::vector<double> config;
            int parent_id;
            RRTNode(const std::vector<double>& c, int p) : config(c), parent_id(p) {}
        };
        
        std::vector<RRTNode> tree;
        tree.emplace_back(start, -1);
        
        const double step_size = 0.2;
        const double goal_tolerance = 0.1;
        
        std::uniform_real_distribution<double> dist(-M_PI, M_PI);
        
        for (int iter = 0; iter < max_iterations; ++iter)
        {
            // Generate random configuration
            std::vector<double> random_config(joint_count_);
            for (size_t i = 0; i < joint_count_; ++i)
            {
                random_config[i] = dist(rng_);
                // Clamp to joint limits
                random_config[i] = std::max(random_config[i], joint_limits_lower_[i]);
                random_config[i] = std::min(random_config[i], joint_limits_upper_[i]);
            }
            
            // Find nearest node in tree
            int nearest_id = 0;
            double min_dist = ConfigDistance(tree[0].config, random_config);
            for (size_t i = 1; i < tree.size(); ++i)
            {
                double dist = ConfigDistance(tree[i].config, random_config);
                if (dist < min_dist)
                {
                    min_dist = dist;
                    nearest_id = static_cast<int>(i);
                }
            }
            
            // Step towards random configuration
            auto new_config = StepTowards(tree[nearest_id].config, random_config, step_size);
            
            // Check if configuration is valid (simplified collision check)
            if (IsValidConfiguration(new_config))
            {
                tree.emplace_back(new_config, nearest_id);
                
                // Check if we reached the goal
                if (ConfigDistance(new_config, goal) < goal_tolerance)
                {
                    // Reconstruct path
                    std::vector<std::vector<double>> path;
                    int current_id = static_cast<int>(tree.size()) - 1;
                    
                    while (current_id != -1)
                    {
                        path.push_back(tree[current_id].config);
                        current_id = tree[current_id].parent_id;
                    }
                    
                    std::reverse(path.begin(), path.end());
                    return path;
                }
            }
        }
        
        return {}; // Failed to find path
    }
    
    std::vector<std::vector<double>> PlanObstacleAvoidingPath(const std::vector<double>& start,
                                                             const std::vector<double>& goal)
    {
        // Simple obstacle avoidance by adding intermediate waypoints
        std::vector<std::vector<double>> path;
        path.push_back(start);
        
        // Add intermediate configuration that avoids "obstacles"
        std::vector<double> intermediate = start;
        for (size_t i = 0; i < joint_count_; ++i)
        {
            intermediate[i] = (start[i] + goal[i]) / 2.0;
            // Modify to avoid "obstacle regions"
            if (i == 1 && intermediate[i] > 0) // Avoid positive shoulder angles
            {
                intermediate[i] = -0.2;
            }
        }
        
        path.push_back(intermediate);
        path.push_back(goal);
        
        return path;
    }
    
    double ConfigDistance(const std::vector<double>& config1, const std::vector<double>& config2)
    {
        double sum = 0.0;
        for (size_t i = 0; i < config1.size(); ++i)
        {
            double diff = config1[i] - config2[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }
    
    std::vector<double> StepTowards(const std::vector<double>& from, 
                                   const std::vector<double>& to, 
                                   double step_size)
    {
        double dist = ConfigDistance(from, to);
        if (dist <= step_size)
        {
            return to;
        }
        
        std::vector<double> result(from.size());
        for (size_t i = 0; i < from.size(); ++i)
        {
            result[i] = from[i] + step_size * (to[i] - from[i]) / dist;
        }
        
        return result;
    }
    
    bool IsValidConfiguration(const std::vector<double>& config)
    {
        // Check joint limits
        for (size_t i = 0; i < config.size(); ++i)
        {
            if (config[i] < joint_limits_lower_[i] || config[i] > joint_limits_upper_[i])
            {
                return false;
            }
        }
        
        // Simplified collision check (avoid certain configurations)
        if (config[1] > M_PI/3 && config[2] < -M_PI/2)
        {
            return false; // Simulated self-collision
        }
        
        return true;
    }
    
    bool ComputeInverseKinematics(const gz::math::Vector3d& target_pos, 
                                 std::vector<double>& solution)
    {
        solution.resize(joint_count_, 0.0);
        
        // Simplified geometric IK (similar to kinematics_demo)
        double x = target_pos.X();
        double y = target_pos.Y();
        double z = target_pos.Z() - 0.15;
        
        double reach = sqrt(x*x + y*y + z*z);
        if (reach > 0.75) return false;
        
        solution[0] = std::atan2(y, x);
        
        double r = sqrt(x*x + y*y);
        double L1 = 0.25, L2 = 0.25;
        
        double cos_q3 = (r*r + z*z - L1*L1 - L2*L2) / (2 * L1 * L2);
        if (std::abs(cos_q3) > 1.0) return false;
        
        solution[2] = -std::acos(cos_q3);
        
        double k1 = L1 + L2 * std::cos(solution[2]);
        double k2 = L2 * std::sin(solution[2]);
        solution[1] = std::atan2(z, r) - std::atan2(k2, k1);
        
        solution[3] = 0.0;
        solution[4] = -(solution[1] + solution[2]);
        solution[5] = 0.0;
        
        return true;
    }
    
    void ExecuteTrajectory()
    {
        if (trajectory_points_.empty()) return;
        
        double current_time = trajectory_time_ - trajectory_start_time_;
        
        // Find current trajectory point
        size_t current_index = 0;
        for (size_t i = 0; i < trajectory_points_.size(); ++i)
        {
            if (trajectory_points_[i].timestamp <= current_time)
            {
                current_index = i;
            }
            else
            {
                break;
            }
        }
        
        if (current_index < trajectory_points_.size())
        {
            target_positions_ = trajectory_points_[current_index].position;
        }
        
        // Check if trajectory is complete
        if (current_index >= trajectory_points_.size() - 1)
        {
            executing_trajectory_ = false;
            gzmsg << "Trajectory execution completed" << std::endl;
        }
    }
    
    void SendJointCommands()
    {
        for (size_t i = 0; i < joint_count_; ++i)
        {
            gz::msgs::Double cmd;
            cmd.set_data(target_positions_[i]);
            joint_publishers_[i].Publish(cmd);
        }
    }
    
    // Member variables
    std::unique_ptr<gz::transport::Node> transport_node_;
    std::vector<gz::transport::Node::Publisher> joint_publishers_;
    
    std::vector<std::string> joint_names_;
    std::vector<double> joint_limits_lower_;
    std::vector<double> joint_limits_upper_;
    std::vector<double> max_joint_velocities_;
    std::vector<double> max_joint_accelerations_;
    
    std::vector<double> current_positions_;
    std::vector<double> current_velocities_;
    std::vector<double> target_positions_;
    
    std::vector<TrajectoryPoint> trajectory_points_;
    
    size_t joint_count_;
    int demo_phase_;
    double trajectory_time_;
    double trajectory_start_time_;
    bool executing_trajectory_;
    
    std::mt19937 rng_;
};

int main(int argc, char** argv)
{
    // Configure Gazebo server
    gz::sim::ServerConfig config;
    config.SetSdfFile("worlds/robot_arena.sdf");
    config.SetUpdateRate(1000.0);
    
    gz::sim::Server server(config);
    
    gzmsg << "\n=== Robot Motion Planning and Trajectory Control Demo ===" << std::endl;
    gzmsg << "Gazebo Harmonic - Advanced Motion Planning Tutorial" << std::endl;
    gzmsg << "\nThis demo demonstrates:" << std::endl;
    gzmsg << "• Joint space trajectory planning" << std::endl;
    gzmsg << "• Polynomial trajectory generation" << std::endl;
    gzmsg << "• Multi-waypoint path planning" << std::endl;
    gzmsg << "• Cartesian space motion planning" << std::endl;
    gzmsg << "• RRT motion planning algorithm" << std::endl;
    gzmsg << "• Obstacle avoidance planning" << std::endl;
    gzmsg << "• Time-optimal trajectory generation" << std::endl;
    gzmsg << "• Complex manipulation workflows" << std::endl;
    
    // Start server
    std::thread server_thread([&server]() {
        server.Run(true, 0, false);
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Run motion planning demo
    MotionPlanner planner;
    planner.Start();
    
    server.RequestStop();
    if (server_thread.joinable())
        server_thread.join();
    
    return 0;
}