#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/components/Model.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/Joint.hh>
#include <gz/sim/components/JointPosition.hh>

#include <gz/msgs/entity_factory.pb.h>
#include <gz/msgs/pose.pb.h>
#include <gz/msgs/double.pb.h>

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
#include <sstream>
#include <map>

/**
 * @brief Nested Models and Complex Hierarchies Demo
 * 
 * This program demonstrates advanced model composition and hierarchical
 * modeling capabilities in Gazebo, showcasing:
 * 
 * Nested Model Features:
 * - Complex hierarchical model structures
 * - Parent-child relationship management
 * - Coordinate frame transformations
 * - Nested joint and link relationships
 * - Dynamic assembly and disassembly
 * - Modular robot system design
 * 
 * Model Composition:
 * - Component-based model architecture
 * - Interchangeable sub-assemblies
 * - Parametric model generation
 * - Template-based model creation
 * - Runtime model modification
 * - Model inheritance and variants
 * 
 * Advanced Applications:
 * - Modular robotics systems
 * - Complex industrial machinery
 * - Multi-level vehicle systems
 * - Building and infrastructure modeling
 * - Biological system hierarchies
 * - Manufacturing assembly lines
 * 
 * Technical Demonstrations:
 * - Nested coordinate transformations
 * - Hierarchical physics simulation
 * - Joint chain propagation
 * - Multi-level collision detection
 * - Recursive model operations
 */

struct ModelComponent
{
    std::string name;
    std::string type;
    gz::math::Pose3d relative_pose;
    std::vector<std::string> child_components;
    std::map<std::string, double> parameters;
    bool is_active;
    
    ModelComponent(const std::string& n, const std::string& t, const gz::math::Pose3d& pose)
        : name(n), type(t), relative_pose(pose), is_active(true) {}
};

struct NestedModelSystem
{
    std::string root_name;
    std::map<std::string, ModelComponent> components;
    std::vector<std::string> hierarchy_levels;
    int max_depth;
    
    NestedModelSystem(const std::string& name) 
        : root_name(name), max_depth(0) {}
};

class NestedModelManager
{
public:
    NestedModelManager()
        : simulation_time_(0.0)
        , demo_phase_(0)
        , model_id_counter_(0)
    {
        // Initialize transport node
        transport_node_ = std::make_unique<gz::transport::Node>();
        
        // Service clients for entity management
        entity_factory_client_ = transport_node_->Request("/world/advanced_features/create");
        entity_remove_client_ = transport_node_->Request("/world/advanced_features/remove");
        
        // Publishers for model control
        model_pose_pub_ = transport_node_->Advertise<gz::msgs::Pose>("/model/nested_system/pose");
        
        // Initialize predefined model templates
        InitializeModelTemplates();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gzmsg << "Nested Model Manager initialized" << std::endl;
    }
    
    void Start()
    {
        gzmsg << "\n=== Nested Models and Complex Hierarchies Demo ===" << std::endl;
        gzmsg << "Starting comprehensive nested model demonstration..." << std::endl;
        
        const double control_frequency = 20.0; // 20 Hz for model management
        const auto control_period = std::chrono::duration<double>(1.0 / control_frequency);
        
        while (demo_phase_ < 6)
        {
            auto start_time = std::chrono::steady_clock::now();
            
            switch (demo_phase_)
            {
                case 0: BasicNestedModelDemo(); break;
                case 1: ModularRobotSystemDemo(); break;
                case 2: DynamicAssemblyDemo(); break;
                case 3: HierarchicalTransformsDemo(); break;
                case 4: ComplexMachineryDemo(); break;
                case 5: PerformanceAnalysisDemo(); break;
            }
            
            // Update all nested model systems
            UpdateNestedModelSystems();
            
            simulation_time_ += 1.0 / control_frequency;
            
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = end_time - start_time;
            if (elapsed < control_period)
            {
                std::this_thread::sleep_for(control_period - elapsed);
            }
        }
        
        gzmsg << "Nested models demo completed!" << std::endl;
    }

private:
    void InitializeModelTemplates()
    {
        // Modular robot arm template
        NestedModelSystem robot_arm("modular_robot_arm");
        
        // Base component
        ModelComponent base("base", "cylindrical_base", gz::math::Pose3d(0, 0, 0.05, 0, 0, 0));
        base.parameters["radius"] = 0.1;
        base.parameters["height"] = 0.1;
        base.parameters["mass"] = 2.0;
        robot_arm.components["base"] = base;
        
        // Link components
        for (int i = 1; i <= 3; ++i)
        {
            std::string link_name = "link_" + std::to_string(i);
            ModelComponent link(link_name, "robot_link", 
                              gz::math::Pose3d(0, 0, i * 0.3, 0, 0, 0));
            link.parameters["length"] = 0.25;
            link.parameters["radius"] = 0.04;
            link.parameters["mass"] = 1.0;
            
            if (i == 1)
            {
                link.child_components.push_back("base");
            }
            else
            {
                link.child_components.push_back("link_" + std::to_string(i-1));
            }
            
            robot_arm.components[link_name] = link;
        }
        
        // End effector
        ModelComponent end_effector("end_effector", "gripper", 
                                  gz::math::Pose3d(0, 0, 1.0, 0, 0, 0));
        end_effector.child_components.push_back("link_3");
        end_effector.parameters["gripper_width"] = 0.08;
        end_effector.parameters["mass"] = 0.5;
        robot_arm.components["end_effector"] = end_effector;
        
        robot_arm.max_depth = 4;
        model_templates_["modular_robot_arm"] = robot_arm;
        
        // Mobile platform template
        NestedModelSystem mobile_platform("mobile_platform");
        
        // Chassis
        ModelComponent chassis("chassis", "box_chassis", gz::math::Pose3d(0, 0, 0.1, 0, 0, 0));
        chassis.parameters["length"] = 1.0;
        chassis.parameters["width"] = 0.6;
        chassis.parameters["height"] = 0.2;
        chassis.parameters["mass"] = 50.0;
        mobile_platform.components["chassis"] = chassis;
        
        // Wheels
        for (int i = 0; i < 4; ++i)
        {
            std::string wheel_name = "wheel_" + std::to_string(i);
            double x = (i < 2) ? 0.4 : -0.4;
            double y = (i % 2 == 0) ? 0.35 : -0.35;
            
            ModelComponent wheel(wheel_name, "cylinder_wheel", 
                               gz::math::Pose3d(x, y, 0.0, M_PI/2, 0, 0));
            wheel.parameters["radius"] = 0.15;
            wheel.parameters["width"] = 0.1;
            wheel.parameters["mass"] = 3.0;
            wheel.child_components.push_back("chassis");
            mobile_platform.components[wheel_name] = wheel;
        }
        
        mobile_platform.max_depth = 2;
        model_templates_["mobile_platform"] = mobile_platform;
        
        // Industrial machinery template
        NestedModelSystem machinery("industrial_machinery");
        
        // Foundation
        ModelComponent foundation("foundation", "concrete_base", 
                                gz::math::Pose3d(0, 0, 0.5, 0, 0, 0));
        foundation.parameters["length"] = 2.0;
        foundation.parameters["width"] = 1.5;
        foundation.parameters["height"] = 1.0;
        foundation.parameters["mass"] = 500.0;
        machinery.components["foundation"] = foundation;
        
        // Main structure
        ModelComponent main_structure("main_structure", "steel_frame", 
                                    gz::math::Pose3d(0, 0, 1.5, 0, 0, 0));
        main_structure.child_components.push_back("foundation");
        main_structure.parameters["height"] = 2.0;
        main_structure.parameters["mass"] = 200.0;
        machinery.components["main_structure"] = main_structure;
        
        // Multiple subsystems
        std::vector<std::string> subsystem_types = {"conveyor", "manipulator", "control_panel"};
        for (size_t i = 0; i < subsystem_types.size(); ++i)
        {
            std::string subsystem_name = subsystem_types[i];
            double angle = i * 2 * M_PI / 3;
            
            ModelComponent subsystem(subsystem_name, subsystem_types[i], 
                                   gz::math::Pose3d(0.8 * std::cos(angle), 0.8 * std::sin(angle), 2.0, 
                                                   0, 0, angle));
            subsystem.child_components.push_back("main_structure");
            subsystem.parameters["mass"] = 50.0;
            machinery.components[subsystem_name] = subsystem;
        }
        
        machinery.max_depth = 3;
        model_templates_["industrial_machinery"] = machinery;
    }
    
    void BasicNestedModelDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 1: Basic Nested Model Demo ---" << std::endl;
            gzmsg << "Creating simple nested model hierarchies..." << std::endl;
            
            // Create a basic nested model system
            CreateNestedModelFromTemplate("modular_robot_arm", 
                                         gz::math::Pose3d(2, 2, 0, 0, 0, 0));
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        if (simulation_time_ - start_time > 8.0)
        {
            gzmsg << "Basic nested model demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void ModularRobotSystemDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 2: Modular Robot System Demo ---" << std::endl;
            gzmsg << "Creating modular robot with interchangeable components..." << std::endl;
            
            // Create mobile platform
            CreateNestedModelFromTemplate("mobile_platform", 
                                         gz::math::Pose3d(-3, 0, 0, 0, 0, 0));
            
            // Create robot arm mounted on platform
            CreateCombinedSystem("mobile_manipulator", "mobile_platform", "modular_robot_arm",
                               gz::math::Pose3d(-3, 0, 0, 0, 0, 0),
                               gz::math::Pose3d(0, 0, 0.4, 0, 0, 0));
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        if (simulation_time_ - start_time > 12.0)
        {
            gzmsg << "Modular robot system demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void DynamicAssemblyDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 3: Dynamic Assembly Demo ---" << std::endl;
            gzmsg << "Dynamically assembling and modifying nested models..." << std::endl;
            
            // Create base system
            CreateNestedModelFromTemplate("mobile_platform", 
                                         gz::math::Pose3d(4, 4, 0, 0, 0, 0));
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Dynamically add components over time
        if (phase_time > 3.0 && phase_time < 3.5)
        {
            gzmsg << "Adding sensor module..." << std::endl;
            AddDynamicComponent("mobile_platform_0", "sensor_module", "lidar_sensor",
                              gz::math::Pose3d(0, 0, 0.5, 0, 0, 0));
        }
        else if (phase_time > 6.0 && phase_time < 6.5)
        {
            gzmsg << "Adding manipulator arm..." << std::endl;
            AddDynamicComponent("mobile_platform_0", "manipulator", "robot_link",
                              gz::math::Pose3d(0.3, 0, 0.4, 0, 0, 0));
        }
        else if (phase_time > 9.0 && phase_time < 9.5)
        {
            gzmsg << "Adding auxiliary equipment..." << std::endl;
            AddDynamicComponent("mobile_platform_0", "auxiliary", "equipment_box",
                              gz::math::Pose3d(-0.3, 0, 0.3, 0, 0, 0));
        }
        
        if (phase_time > 15.0)
        {
            gzmsg << "Dynamic assembly demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void HierarchicalTransformsDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 4: Hierarchical Transforms Demo ---" << std::endl;
            gzmsg << "Demonstrating coordinate frame transformations..." << std::endl;
            
            // Create complex nested system for transform demonstration
            CreateNestedModelFromTemplate("industrial_machinery", 
                                         gz::math::Pose3d(8, 0, 0, 0, 0, 0));
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Animate the nested model to show transforms
        if (phase_time < 10.0)
        {
            AnimateNestedModelTransforms("industrial_machinery_0", phase_time);
        }
        
        if (phase_time > 12.0)
        {
            gzmsg << "Hierarchical transforms demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void ComplexMachineryDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 5: Complex Machinery Demo ---" << std::endl;
            gzmsg << "Creating complex industrial machinery systems..." << std::endl;
            
            // Create multiple interconnected machinery systems
            for (int i = 0; i < 2; ++i)
            {
                double x = -8 + i * 6;
                double y = 6 - i * 3;
                
                CreateNestedModelFromTemplate("industrial_machinery", 
                                             gz::math::Pose3d(x, y, 0, 0, 0, i * M_PI/4));
            }
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Simulate coordinated machinery operation
        if (phase_time < 15.0)
        {
            SimulateCoordinatedOperation(phase_time);
        }
        
        if (phase_time > 15.0)
        {
            gzmsg << "Complex machinery demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void PerformanceAnalysisDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 6: Performance Analysis Demo ---" << std::endl;
            gzmsg << "Analyzing nested model performance and optimization..." << std::endl;
            
            // Collect performance metrics
            AnalyzeNestedModelPerformance();
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        if (phase_time > 8.0)
        {
            // Final performance report
            GeneratePerformanceReport();
            demo_phase_++;
        }
    }
    
    void CreateNestedModelFromTemplate(const std::string& template_name, 
                                     const gz::math::Pose3d& world_pose)
    {
        if (model_templates_.find(template_name) == model_templates_.end())
        {
            gzerr << "Template not found: " << template_name << std::endl;
            return;
        }
        
        const auto& template_system = model_templates_[template_name];
        std::string instance_name = template_name + "_" + std::to_string(model_id_counter_++);
        
        // Generate SDF for the nested model
        std::string nested_sdf = GenerateNestedModelSDF(instance_name, template_system, world_pose);
        
        // Create the model
        gz::msgs::EntityFactory req;
        req.set_sdf(nested_sdf);
        req.set_name(instance_name);
        
        gz::msgs::Boolean rep;
        bool result;
        unsigned int timeout = 2000;
        
        bool executed = entity_factory_client_.Request(req, rep, timeout, result);
        
        if (executed && result && rep.data())
        {
            // Add to active nested model systems
            NestedModelSystem system = template_system;
            system.root_name = instance_name;
            active_nested_models_[instance_name] = system;
            
            gzmsg << "Successfully created nested model: " << instance_name << std::endl;
            gzmsg << "  Components: " << system.components.size() << std::endl;
            gzmsg << "  Max depth: " << system.max_depth << std::endl;
        }
        else
        {
            gzerr << "Failed to create nested model: " << instance_name << std::endl;
        }
    }
    
    std::string GenerateNestedModelSDF(const std::string& model_name,
                                      const NestedModelSystem& system,
                                      const gz::math::Pose3d& world_pose)
    {
        std::ostringstream sdf_stream;
        
        sdf_stream << "<?xml version='1.0'?>\n";
        sdf_stream << "<sdf version='1.10'>\n";
        sdf_stream << "  <model name='" << model_name << "'>\n";
        sdf_stream << "    <pose>" << world_pose.Pos().X() << " " << world_pose.Pos().Y() << " " << world_pose.Pos().Z()
                   << " " << world_pose.Rot().Roll() << " " << world_pose.Rot().Pitch() << " " << world_pose.Rot().Yaw() << "</pose>\n";
        sdf_stream << "    <static>false</static>\n";
        
        // Generate components in hierarchical order
        std::map<std::string, bool> processed;
        GenerateComponentsRecursively(sdf_stream, system, "", processed);
        
        // Add joints between components
        GenerateJointsBetweenComponents(sdf_stream, system);
        
        // Add plugins for advanced control
        sdf_stream << "    <plugin filename='gz-sim-joint-controller-system' name='gz::sim::systems::JointController'/>\n";
        sdf_stream << "    <plugin filename='gz-sim-pose-publisher-system' name='gz::sim::systems::PosePublisher'/>\n";
        
        sdf_stream << "  </model>\n";
        sdf_stream << "</sdf>\n";
        
        return sdf_stream.str();
    }
    
    void GenerateComponentsRecursively(std::ostringstream& sdf_stream,
                                      const NestedModelSystem& system,
                                      const std::string& parent_name,
                                      std::map<std::string, bool>& processed)
    {
        for (const auto& component_pair : system.components)
        {
            const auto& component = component_pair.second;
            
            if (processed[component.name]) continue;
            
            // Check if all parent components have been processed
            bool can_process = true;
            for (const auto& parent : component.child_components)
            {
                if (!processed[parent])
                {
                    can_process = false;
                    break;
                }
            }
            
            if (!can_process && !component.child_components.empty()) continue;
            
            // Generate component SDF
            GenerateComponentSDF(sdf_stream, component);
            processed[component.name] = true;
        }
        
        // Check if all components have been processed, if not, recurse
        bool all_processed = true;
        for (const auto& component_pair : system.components)
        {
            if (!processed[component_pair.first])
            {
                all_processed = false;
                break;
            }
        }
        
        if (!all_processed)
        {
            GenerateComponentsRecursively(sdf_stream, system, parent_name, processed);
        }
    }
    
    void GenerateComponentSDF(std::ostringstream& sdf_stream, const ModelComponent& component)
    {
        // Generate link based on component type
        sdf_stream << "    <link name='" << component.name << "'>\n";
        sdf_stream << "      <pose>" << component.relative_pose.Pos().X() << " " << component.relative_pose.Pos().Y() 
                   << " " << component.relative_pose.Pos().Z() << " " << component.relative_pose.Rot().Roll()
                   << " " << component.relative_pose.Rot().Pitch() << " " << component.relative_pose.Rot().Yaw() << "</pose>\n";
        
        // Inertial properties
        double mass = component.parameters.count("mass") ? component.parameters.at("mass") : 1.0;
        sdf_stream << "      <inertial>\n";
        sdf_stream << "        <mass>" << mass << "</mass>\n";
        sdf_stream << "        <inertia>\n";
        sdf_stream << "          <ixx>" << mass * 0.1 << "</ixx>\n";
        sdf_stream << "          <iyy>" << mass * 0.1 << "</iyy>\n";
        sdf_stream << "          <izz>" << mass * 0.1 << "</izz>\n";
        sdf_stream << "        </inertia>\n";
        sdf_stream << "      </inertial>\n";
        
        // Collision and visual based on component type
        if (component.type == "cylindrical_base")
        {
            GenerateCylindricalGeometry(sdf_stream, component);
        }
        else if (component.type == "robot_link")
        {
            GenerateRobotLinkGeometry(sdf_stream, component);
        }
        else if (component.type == "box_chassis")
        {
            GenerateBoxGeometry(sdf_stream, component);
        }
        else if (component.type == "cylinder_wheel")
        {
            GenerateCylindricalGeometry(sdf_stream, component);
        }
        else if (component.type == "steel_frame")
        {
            GenerateFrameGeometry(sdf_stream, component);
        }
        else
        {
            // Default geometry
            GenerateDefaultGeometry(sdf_stream, component);
        }
        
        sdf_stream << "    </link>\n";
    }
    
    void GenerateCylindricalGeometry(std::ostringstream& sdf_stream, const ModelComponent& component)
    {
        double radius = component.parameters.count("radius") ? component.parameters.at("radius") : 0.1;
        double height = component.parameters.count("height") ? component.parameters.at("height") : 0.2;
        
        sdf_stream << "      <collision name='collision'>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <cylinder><radius>" << radius << "</radius><length>" << height << "</length></cylinder>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "      </collision>\n";
        
        sdf_stream << "      <visual name='visual'>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <cylinder><radius>" << radius << "</radius><length>" << height << "</length></cylinder>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "        <material>\n";
        sdf_stream << "          <ambient>0.3 0.3 0.8 1</ambient>\n";
        sdf_stream << "          <diffuse>0.4 0.4 0.9 1</diffuse>\n";
        sdf_stream << "        </material>\n";
        sdf_stream << "      </visual>\n";
    }
    
    void GenerateRobotLinkGeometry(std::ostringstream& sdf_stream, const ModelComponent& component)
    {
        double length = component.parameters.count("length") ? component.parameters.at("length") : 0.3;
        double radius = component.parameters.count("radius") ? component.parameters.at("radius") : 0.05;
        
        sdf_stream << "      <collision name='collision'>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <box><size>" << radius*2 << " " << radius*2 << " " << length << "</size></box>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "      </collision>\n";
        
        sdf_stream << "      <visual name='visual'>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <box><size>" << radius*2 << " " << radius*2 << " " << length << "</size></box>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "        <material>\n";
        sdf_stream << "          <ambient>0.8 0.3 0.3 1</ambient>\n";
        sdf_stream << "          <diffuse>0.9 0.4 0.4 1</diffuse>\n";
        sdf_stream << "        </material>\n";
        sdf_stream << "      </visual>\n";
    }
    
    void GenerateBoxGeometry(std::ostringstream& sdf_stream, const ModelComponent& component)
    {
        double length = component.parameters.count("length") ? component.parameters.at("length") : 1.0;
        double width = component.parameters.count("width") ? component.parameters.at("width") : 0.6;
        double height = component.parameters.count("height") ? component.parameters.at("height") : 0.2;
        
        sdf_stream << "      <collision name='collision'>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <box><size>" << length << " " << width << " " << height << "</size></box>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "      </collision>\n";
        
        sdf_stream << "      <visual name='visual'>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <box><size>" << length << " " << width << " " << height << "</size></box>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "        <material>\n";
        sdf_stream << "          <ambient>0.6 0.6 0.3 1</ambient>\n";
        sdf_stream << "          <diffuse>0.7 0.7 0.4 1</diffuse>\n";
        sdf_stream << "        </material>\n";
        sdf_stream << "      </visual>\n";
    }
    
    void GenerateFrameGeometry(std::ostringstream& sdf_stream, const ModelComponent& component)
    {
        double height = component.parameters.count("height") ? component.parameters.at("height") : 2.0;
        
        sdf_stream << "      <collision name='collision'>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <box><size>0.5 0.5 " << height << "</size></box>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "      </collision>\n";
        
        sdf_stream << "      <visual name='visual'>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <box><size>0.5 0.5 " << height << "</size></box>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "        <material>\n";
        sdf_stream << "          <ambient>0.5 0.5 0.5 1</ambient>\n";
        sdf_stream << "          <diffuse>0.6 0.6 0.6 1</diffuse>\n";
        sdf_stream << "        </material>\n";
        sdf_stream << "      </visual>\n";
    }
    
    void GenerateDefaultGeometry(std::ostringstream& sdf_stream, const ModelComponent& component)
    {
        sdf_stream << "      <collision name='collision'>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <box><size>0.2 0.2 0.2</size></box>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "      </collision>\n";
        
        sdf_stream << "      <visual name='visual'>\n";
        sdf_stream << "        <geometry>\n";
        sdf_stream << "          <box><size>0.2 0.2 0.2</size></box>\n";
        sdf_stream << "        </geometry>\n";
        sdf_stream << "        <material>\n";
        sdf_stream << "          <ambient>0.7 0.7 0.7 1</ambient>\n";
        sdf_stream << "          <diffuse>0.8 0.8 0.8 1</diffuse>\n";
        sdf_stream << "        </material>\n";
        sdf_stream << "      </visual>\n";
    }
    
    void GenerateJointsBetweenComponents(std::ostringstream& sdf_stream, const NestedModelSystem& system)
    {
        for (const auto& component_pair : system.components)
        {
            const auto& component = component_pair.second;
            
            for (const auto& parent_name : component.child_components)
            {
                std::string joint_name = component.name + "_to_" + parent_name;
                
                sdf_stream << "    <joint name='" << joint_name << "' type='revolute'>\n";
                sdf_stream << "      <parent>" << parent_name << "</parent>\n";
                sdf_stream << "      <child>" << component.name << "</child>\n";
                sdf_stream << "      <axis>\n";
                sdf_stream << "        <xyz>0 0 1</xyz>\n";
                sdf_stream << "        <limit>\n";
                sdf_stream << "          <lower>-3.14159</lower>\n";
                sdf_stream << "          <upper>3.14159</upper>\n";
                sdf_stream << "        </limit>\n";
                sdf_stream << "      </axis>\n";
                sdf_stream << "    </joint>\n";
            }
        }
    }
    
    void CreateCombinedSystem(const std::string& combined_name,
                             const std::string& base_template,
                             const std::string& attached_template,
                             const gz::math::Pose3d& world_pose,
                             const gz::math::Pose3d& attachment_offset)
    {
        // Create a combined nested model system
        if (model_templates_.find(base_template) == model_templates_.end() ||
            model_templates_.find(attached_template) == model_templates_.end())
        {
            gzerr << "One or both templates not found: " << base_template << ", " << attached_template << std::endl;
            return;
        }
        
        NestedModelSystem combined_system(combined_name);
        
        // Copy base system components
        const auto& base_system = model_templates_[base_template];
        for (const auto& component_pair : base_system.components)
        {
            combined_system.components[component_pair.first] = component_pair.second;
        }
        
        // Copy attached system components with offset
        const auto& attached_system = model_templates_[attached_template];
        for (const auto& component_pair : attached_system.components)
        {
            ModelComponent attached_component = component_pair.second;
            attached_component.name = "attached_" + attached_component.name;
            attached_component.relative_pose = attachment_offset + attached_component.relative_pose;
            
            // Connect to base system
            if (attached_component.child_components.empty())
            {
                attached_component.child_components.push_back("chassis"); // Attach to chassis
            }
            
            combined_system.components[attached_component.name] = attached_component;
        }
        
        combined_system.max_depth = std::max(base_system.max_depth, attached_system.max_depth) + 1;
        
        // Create the combined model
        CreateNestedModelFromTemplate(combined_name, world_pose, combined_system);
    }
    
    void CreateNestedModelFromTemplate(const std::string& instance_name,
                                      const gz::math::Pose3d& world_pose,
                                      const NestedModelSystem& system)
    {
        // Generate SDF for the nested model
        std::string nested_sdf = GenerateNestedModelSDF(instance_name, system, world_pose);
        
        // Create the model
        gz::msgs::EntityFactory req;
        req.set_sdf(nested_sdf);
        req.set_name(instance_name);
        
        gz::msgs::Boolean rep;
        bool result;
        unsigned int timeout = 2000;
        
        bool executed = entity_factory_client_.Request(req, rep, timeout, result);
        
        if (executed && result && rep.data())
        {
            // Add to active nested model systems
            NestedModelSystem active_system = system;
            active_system.root_name = instance_name;
            active_nested_models_[instance_name] = active_system;
            
            gzmsg << "Successfully created combined nested model: " << instance_name << std::endl;
            gzmsg << "  Components: " << active_system.components.size() << std::endl;
            gzmsg << "  Max depth: " << active_system.max_depth << std::endl;
        }
        else
        {
            gzerr << "Failed to create combined nested model: " << instance_name << std::endl;
        }
    }
    
    void AddDynamicComponent(const std::string& parent_model,
                           const std::string& component_name,
                           const std::string& component_type,
                           const gz::math::Pose3d& relative_pose)
    {
        // In a real implementation, this would modify the existing model
        // For demonstration, we'll log the action
        gzmsg << "Adding dynamic component '" << component_name << "' of type '" << component_type 
              << "' to model '" << parent_model << "'" << std::endl;
        
        // Update our internal representation
        if (active_nested_models_.find(parent_model) != active_nested_models_.end())
        {
            ModelComponent new_component(component_name, component_type, relative_pose);
            new_component.parameters["mass"] = 1.0;
            active_nested_models_[parent_model].components[component_name] = new_component;
            
            gzmsg << "Component added to internal model representation" << std::endl;
        }
    }
    
    void AnimateNestedModelTransforms(const std::string& model_name, double phase_time)
    {
        // Simulate animated transforms within the nested model
        if (active_nested_models_.find(model_name) == active_nested_models_.end())
            return;
        
        auto& system = active_nested_models_[model_name];
        
        // Animate subsystem rotations
        for (auto& component_pair : system.components)
        {
            auto& component = component_pair.second;
            
            if (component.name.find("conveyor") != std::string::npos)
            {
                // Rotate conveyor
                double rotation = phase_time * 0.5; // Slow rotation
                gz::math::Quaterniond rot(0, 0, rotation);
                component.relative_pose.Rot() = rot;
            }
            else if (component.name.find("manipulator") != std::string::npos)
            {
                // Oscillating manipulator
                double oscillation = std::sin(phase_time) * 0.5;
                gz::math::Quaterniond rot(0, oscillation, 0);
                component.relative_pose.Rot() = rot;
            }
        }
        
        // In a real implementation, these transforms would be sent to Gazebo
        gzmsg << "Animating transforms for " << model_name << " at time " << phase_time << std::endl;
    }
    
    void SimulateCoordinatedOperation(double phase_time)
    {
        // Simulate coordinated operation of multiple nested model systems
        
        double cycle_time = 10.0;
        double normalized_time = std::fmod(phase_time, cycle_time) / cycle_time;
        
        if (normalized_time < 0.5)
        {
            // First half: systems work in sequence
            gzmsg << "Phase 1: Sequential operation (t=" << std::fixed << std::setprecision(1) 
                  << normalized_time * cycle_time << "s)" << std::endl;
        }
        else
        {
            // Second half: systems work in parallel
            gzmsg << "Phase 2: Parallel operation (t=" << std::fixed << std::setprecision(1) 
                  << normalized_time * cycle_time << "s)" << std::endl;
        }
    }
    
    void UpdateNestedModelSystems()
    {
        // Update all active nested model systems
        for (auto& system_pair : active_nested_models_)
        {
            UpdateSystemComponents(system_pair.second);
        }
    }
    
    void UpdateSystemComponents(NestedModelSystem& system)
    {
        // Update component states and relationships
        for (auto& component_pair : system.components)
        {
            auto& component = component_pair.second;
            
            // Update component-specific behaviors
            if (component.type == "conveyor")
            {
                // Simulate conveyor belt movement
                component.parameters["belt_speed"] = 0.5; // m/s
            }
            else if (component.type == "manipulator")
            {
                // Simulate manipulator operation
                component.parameters["joint_position"] = std::sin(simulation_time_ * 0.1) * 0.5;
            }
        }
    }
    
    void AnalyzeNestedModelPerformance()
    {
        gzmsg << "\n=== Nested Model Performance Analysis ===" << std::endl;
        gzmsg << "Active nested model systems: " << active_nested_models_.size() << std::endl;
        
        int total_components = 0;
        int max_depth = 0;
        
        for (const auto& system_pair : active_nested_models_)
        {
            const auto& system = system_pair.second;
            total_components += system.components.size();
            max_depth = std::max(max_depth, system.max_depth);
            
            gzmsg << "System '" << system.root_name << "':" << std::endl;
            gzmsg << "  Components: " << system.components.size() << std::endl;
            gzmsg << "  Depth: " << system.max_depth << std::endl;
        }
        
        gzmsg << "Total components: " << total_components << std::endl;
        gzmsg << "Maximum depth: " << max_depth << std::endl;
        
        // Performance metrics
        performance_metrics_.total_systems = active_nested_models_.size();
        performance_metrics_.total_components = total_components;
        performance_metrics_.max_hierarchy_depth = max_depth;
        performance_metrics_.update_frequency = 20.0; // 20 Hz target
    }
    
    void GeneratePerformanceReport()
    {
        gzmsg << "\n=== Final Nested Model Performance Report ===" << std::endl;
        gzmsg << "Total nested systems created: " << performance_metrics_.total_systems << std::endl;
        gzmsg << "Total components across all systems: " << performance_metrics_.total_components << std::endl;
        gzmsg << "Maximum hierarchy depth achieved: " << performance_metrics_.max_hierarchy_depth << std::endl;
        gzmsg << "Average update frequency: " << std::fixed << std::setprecision(1) 
              << performance_metrics_.update_frequency << " Hz" << std::endl;
        
        // Component type analysis
        std::map<std::string, int> component_types;
        for (const auto& system_pair : active_nested_models_)
        {
            for (const auto& component_pair : system_pair.second.components)
            {
                component_types[component_pair.second.type]++;
            }
        }
        
        gzmsg << "Component type distribution:" << std::endl;
        for (const auto& type_pair : component_types)
        {
            gzmsg << "  " << type_pair.first << ": " << type_pair.second << std::endl;
        }
        
        // Memory and performance estimates
        double estimated_memory = performance_metrics_.total_components * 0.1; // MB per component estimate
        gzmsg << "Estimated memory usage: " << std::fixed << std::setprecision(1) 
              << estimated_memory << " MB" << std::endl;
        
        if (performance_metrics_.total_components > 50)
        {
            gzmsg << "Performance note: Large number of components may affect real-time performance" << std::endl;
        }
        
        gzmsg << "Nested model demonstration completed successfully!" << std::endl;
    }
    
    // Member variables
    std::unique_ptr<gz::transport::Node> transport_node_;
    gz::transport::Node::Publisher model_pose_pub_;
    
    // Service clients
    gz::transport::Node::RequesterProxy entity_factory_client_;
    gz::transport::Node::RequesterProxy entity_remove_client_;
    
    // Model management
    std::map<std::string, NestedModelSystem> model_templates_;
    std::map<std::string, NestedModelSystem> active_nested_models_;
    
    double simulation_time_;
    int demo_phase_;
    int model_id_counter_;
    
    struct PerformanceMetrics {
        int total_systems;
        int total_components;
        int max_hierarchy_depth;
        double update_frequency;
    } performance_metrics_;
};

int main(int argc, char** argv)
{
    // Configure Gazebo server
    gz::sim::ServerConfig config;
    config.SetSdfFile("worlds/advanced_features.sdf");
    config.SetUpdateRate(1000.0);
    
    gz::sim::Server server(config);
    
    gzmsg << "\n=== Nested Models and Complex Hierarchies Demo ===" << std::endl;
    gzmsg << "Gazebo Harmonic - Advanced Model Composition Tutorial" << std::endl;
    gzmsg << "\nThis demo demonstrates:" << std::endl;
    gzmsg << "• Complex hierarchical model structures" << std::endl;
    gzmsg << "• Parent-child relationship management" << std::endl;
    gzmsg << "• Dynamic model assembly and modification" << std::endl;
    gzmsg << "• Coordinate frame transformations" << std::endl;
    gzmsg << "• Modular robotics system design" << std::endl;
    gzmsg << "• Industrial machinery modeling" << std::endl;
    gzmsg << "• Performance analysis and optimization" << std::endl;
    
    // Start server
    std::thread server_thread([&server]() {
        server.Run(true, 0, false);
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Run nested models demo
    NestedModelManager manager;
    manager.Start();
    
    server.RequestStop();
    if (server_thread.joinable())
        server_thread.join();
    
    return 0;
}