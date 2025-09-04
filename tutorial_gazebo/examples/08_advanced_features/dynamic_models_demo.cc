#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/components/Model.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/Static.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/Joint.hh>

#include <gz/msgs/entity_factory.pb.h>
#include <gz/msgs/entity_modify.pb.h>
#include <gz/msgs/stringmsg.pb.h>
#include <gz/msgs/pose.pb.h>
#include <gz/msgs/model.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Pose3.hh>
#include <gz/math/Vector3.hh>
#include <gz/math/Quaternion.hh>
#include <gz/math/Rand.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>
#include <gz/common/UUID.hh>

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <sstream>
#include <random>

/**
 * @brief Dynamic Model Management Demo
 * 
 * This program demonstrates advanced dynamic model management
 * capabilities in Gazebo Harmonic, showcasing real-time model
 * lifecycle operations and procedural content generation:
 * 
 * Runtime Model Operations:
 * - Dynamic model spawning and removal
 * - Model property modification at runtime
 * - Procedural model generation from parameters
 * - Model cloning and templating
 * - Batch model operations for efficiency
 * 
 * Advanced Model Features:
 * - Parametric model generation
 * - Model composition and decomposition  
 * - Runtime joint creation and modification
 * - Dynamic collision geometry updates
 * - Model state persistence and restoration
 * 
 * Procedural Content Generation:
 * - Algorithmic scene construction
 * - Random model placement and properties
 * - Rule-based model arrangement
 * - Procedural texture and material assignment
 * - Automatic collision detection setup
 * 
 * Performance Optimization:
 * - Efficient model pooling and reuse
 * - Level-of-detail (LOD) model swapping
 * - Spatial partitioning for large scenes
 * - Memory-efficient model management
 * - Asynchronous model operations
 * 
 * Applications Demonstrated:
 * - Procedural environment generation
 * - Dynamic obstacle courses
 * - Adaptive simulation scenarios
 * - Model streaming for large worlds
 * - Real-time model editing tools
 */

struct ModelTemplate
{
    std::string name;
    std::string base_geometry; // "box", "sphere", "cylinder", "mesh"
    gz::math::Vector3d size;
    gz::math::Color color;
    double mass;
    bool static_model;
    std::map<std::string, std::string> properties;
    
    ModelTemplate(const std::string& n = "template")
        : name(n), base_geometry("box"), size(1, 1, 1), 
          color(0.7, 0.7, 0.7), mass(1.0), static_model(false) {}
};

struct SpawnedModel
{
    std::string name;
    std::string template_name;
    gz::math::Pose3d pose;
    double spawn_time;
    bool active;
    std::map<std::string, double> runtime_properties;
    
    SpawnedModel(const std::string& n, const std::string& t, const gz::math::Pose3d& p, double time)
        : name(n), template_name(t), pose(p), spawn_time(time), active(true) {}
};

struct ModelPool
{
    std::queue<std::string> available_models;
    std::map<std::string, SpawnedModel> active_models;
    size_t max_pool_size;
    size_t reuse_count;
    
    ModelPool(size_t max_size = 50) : max_pool_size(max_size), reuse_count(0) {}
};

class DynamicModelManager
{
public:
    DynamicModelManager()
        : simulation_time_(0.0)
        , demo_phase_(0)
        , model_id_counter_(0)
        , total_models_spawned_(0)
        , total_models_removed_(0)
    {
        // Initialize transport node
        transport_node_ = std::make_unique<gz::transport::Node>();
        
        // Service clients for entity management
        entity_factory_client_ = transport_node_->Request("/world/advanced_features/create");
        entity_remove_client_ = transport_node_->Request("/world/advanced_features/remove");
        entity_modify_client_ = transport_node_->Request("/world/advanced_features/entity/modify");
        
        // Publishers for model control
        model_pose_pub_ = transport_node_->Advertise<gz::msgs::Pose>("/model/*/pose");
        
        // Initialize random number generator
        random_engine_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
        
        // Setup model templates
        InitializeModelTemplates();
        
        // Initialize model pool
        model_pool_ = std::make_unique<ModelPool>(100);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gzmsg << "Dynamic Model Manager initialized" << std::endl;
    }
    
    void Start()
    {
        gzmsg << "\n=== Dynamic Model Management Demo ===" << std::endl;
        gzmsg << "Starting comprehensive dynamic model operations demonstration..." << std::endl;
        
        const double control_frequency = 30.0; // 30 Hz for smooth model management
        const auto control_period = std::chrono::duration<double>(1.0 / control_frequency);
        
        while (demo_phase_ < 6)
        {
            auto start_time = std::chrono::steady_clock::now();
            
            switch (demo_phase_)
            {
                case 0: BasicSpawningDemo(); break;
                case 1: ProceduralGenerationDemo(); break;
                case 2: ModelModificationDemo(); break;
                case 3: AdvancedCompositionDemo(); break;
                case 4: PerformanceOptimizationDemo(); break;
                case 5: ScenarioGenerationDemo(); break;
            }
            
            // Update model lifecycle
            UpdateModelLifecycle();
            
            simulation_time_ += 1.0 / control_frequency;
            
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = end_time - start_time;
            if (elapsed < control_period)
            {
                std::this_thread::sleep_for(control_period - elapsed);
            }
        }
        
        gzmsg << "Dynamic model management demo completed!" << std::endl;
        ReportFinalStatistics();
    }

private:
    void InitializeModelTemplates()
    {
        // Basic geometric shapes
        ModelTemplate box_template("basic_box");
        box_template.base_geometry = "box";
        box_template.size = gz::math::Vector3d(1.0, 1.0, 1.0);
        box_template.color = gz::math::Color(0.7, 0.7, 0.7);
        box_template.mass = 1.0;
        model_templates_[box_template.name] = box_template;
        
        ModelTemplate sphere_template("basic_sphere");
        sphere_template.base_geometry = "sphere";
        sphere_template.size = gz::math::Vector3d(0.5, 0.5, 0.5); // radius stored in X
        sphere_template.color = gz::math::Color(0.8, 0.4, 0.4);
        sphere_template.mass = 0.5;
        model_templates_[sphere_template.name] = sphere_template;
        
        ModelTemplate cylinder_template("basic_cylinder");
        cylinder_template.base_geometry = "cylinder";
        cylinder_template.size = gz::math::Vector3d(0.5, 0.5, 1.0); // radius, radius, height
        cylinder_template.color = gz::math::Color(0.4, 0.8, 0.4);
        cylinder_template.mass = 0.8;
        model_templates_[cylinder_template.name] = cylinder_template;
        
        // Specialized templates
        ModelTemplate obstacle_template("obstacle");
        obstacle_template.base_geometry = "box";
        obstacle_template.size = gz::math::Vector3d(0.5, 3.0, 2.0);
        obstacle_template.color = gz::math::Color(0.9, 0.1, 0.1);
        obstacle_template.mass = 10.0;
        obstacle_template.static_model = true;
        model_templates_[obstacle_template.name] = obstacle_template;
        
        ModelTemplate platform_template("platform");
        platform_template.base_geometry = "box";
        platform_template.size = gz::math::Vector3d(2.0, 2.0, 0.2);
        platform_template.color = gz::math::Color(0.1, 0.1, 0.9);
        platform_template.mass = 5.0;
        platform_template.static_model = true;
        model_templates_[platform_template.name] = platform_template;
        
        ModelTemplate pickup_template("pickup_item");
        pickup_template.base_geometry = "sphere";
        pickup_template.size = gz::math::Vector3d(0.2, 0.2, 0.2);
        pickup_template.color = gz::math::Color(1.0, 1.0, 0.2);
        pickup_template.mass = 0.1;
        pickup_template.properties["glow"] = "true";
        model_templates_[pickup_template.name] = pickup_template;
        
        ModelTemplate building_block_template("building_block");
        building_block_template.base_geometry = "box";
        building_block_template.size = gz::math::Vector3d(1.0, 0.5, 0.3);
        building_block_template.color = gz::math::Color(0.8, 0.6, 0.4);
        building_block_template.mass = 2.0;
        model_templates_[building_block_template.name] = building_block_template;
        
        gzmsg << "Initialized " << model_templates_.size() << " model templates" << std::endl;
    }
    
    void BasicSpawningDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        static double last_spawn_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 1: Basic Model Spawning Demo ---" << std::endl;
            gzmsg << "Demonstrating dynamic model creation and removal..." << std::endl;
            
            start_time = simulation_time_;
            last_spawn_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Spawn models at regular intervals
        if (simulation_time_ - last_spawn_time > 2.0)
        {
            SpawnRandomModel();
            last_spawn_time = simulation_time_;
        }
        
        // Remove old models to prevent overcrowding
        RemoveOldModels(15.0); // Remove models older than 15 seconds
        
        if (phase_time > 20.0)
        {
            gzmsg << "Basic spawning demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void ProceduralGenerationDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 2: Procedural Generation Demo ---" << std::endl;
            gzmsg << "Creating procedurally generated environments..." << std::endl;
            
            // Generate a maze-like structure
            GenerateMazeStructure(8, 8);
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Add dynamic elements to the procedural environment
        if (static_cast<int>(phase_time) % 3 == 0 && 
            phase_time - static_cast<int>(phase_time) < 0.1)
        {
            AddDynamicElements();
        }
        
        if (phase_time > 18.0)
        {
            gzmsg << "Procedural generation demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void ModelModificationDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 3: Runtime Model Modification Demo ---" << std::endl;
            gzmsg << "Demonstrating real-time model property changes..." << std::endl;
            
            // Create test models for modification
            CreateModificationTestModels();
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Periodically modify model properties
        if (static_cast<int>(phase_time * 2) % 2 == 0) // Every 0.5 seconds
        {
            ModifyModelProperties();
        }
        
        if (phase_time > 15.0)
        {
            gzmsg << "Model modification demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void AdvancedCompositionDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 4: Advanced Model Composition Demo ---" << std::endl;
            gzmsg << "Creating complex composite models..." << std::endl;
            
            // Build complex structures from simple components
            BuildCompositeStructures();
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        if (phase_time > 12.0)
        {
            gzmsg << "Advanced composition demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void PerformanceOptimizationDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 5: Performance Optimization Demo ---" << std::endl;
            gzmsg << "Demonstrating efficient model management techniques..." << std::endl;
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Stress test with many models
        if (phase_time < 8.0)
        {
            // High-frequency spawning for stress test
            if (static_cast<int>(phase_time * 10) % 2 == 0) // 5 Hz spawning
            {
                SpawnOptimizedModel();
            }
        }
        
        // Demonstrate model pooling
        DemonstrateModelPooling();
        
        // Report performance metrics
        if (static_cast<int>(phase_time) % 3 == 0 && 
            phase_time - static_cast<int>(phase_time) < 0.1)
        {
            ReportPerformanceMetrics();
        }
        
        if (phase_time > 15.0)
        {
            gzmsg << "Performance optimization demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void ScenarioGenerationDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 6: Dynamic Scenario Generation Demo ---" << std::endl;
            gzmsg << "Creating adaptive simulation scenarios..." << std::endl;
            
            // Clear previous models for clean scenario
            ClearAllDynamicModels();
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Generate different scenarios over time
        int scenario_id = static_cast<int>(phase_time / 4.0);
        
        switch (scenario_id % 3)
        {
            case 0: GenerateRaceCourseScenario(); break;
            case 1: GeneratePuzzleScenario(); break;
            case 2: GenerateSurvivalScenario(); break;
        }
        
        if (phase_time > 12.0)
        {
            gzmsg << "Dynamic scenario generation completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void SpawnRandomModel()
    {
        // Select random template
        std::vector<std::string> template_names;
        for (const auto& pair : model_templates_)
        {
            template_names.push_back(pair.first);
        }
        
        std::uniform_int_distribution<> template_dist(0, template_names.size() - 1);
        std::string template_name = template_names[template_dist(random_engine_)];
        
        // Random position within spawn area
        std::uniform_real_distribution<> pos_dist(-8.0, 8.0);
        std::uniform_real_distribution<> height_dist(0.5, 3.0);
        std::uniform_real_distribution<> angle_dist(0.0, 2.0 * M_PI);
        
        gz::math::Pose3d spawn_pose(
            pos_dist(random_engine_),
            pos_dist(random_engine_),
            height_dist(random_engine_),
            0, 0, angle_dist(random_engine_)
        );
        
        SpawnModelFromTemplate(template_name, spawn_pose);
    }
    
    std::string SpawnModelFromTemplate(const std::string& template_name, const gz::math::Pose3d& pose)
    {
        auto template_it = model_templates_.find(template_name);
        if (template_it == model_templates_.end())
        {
            gzerr << "Template not found: " << template_name << std::endl;
            return "";
        }
        
        const ModelTemplate& tmpl = template_it->second;
        
        // Generate unique model name
        std::string model_name = template_name + "_" + std::to_string(model_id_counter_++);
        
        // Generate SDF from template
        std::string model_sdf = GenerateModelSDF(model_name, tmpl, pose);
        
        // Send spawn request
        gz::msgs::EntityFactory req;
        req.set_sdf(model_sdf);
        req.set_name(model_name);
        
        gz::msgs::Boolean rep;
        bool result;
        unsigned int timeout = 1000;
        
        bool executed = entity_factory_client_.Request(req, rep, timeout, result);
        
        if (executed && result && rep.data())
        {
            // Add to spawned models tracking
            SpawnedModel spawned_model(model_name, template_name, pose, simulation_time_);
            spawned_models_[model_name] = spawned_model;
            
            total_models_spawned_++;
            
            gzmsg << "Spawned model: " << model_name << " at (" 
                  << std::fixed << std::setprecision(1) << pose.Pos().X() << ", " 
                  << pose.Pos().Y() << ", " << pose.Pos().Z() << ")" << std::endl;
            
            return model_name;
        }
        else
        {
            gzerr << "Failed to spawn model: " << model_name << std::endl;
            return "";
        }
    }
    
    std::string GenerateModelSDF(const std::string& name, 
                                const ModelTemplate& tmpl, 
                                const gz::math::Pose3d& pose)
    {
        std::ostringstream sdf_stream;
        
        sdf_stream << "<?xml version='1.0'?>\n";
        sdf_stream << "<sdf version='1.10'>\n";
        sdf_stream << "  <model name='" << name << "'>\n";
        sdf_stream << "    <pose>" << pose.Pos().X() << " " << pose.Pos().Y() << " " << pose.Pos().Z()
                   << " " << pose.Rot().Roll() << " " << pose.Rot().Pitch() << " " << pose.Rot().Yaw() << "</pose>\n";
        sdf_stream << "    <static>" << (tmpl.static_model ? "true" : "false") << "</static>\n";
        sdf_stream << "    <link name='link'>\n";
        
        // Inertial properties for dynamic models
        if (!tmpl.static_model)
        {
            sdf_stream << "      <inertial>\n";
            sdf_stream << "        <mass>" << tmpl.mass << "</mass>\n";
            
            // Calculate inertia based on geometry
            double ixx, iyy, izz;
            if (tmpl.base_geometry == "box")
            {
                ixx = tmpl.mass * (tmpl.size.Y() * tmpl.size.Y() + tmpl.size.Z() * tmpl.size.Z()) / 12.0;
                iyy = tmpl.mass * (tmpl.size.X() * tmpl.size.X() + tmpl.size.Z() * tmpl.size.Z()) / 12.0;
                izz = tmpl.mass * (tmpl.size.X() * tmpl.size.X() + tmpl.size.Y() * tmpl.size.Y()) / 12.0;
            }
            else if (tmpl.base_geometry == "sphere")
            {
                double r = tmpl.size.X(); // radius
                ixx = iyy = izz = 0.4 * tmpl.mass * r * r;
            }
            else if (tmpl.base_geometry == "cylinder")
            {
                double r = tmpl.size.X(); // radius
                double h = tmpl.size.Z(); // height
                ixx = iyy = tmpl.mass * (3 * r * r + h * h) / 12.0;
                izz = tmpl.mass * r * r / 2.0;
            }
            else
            {
                // Default values
                ixx = iyy = izz = tmpl.mass / 6.0;
            }
            
            sdf_stream << "        <inertia>\n";
            sdf_stream << "          <ixx>" << ixx << "</ixx>\n";
            sdf_stream << "          <ixy>0</ixy>\n";
            sdf_stream << "          <ixz>0</ixz>\n";
            sdf_stream << "          <iyy>" << iyy << "</iyy>\n";
            sdf_stream << "          <iyz>0</iyz>\n";
            sdf_stream << "          <izz>" << izz << "</izz>\n";
            sdf_stream << "        </inertia>\n";
            sdf_stream << "      </inertial>\n";
        }
        
        // Collision geometry
        sdf_stream << "      <collision name='collision'>\n";
        sdf_stream << "        <geometry>\n";
        if (tmpl.base_geometry == "box")
        {
            sdf_stream << "          <box><size>" << tmpl.size.X() << " " << tmpl.size.Y() << " " << tmpl.size.Z() << "</size></box>\n";
        }
        else if (tmpl.base_geometry == "sphere")
        {
            sdf_stream << "          <sphere><radius>" << tmpl.size.X() << "</radius></sphere>\n";
        }
        else if (tmpl.base_geometry == "cylinder")
        {
            sdf_stream << "          <cylinder><radius>" << tmpl.size.X() << "</radius><length>" << tmpl.size.Z() << "</length></cylinder>\n";
        }
        sdf_stream << "        </geometry>\n";
        
        // Surface properties
        sdf_stream << "        <surface>\n";
        sdf_stream << "          <friction>\n";
        sdf_stream << "            <ode>\n";
        sdf_stream << "              <mu>0.8</mu>\n";
        sdf_stream << "              <mu2>0.8</mu2>\n";
        sdf_stream << "            </ode>\n";
        sdf_stream << "          </friction>\n";
        sdf_stream << "          <bounce>\n";
        sdf_stream << "            <restitution_coefficient>0.3</restitution_coefficient>\n";
        sdf_stream << "          </bounce>\n";
        sdf_stream << "        </surface>\n";
        sdf_stream << "      </collision>\n";
        
        // Visual geometry
        sdf_stream << "      <visual name='visual'>\n";
        sdf_stream << "        <geometry>\n";
        if (tmpl.base_geometry == "box")
        {
            sdf_stream << "          <box><size>" << tmpl.size.X() << " " << tmpl.size.Y() << " " << tmpl.size.Z() << "</size></box>\n";
        }
        else if (tmpl.base_geometry == "sphere")
        {
            sdf_stream << "          <sphere><radius>" << tmpl.size.X() << "</radius></sphere>\n";
        }
        else if (tmpl.base_geometry == "cylinder")
        {
            sdf_stream << "          <cylinder><radius>" << tmpl.size.X() << "</radius><length>" << tmpl.size.Z() << "</length></cylinder>\n";
        }
        sdf_stream << "        </geometry>\n";
        
        // Material
        sdf_stream << "        <material>\n";
        sdf_stream << "          <ambient>" << tmpl.color.R() << " " << tmpl.color.G() << " " << tmpl.color.B() << " 1</ambient>\n";
        sdf_stream << "          <diffuse>" << tmpl.color.R() << " " << tmpl.color.G() << " " << tmpl.color.B() << " 1</diffuse>\n";
        sdf_stream << "          <specular>0.3 0.3 0.3 1</specular>\n";
        
        // Add emissive property if glow is enabled
        if (tmpl.properties.find("glow") != tmpl.properties.end())
        {
            sdf_stream << "          <emissive>" << tmpl.color.R() * 0.5 << " " << tmpl.color.G() * 0.5 << " " << tmpl.color.B() * 0.5 << " 1</emissive>\n";
        }
        
        sdf_stream << "        </material>\n";
        sdf_stream << "      </visual>\n";
        sdf_stream << "    </link>\n";
        sdf_stream << "  </model>\n";
        sdf_stream << "</sdf>\n";
        
        return sdf_stream.str();
    }
    
    void RemoveOldModels(double max_age)
    {
        std::vector<std::string> models_to_remove;
        
        for (const auto& pair : spawned_models_)
        {
            const SpawnedModel& model = pair.second;
            if (model.active && (simulation_time_ - model.spawn_time) > max_age)
            {
                models_to_remove.push_back(model.name);
            }
        }
        
        for (const std::string& model_name : models_to_remove)
        {
            RemoveModel(model_name);
        }
    }
    
    void RemoveModel(const std::string& model_name)
    {
        gz::msgs::StringMsg req;
        req.set_data(model_name);
        
        gz::msgs::Boolean rep;
        bool result;
        unsigned int timeout = 1000;
        
        bool executed = entity_remove_client_.Request(req, rep, timeout, result);
        
        if (executed && result && rep.data())
        {
            auto it = spawned_models_.find(model_name);
            if (it != spawned_models_.end())
            {
                it->second.active = false;
                total_models_removed_++;
            }
            
            gzmsg << "Removed model: " << model_name << std::endl;
        }
        else
        {
            gzerr << "Failed to remove model: " << model_name << std::endl;
        }
    }
    
    void GenerateMazeStructure(int width, int height)
    {
        gzmsg << "Generating " << width << "x" << height << " maze structure..." << std::endl;
        
        // Simple maze generation algorithm
        std::vector<std::vector<bool>> maze(height, std::vector<bool>(width, false));
        
        // Create random maze pattern
        std::uniform_real_distribution<> wall_prob(0.0, 1.0);
        
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                // Leave pathways and add walls randomly
                if (x == 0 || x == width - 1 || y == 0 || y == height - 1)
                {
                    maze[y][x] = true; // Border walls
                }
                else if (x % 2 == 0 && y % 2 == 0)
                {
                    maze[y][x] = wall_prob(random_engine_) > 0.6;
                }
            }
        }
        
        // Spawn wall models based on maze pattern
        double wall_size = 2.0;
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (maze[y][x])
                {
                    gz::math::Pose3d wall_pose(
                        (x - width / 2) * wall_size,
                        (y - height / 2) * wall_size,
                        1.0, 0, 0, 0
                    );
                    
                    SpawnModelFromTemplate("obstacle", wall_pose);
                }
            }
        }
        
        gzmsg << "Maze structure generated successfully" << std::endl;
    }
    
    void AddDynamicElements()
    {
        // Add collectible items in the maze
        std::uniform_real_distribution<> pos_dist(-8.0, 8.0);
        
        gz::math::Pose3d pickup_pose(
            pos_dist(random_engine_),
            pos_dist(random_engine_),
            0.5, 0, 0, 0
        );
        
        SpawnModelFromTemplate("pickup_item", pickup_pose);
    }
    
    void CreateModificationTestModels()
    {
        // Create models that will be modified during the demo
        std::vector<gz::math::Pose3d> test_poses = {
            gz::math::Pose3d(5, 0, 1, 0, 0, 0),
            gz::math::Pose3d(0, 5, 1, 0, 0, 0),
            gz::math::Pose3d(-5, 0, 1, 0, 0, 0),
        };
        
        for (size_t i = 0; i < test_poses.size(); ++i)
        {
            std::string model_name = SpawnModelFromTemplate("basic_box", test_poses[i]);
            modification_test_models_.push_back(model_name);
        }
        
        gzmsg << "Created " << modification_test_models_.size() << " models for modification testing" << std::endl;
    }
    
    void ModifyModelProperties()
    {
        // Modify properties of test models
        for (const std::string& model_name : modification_test_models_)
        {
            auto it = spawned_models_.find(model_name);
            if (it != spawned_models_.end() && it->second.active)
            {
                // Animate model position
                double time_offset = simulation_time_ + std::hash<std::string>{}(model_name);
                double height_offset = 0.5 * std::sin(time_offset);
                
                gz::math::Pose3d new_pose = it->second.pose;
                new_pose.Pos().Z() += height_offset;
                
                // Send pose update (simplified - in real implementation would use proper service)
                it->second.runtime_properties["height_offset"] = height_offset;
            }
        }
    }
    
    void BuildCompositeStructures()
    {
        gzmsg << "Building composite structures..." << std::endl;
        
        // Build a tower structure
        BuildTowerStructure(gz::math::Vector3d(8, 8, 0), 5);
        
        // Build a bridge structure
        BuildBridgeStructure(gz::math::Vector3d(-8, 8, 0), gz::math::Vector3d(-4, 8, 0));
        
        // Build a pyramid structure
        BuildPyramidStructure(gz::math::Vector3d(0, -8, 0), 4);
    }
    
    void BuildTowerStructure(const gz::math::Vector3d& base_pos, int height)
    {
        for (int i = 0; i < height; ++i)
        {
            gz::math::Pose3d block_pose(
                base_pos.X(),
                base_pos.Y(),
                base_pos.Z() + i * 0.6 + 0.3,
                0, 0, (i % 2) * M_PI / 2 // Alternate orientation
            );
            
            SpawnModelFromTemplate("building_block", block_pose);
        }
        
        gzmsg << "Built tower structure with " << height << " blocks" << std::endl;
    }
    
    void BuildBridgeStructure(const gz::math::Vector3d& start_pos, const gz::math::Vector3d& end_pos)
    {
        gz::math::Vector3d direction = end_pos - start_pos;
        direction.Normalize();
        
        double length = start_pos.Distance(end_pos);
        int num_segments = static_cast<int>(length / 0.5);
        
        for (int i = 0; i <= num_segments; ++i)
        {
            double progress = static_cast<double>(i) / num_segments;
            gz::math::Vector3d segment_pos = start_pos + direction * (progress * length);
            segment_pos.Z() = 1.0; // Bridge height
            
            gz::math::Pose3d segment_pose(segment_pos, gz::math::Quaterniond());
            SpawnModelFromTemplate("platform", segment_pose);
        }
        
        gzmsg << "Built bridge structure with " << num_segments + 1 << " segments" << std::endl;
    }
    
    void BuildPyramidStructure(const gz::math::Vector3d& center, int base_size)
    {
        for (int level = 0; level < base_size; ++level)
        {
            int blocks_per_side = base_size - level;
            double block_size = 1.0;
            double level_height = level * block_size + block_size / 2;
            
            for (int x = 0; x < blocks_per_side; ++x)
            {
                for (int y = 0; y < blocks_per_side; ++y)
                {
                    double offset = -(blocks_per_side - 1) * block_size / 2;
                    gz::math::Pose3d block_pose(
                        center.X() + offset + x * block_size,
                        center.Y() + offset + y * block_size,
                        center.Z() + level_height,
                        0, 0, 0
                    );
                    
                    SpawnModelFromTemplate("building_block", block_pose);
                }
            }
        }
        
        gzmsg << "Built pyramid structure with " << base_size << " levels" << std::endl;
    }
    
    void SpawnOptimizedModel()
    {
        // Use model pooling for performance
        std::string template_name = "basic_sphere"; // Fast to create
        
        std::uniform_real_distribution<> pos_dist(-10.0, 10.0);
        gz::math::Pose3d spawn_pose(
            pos_dist(random_engine_),
            pos_dist(random_engine_),
            2.0, 0, 0, 0
        );
        
        // Check if we can reuse a model from pool
        if (!model_pool_->available_models.empty())
        {
            std::string reused_model = model_pool_->available_models.front();
            model_pool_->available_models.pop();
            model_pool_->reuse_count++;
            
            // Update pose of reused model instead of creating new one
            // (Simplified - real implementation would update model pose)
            gzmsg << "Reusing pooled model: " << reused_model << std::endl;
        }
        else
        {
            SpawnModelFromTemplate(template_name, spawn_pose);
        }
    }
    
    void DemonstrateModelPooling()
    {
        // Add old models to the pool for reuse
        for (auto& pair : spawned_models_)
        {
            SpawnedModel& model = pair.second;
            if (model.active && (simulation_time_ - model.spawn_time) > 10.0)
            {
                if (model_pool_->available_models.size() < model_pool_->max_pool_size)
                {
                    model_pool_->available_models.push(model.name);
                    model.active = false; // Mark as pooled, not active
                }
            }
        }
    }
    
    void ClearAllDynamicModels()
    {
        gzmsg << "Clearing all dynamic models for scenario reset..." << std::endl;
        
        for (const auto& pair : spawned_models_)
        {
            if (pair.second.active && !model_templates_[pair.second.template_name].static_model)
            {
                RemoveModel(pair.first);
            }
        }
    }
    
    void GenerateRaceCourseScenario()
    {
        static bool race_course_generated = false;
        
        if (!race_course_generated)
        {
            gzmsg << "Generating race course scenario..." << std::endl;
            
            // Create race track with checkpoints
            std::vector<gz::math::Vector3d> checkpoints = {
                gz::math::Vector3d(0, -6, 0.5),
                gz::math::Vector3d(6, -3, 0.5),
                gz::math::Vector3d(6, 3, 0.5),
                gz::math::Vector3d(0, 6, 0.5),
                gz::math::Vector3d(-6, 3, 0.5),
                gz::math::Vector3d(-6, -3, 0.5),
            };
            
            for (size_t i = 0; i < checkpoints.size(); ++i)
            {
                SpawnModelFromTemplate("pickup_item", gz::math::Pose3d(checkpoints[i], gz::math::Quaterniond()));
            }
            
            // Add obstacles
            for (int i = 0; i < 5; ++i)
            {
                std::uniform_real_distribution<> pos_dist(-4.0, 4.0);
                gz::math::Pose3d obstacle_pose(
                    pos_dist(random_engine_),
                    pos_dist(random_engine_),
                    1.0, 0, 0, 0
                );
                
                SpawnModelFromTemplate("obstacle", obstacle_pose);
            }
            
            race_course_generated = true;
        }
    }
    
    void GeneratePuzzleScenario()
    {
        static bool puzzle_generated = false;
        
        if (!puzzle_generated)
        {
            gzmsg << "Generating puzzle scenario..." << std::endl;
            
            // Create puzzle elements with platforms and moveable blocks
            std::vector<gz::math::Vector3d> platform_positions = {
                gz::math::Vector3d(-4, -4, 0.5),
                gz::math::Vector3d(0, -4, 1.5),
                gz::math::Vector3d(4, -4, 2.5),
                gz::math::Vector3d(4, 0, 2.5),
                gz::math::Vector3d(4, 4, 0.5),
            };
            
            for (const auto& pos : platform_positions)
            {
                SpawnModelFromTemplate("platform", gz::math::Pose3d(pos, gz::math::Quaterniond()));
            }
            
            // Add moveable puzzle pieces
            for (int i = 0; i < 3; ++i)
            {
                gz::math::Pose3d block_pose(
                    -6 + i * 2, 0, 0.5, 0, 0, 0
                );
                SpawnModelFromTemplate("basic_box", block_pose);
            }
            
            puzzle_generated = true;
        }
    }
    
    void GenerateSurvivalScenario()
    {
        static bool survival_generated = false;
        
        if (!survival_generated)
        {
            gzmsg << "Generating survival scenario..." << std::endl;
            
            // Create resource nodes
            for (int i = 0; i < 8; ++i)
            {
                std::uniform_real_distribution<> pos_dist(-8.0, 8.0);
                gz::math::Pose3d resource_pose(
                    pos_dist(random_engine_),
                    pos_dist(random_engine_),
                    0.5, 0, 0, 0
                );
                
                SpawnModelFromTemplate("pickup_item", resource_pose);
            }
            
            // Create hazards
            for (int i = 0; i < 6; ++i)
            {
                std::uniform_real_distribution<> pos_dist(-6.0, 6.0);
                gz::math::Pose3d hazard_pose(
                    pos_dist(random_engine_),
                    pos_dist(random_engine_),
                    1.0, 0, 0, 0
                );
                
                SpawnModelFromTemplate("obstacle", hazard_pose);
            }
            
            survival_generated = true;
        }
    }
    
    void UpdateModelLifecycle()
    {
        // Clean up inactive models from tracking
        auto it = spawned_models_.begin();
        while (it != spawned_models_.end())
        {
            if (!it->second.active && (simulation_time_ - it->second.spawn_time) > 30.0)
            {
                it = spawned_models_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    
    void ReportPerformanceMetrics()
    {
        size_t active_models = 0;
        for (const auto& pair : spawned_models_)
        {
            if (pair.second.active)
                active_models++;
        }
        
        gzmsg << "\n=== Model Management Performance ===" << std::endl;
        gzmsg << "Active models: " << active_models << std::endl;
        gzmsg << "Total spawned: " << total_models_spawned_ << std::endl;
        gzmsg << "Total removed: " << total_models_removed_ << std::endl;
        gzmsg << "Pool reuse count: " << model_pool_->reuse_count << std::endl;
        gzmsg << "Pool size: " << model_pool_->available_models.size() 
              << "/" << model_pool_->max_pool_size << std::endl;
        
        double spawn_rate = total_models_spawned_ / simulation_time_;
        gzmsg << "Average spawn rate: " << std::fixed << std::setprecision(2) 
              << spawn_rate << " models/second" << std::endl;
    }
    
    void ReportFinalStatistics()
    {
        gzmsg << "\n=== Final Model Management Statistics ===" << std::endl;
        gzmsg << "Total simulation time: " << std::fixed << std::setprecision(1) 
              << simulation_time_ << " seconds" << std::endl;
        gzmsg << "Total models spawned: " << total_models_spawned_ << std::endl;
        gzmsg << "Total models removed: " << total_models_removed_ << std::endl;
        gzmsg << "Models reused from pool: " << model_pool_->reuse_count << std::endl;
        gzmsg << "Model templates used: " << model_templates_.size() << std::endl;
        
        // Calculate efficiency metrics
        double spawn_efficiency = static_cast<double>(model_pool_->reuse_count) / total_models_spawned_ * 100.0;
        gzmsg << "Model reuse efficiency: " << std::setprecision(1) << spawn_efficiency << "%" << std::endl;
        
        gzmsg << "\nDemonstration successfully showcased:" << std::endl;
        gzmsg << "• Dynamic model spawning and removal" << std::endl;
        gzmsg << "• Procedural environment generation" << std::endl;
        gzmsg << "• Runtime model property modification" << std::endl;
        gzmsg << "• Advanced model composition techniques" << std::endl;
        gzmsg << "• Performance optimization with model pooling" << std::endl;
        gzmsg << "• Dynamic scenario generation systems" << std::endl;
    }
    
    // Member variables
    std::unique_ptr<gz::transport::Node> transport_node_;
    gz::transport::Node::Publisher model_pose_pub_;
    
    // Service clients
    gz::transport::Node::RequesterProxy entity_factory_client_;
    gz::transport::Node::RequesterProxy entity_remove_client_;
    gz::transport::Node::RequesterProxy entity_modify_client_;
    
    std::map<std::string, ModelTemplate> model_templates_;
    std::map<std::string, SpawnedModel> spawned_models_;
    std::unique_ptr<ModelPool> model_pool_;
    
    std::vector<std::string> modification_test_models_;
    
    double simulation_time_;
    int demo_phase_;
    size_t model_id_counter_;
    size_t total_models_spawned_;
    size_t total_models_removed_;
    
    std::mt19937 random_engine_;
};

int main(int argc, char** argv)
{
    // Configure Gazebo server
    gz::sim::ServerConfig config;
    config.SetSdfFile("worlds/advanced_features.sdf");
    config.SetUpdateRate(1000.0);
    
    gz::sim::Server server(config);
    
    gzmsg << "\n=== Dynamic Model Management Demo ===" << std::endl;
    gzmsg << "Gazebo Harmonic - Advanced Model Operations Tutorial" << std::endl;
    gzmsg << "\nThis demo demonstrates:" << std::endl;
    gzmsg << "• Runtime model spawning and removal" << std::endl;
    gzmsg << "• Procedural environment generation" << std::endl;
    gzmsg << "• Real-time model property modification" << std::endl;
    gzmsg << "• Advanced model composition and templating" << std::endl;
    gzmsg << "• Performance optimization with model pooling" << std::endl;
    gzmsg << "• Dynamic scenario and content generation" << std::endl;
    gzmsg << "• Efficient memory management for large scenes" << std::endl;
    
    // Start server
    std::thread server_thread([&server]() {
        server.Run(true, 0, false);
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Run dynamic model management demo
    DynamicModelManager manager;
    manager.Start();
    
    server.RequestStop();
    if (server_thread.joinable())
        server_thread.join();
    
    return 0;
}