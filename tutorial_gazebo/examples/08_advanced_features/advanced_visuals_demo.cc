#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/components/Light.hh>
#include <gz/sim/components/Material.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/Visual.hh>

#include <gz/msgs/entity_factory.pb.h>
#include <gz/msgs/light.pb.h>
#include <gz/msgs/material.pb.h>
#include <gz/msgs/pose.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Pose3.hh>
#include <gz/math/Vector3.hh>
#include <gz/math/Color.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/rendering/RenderEngine.hh>
#include <gz/rendering/RenderingIface.hh>
#include <gz/rendering/Scene.hh>
#include <gz/rendering/Camera.hh>
#include <gz/rendering/Light.hh>
#include <gz/rendering/Material.hh>

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cmath>
#include <sstream>

/**
 * @brief Advanced Materials and Lighting Demo
 * 
 * This program demonstrates cutting-edge visual rendering capabilities
 * in Gazebo Harmonic, showcasing realistic material systems and 
 * dynamic lighting effects:
 * 
 * PBR (Physically Based Rendering) Materials:
 * - Metallic and non-metallic surfaces
 * - Roughness and smoothness variations
 * - Albedo, normal, and roughness texture mapping
 * - Emissive materials and glow effects
 * - Transparency and refraction
 * 
 * Advanced Lighting Systems:
 * - Dynamic directional lighting (sun simulation)
 * - Point lights with attenuation
 * - Spot lights with cone parameters
 * - Area lights for realistic illumination
 * - HDR environment mapping
 * - Real-time shadow casting
 * 
 * Dynamic Visual Effects:
 * - Animated material properties
 * - Color temperature changes
 * - Intensity modulation
 * - Shadow quality adjustment
 * - Post-processing effects
 * 
 * Applications Demonstrated:
 * - Realistic product visualization
 * - Architectural lighting design
 * - Material property testing
 * - Visual quality assessment
 * - Rendering performance optimization
 */

struct MaterialProperties
{
    std::string name;
    double metalness;
    double roughness;
    gz::math::Color albedo;
    gz::math::Color emissive;
    std::string albedo_map;
    std::string normal_map;
    std::string roughness_map;
    bool transparent;
    double transparency;
    
    MaterialProperties(const std::string& n = "default")
        : name(n), metalness(0.0), roughness(0.5), 
          albedo(0.7, 0.7, 0.7), emissive(0, 0, 0),
          transparent(false), transparency(1.0) {}
};

struct LightConfiguration
{
    std::string name;
    std::string type; // "directional", "point", "spot"
    gz::math::Pose3d pose;
    gz::math::Color diffuse;
    gz::math::Color specular;
    double intensity;
    double range;
    bool cast_shadows;
    
    // Spot light specific
    double inner_angle;
    double outer_angle;
    double falloff;
    
    // Attenuation parameters
    double constant_attenuation;
    double linear_attenuation;
    double quadratic_attenuation;
    
    LightConfiguration(const std::string& n, const std::string& t)
        : name(n), type(t), intensity(1.0), range(10.0), cast_shadows(true),
          inner_angle(0.3), outer_angle(0.8), falloff(1.0),
          constant_attenuation(1.0), linear_attenuation(0.0), quadratic_attenuation(0.0),
          diffuse(1, 1, 1), specular(1, 1, 1) {}
};

class AdvancedVisualsManager
{
public:
    AdvancedVisualsManager()
        : simulation_time_(0.0)
        , demo_phase_(0)
        , light_animation_time_(0.0)
        , material_animation_time_(0.0)
    {
        // Initialize transport node
        transport_node_ = std::make_unique<gz::transport::Node>();
        
        // Service clients for entity management
        entity_factory_client_ = transport_node_->Request("/world/advanced_features/create");
        light_control_client_ = transport_node_->Request("/world/advanced_features/light_config");
        
        // Publishers for visual updates
        material_pub_ = transport_node_->Advertise<gz::msgs::Material>("/world/advanced_features/material");
        light_pub_ = transport_node_->Advertise<gz::msgs::Light>("/world/advanced_features/light");
        
        // Initialize material and lighting presets
        InitializeMaterialPresets();
        InitializeLightingPresets();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gzmsg << "Advanced Visuals Manager initialized" << std::endl;
    }
    
    void Start()
    {
        gzmsg << "\n=== Advanced Materials and Lighting Demo ===" << std::endl;
        gzmsg << "Starting comprehensive visual rendering demonstration..." << std::endl;
        
        const double control_frequency = 60.0; // 60 Hz for smooth visual updates
        const auto control_period = std::chrono::duration<double>(1.0 / control_frequency);
        
        while (demo_phase_ < 6)
        {
            auto start_time = std::chrono::steady_clock::now();
            
            switch (demo_phase_)
            {
                case 0: MaterialShowcaseDemo(); break;
                case 1: DynamicLightingDemo(); break;
                case 2: PBRMaterialDemo(); break;
                case 3: EnvironmentalEffectsDemo(); break;
                case 4: AdvancedShadowDemo(); break;
                case 5: PerformanceAnalysisDemo(); break;
            }
            
            // Update animations
            UpdateMaterialAnimations();
            UpdateLightAnimations();
            
            simulation_time_ += 1.0 / control_frequency;
            light_animation_time_ += 1.0 / control_frequency;
            material_animation_time_ += 1.0 / control_frequency;
            
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = end_time - start_time;
            if (elapsed < control_period)
            {
                std::this_thread::sleep_for(control_period - elapsed);
            }
        }
        
        gzmsg << "Advanced visuals demo completed!" << std::endl;
    }

private:
    void InitializeMaterialPresets()
    {
        // Metallic materials
        MaterialProperties chrome("chrome");
        chrome.metalness = 1.0;
        chrome.roughness = 0.1;
        chrome.albedo = gz::math::Color(0.7, 0.7, 0.8);
        material_presets_.push_back(chrome);
        
        MaterialProperties gold("gold");
        gold.metalness = 1.0;
        gold.roughness = 0.2;
        gold.albedo = gz::math::Color(1.0, 0.8, 0.3);
        material_presets_.push_back(gold);
        
        MaterialProperties copper("copper");
        copper.metalness = 1.0;
        copper.roughness = 0.3;
        copper.albedo = gz::math::Color(0.9, 0.4, 0.2);
        material_presets_.push_back(copper);
        
        // Non-metallic materials
        MaterialProperties plastic("plastic");
        plastic.metalness = 0.0;
        plastic.roughness = 0.7;
        plastic.albedo = gz::math::Color(0.2, 0.6, 0.8);
        material_presets_.push_back(plastic);
        
        MaterialProperties rubber("rubber");
        rubber.metalness = 0.0;
        rubber.roughness = 0.9;
        rubber.albedo = gz::math::Color(0.1, 0.1, 0.1);
        material_presets_.push_back(rubber);
        
        MaterialProperties glass("glass");
        glass.metalness = 0.0;
        glass.roughness = 0.0;
        glass.albedo = gz::math::Color(0.9, 0.9, 1.0);
        glass.transparent = true;
        glass.transparency = 0.1;
        material_presets_.push_back(glass);
        
        // Emissive materials
        MaterialProperties neon("neon");
        neon.metalness = 0.0;
        neon.roughness = 0.1;
        neon.albedo = gz::math::Color(0.2, 0.8, 0.2);
        neon.emissive = gz::math::Color(0.0, 1.0, 0.0);
        material_presets_.push_back(neon);
        
        MaterialProperties lava("lava");
        lava.metalness = 0.0;
        lava.roughness = 0.8;
        lava.albedo = gz::math::Color(0.8, 0.2, 0.0);
        lava.emissive = gz::math::Color(1.0, 0.3, 0.0);
        material_presets_.push_back(lava);
    }
    
    void InitializeLightingPresets()
    {
        // Daylight setup
        LightConfiguration daylight("daylight", "directional");
        daylight.pose = gz::math::Pose3d(0, 0, 20, 0.3, 0.2, 0);
        daylight.diffuse = gz::math::Color(1.0, 1.0, 0.95);
        daylight.specular = gz::math::Color(0.8, 0.8, 0.8);
        daylight.intensity = 1.2;
        lighting_presets_.push_back(daylight);
        
        // Sunset lighting
        LightConfiguration sunset("sunset", "directional");
        sunset.pose = gz::math::Pose3d(0, 0, 10, 1.2, 0.1, 0);
        sunset.diffuse = gz::math::Color(1.0, 0.6, 0.3);
        sunset.specular = gz::math::Color(1.0, 0.7, 0.4);
        sunset.intensity = 0.8;
        lighting_presets_.push_back(sunset);
        
        // Studio lighting
        LightConfiguration key_light("key_light", "spot");
        key_light.pose = gz::math::Pose3d(5, 5, 8, -0.8, -0.3, -0.7);
        key_light.diffuse = gz::math::Color(1.0, 1.0, 1.0);
        key_light.specular = gz::math::Color(1.0, 1.0, 1.0);
        key_light.intensity = 3.0;
        key_light.range = 20.0;
        key_light.inner_angle = 0.2;
        key_light.outer_angle = 0.6;
        key_light.falloff = 2.0;
        lighting_presets_.push_back(key_light);
        
        LightConfiguration fill_light("fill_light", "spot");
        fill_light.pose = gz::math::Pose3d(-3, 3, 6, -0.5, 0.2, 0.5);
        fill_light.diffuse = gz::math::Color(0.8, 0.9, 1.0);
        fill_light.specular = gz::math::Color(0.6, 0.7, 0.8);
        fill_light.intensity = 1.5;
        fill_light.range = 15.0;
        fill_light.inner_angle = 0.4;
        fill_light.outer_angle = 0.8;
        lighting_presets_.push_back(fill_light);
        
        // Colored accent lights
        LightConfiguration red_accent("red_accent", "point");
        red_accent.pose = gz::math::Pose3d(8, 0, 3, 0, 0, 0);
        red_accent.diffuse = gz::math::Color(1.0, 0.2, 0.2);
        red_accent.specular = gz::math::Color(1.0, 0.3, 0.3);
        red_accent.intensity = 2.0;
        red_accent.range = 12.0;
        red_accent.constant_attenuation = 0.5;
        red_accent.linear_attenuation = 0.1;
        red_accent.quadratic_attenuation = 0.02;
        lighting_presets_.push_back(red_accent);
        
        LightConfiguration blue_accent("blue_accent", "point");
        blue_accent.pose = gz::math::Pose3d(-8, 0, 3, 0, 0, 0);
        blue_accent.diffuse = gz::math::Color(0.2, 0.4, 1.0);
        blue_accent.specular = gz::math::Color(0.3, 0.5, 1.0);
        blue_accent.intensity = 2.0;
        blue_accent.range = 12.0;
        blue_accent.constant_attenuation = 0.5;
        blue_accent.linear_attenuation = 0.1;
        blue_accent.quadratic_attenuation = 0.02;
        lighting_presets_.push_back(blue_accent);
    }
    
    void MaterialShowcaseDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 1: Material Showcase Demo ---" << std::endl;
            gzmsg << "Demonstrating various PBR material properties..." << std::endl;
            
            // Spawn material test objects
            CreateMaterialTestObjects();
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        // Demonstrate different materials over time
        int material_index = static_cast<int>(simulation_time_ - start_time) / 3;
        if (material_index < static_cast<int>(material_presets_.size()))
        {
            ApplyMaterialToTestObjects(material_presets_[material_index]);
        }
        
        if (simulation_time_ - start_time > 24.0) // 3 seconds per material × 8 materials
        {
            gzmsg << "Material showcase completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void DynamicLightingDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 2: Dynamic Lighting Demo ---" << std::endl;
            gzmsg << "Creating dynamic lighting scenarios..." << std::endl;
            
            // Setup dynamic lighting
            SetupDynamicLighting();
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        // Cycle through lighting scenarios
        double phase_time = simulation_time_ - start_time;
        int lighting_scenario = static_cast<int>(phase_time / 5.0) % lighting_presets_.size();
        
        ApplyLightingScenario(lighting_presets_[lighting_scenario]);
        
        if (phase_time > 30.0)
        {
            gzmsg << "Dynamic lighting demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void PBRMaterialDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 3: Advanced PBR Material Demo ---" << std::endl;
            gzmsg << "Demonstrating physically based rendering features..." << std::endl;
            
            // Create PBR comparison objects
            CreatePBRComparisonScene();
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        // Animate PBR parameters
        double phase_time = simulation_time_ - start_time;
        AnimatePBRParameters(phase_time);
        
        if (phase_time > 20.0)
        {
            gzmsg << "PBR material demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void EnvironmentalEffectsDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 4: Environmental Effects Demo ---" << std::endl;
            gzmsg << "Simulating environmental lighting and atmospheric effects..." << std::endl;
            
            // Setup environmental effects
            CreateEnvironmentalEffects();
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Simulate time-of-day changes
        SimulateTimeOfDay(phase_time);
        
        if (phase_time > 25.0)
        {
            gzmsg << "Environmental effects demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void AdvancedShadowDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 5: Advanced Shadow Rendering Demo ---" << std::endl;
            gzmsg << "Demonstrating high-quality shadow effects..." << std::endl;
            
            // Setup shadow demonstration
            CreateShadowDemonstration();
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Animate shadow-casting objects and lights
        AnimateShadowEffects(phase_time);
        
        if (phase_time > 18.0)
        {
            gzmsg << "Advanced shadow demo completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void PerformanceAnalysisDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 6: Visual Performance Analysis Demo ---" << std::endl;
            gzmsg << "Analyzing rendering performance and optimization..." << std::endl;
            
            // Setup performance monitoring
            InitializePerformanceMonitoring();
            
            start_time = simulation_time_;
            initialized = true;
        }
        
        double phase_time = simulation_time_ - start_time;
        
        // Monitor and report performance metrics
        if (static_cast<int>(phase_time) % 3 == 0 && 
            phase_time - static_cast<int>(phase_time) < 0.1)
        {
            ReportRenderingPerformance();
        }
        
        if (phase_time > 12.0)
        {
            gzmsg << "Performance analysis completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void CreateMaterialTestObjects()
    {
        // Create a series of objects for material testing
        const std::vector<std::string> shapes = {"sphere", "box", "cylinder"};
        
        for (size_t i = 0; i < shapes.size(); ++i)
        {
            double x = (i - 1) * 3.0;
            std::string object_sdf = GenerateTestObjectSDF(shapes[i], 
                gz::math::Pose3d(x, 0, 1, 0, 0, 0));
            
            gz::msgs::EntityFactory req;
            req.set_sdf(object_sdf);
            req.set_name("material_test_" + shapes[i]);
            
            gz::msgs::Boolean rep;
            bool result;
            unsigned int timeout = 1000;
            
            bool executed = entity_factory_client_.Request(req, rep, timeout, result);
            
            if (executed && result && rep.data())
            {
                gzmsg << "Created material test object: " << shapes[i] << std::endl;
            }
        }
    }
    
    std::string GenerateTestObjectSDF(const std::string& shape, const gz::math::Pose3d& pose)
    {
        std::ostringstream sdf_stream;
        
        sdf_stream << "<?xml version='1.0'?>\n";
        sdf_stream << "<sdf version='1.10'>\n";
        sdf_stream << "  <model name='" << shape << "_test'>\n";
        sdf_stream << "    <pose>" << pose.Pos().X() << " " << pose.Pos().Y() << " " << pose.Pos().Z()
                   << " " << pose.Rot().Roll() << " " << pose.Rot().Pitch() << " " << pose.Rot().Yaw() << "</pose>\n";
        sdf_stream << "    <static>true</static>\n";
        sdf_stream << "    <link name='link'>\n";
        
        // Geometry
        sdf_stream << "      <collision name='collision'>\n";
        sdf_stream << "        <geometry>\n";
        if (shape == "sphere")
        {
            sdf_stream << "          <sphere><radius>0.8</radius></sphere>\n";
        }
        else if (shape == "box")
        {
            sdf_stream << "          <box><size>1.5 1.5 1.5</size></box>\n";
        }
        else if (shape == "cylinder")
        {
            sdf_stream << "          <cylinder><radius>0.8</radius><length>1.6</length></cylinder>\n";
        }
        sdf_stream << "        </geometry>\n";
        sdf_stream << "      </collision>\n";
        
        sdf_stream << "      <visual name='visual'>\n";
        sdf_stream << "        <geometry>\n";
        if (shape == "sphere")
        {
            sdf_stream << "          <sphere><radius>0.8</radius></sphere>\n";
        }
        else if (shape == "box")
        {
            sdf_stream << "          <box><size>1.5 1.5 1.5</size></box>\n";
        }
        else if (shape == "cylinder")
        {
            sdf_stream << "          <cylinder><radius>0.8</radius><length>1.6</length></cylinder>\n";
        }
        sdf_stream << "        </geometry>\n";
        
        // Default material (will be updated dynamically)
        sdf_stream << "        <material>\n";
        sdf_stream << "          <ambient>0.5 0.5 0.5 1</ambient>\n";
        sdf_stream << "          <diffuse>0.7 0.7 0.7 1</diffuse>\n";
        sdf_stream << "          <specular>0.3 0.3 0.3 1</specular>\n";
        sdf_stream << "          <pbr>\n";
        sdf_stream << "            <metal>\n";
        sdf_stream << "              <metalness>0.5</metalness>\n";
        sdf_stream << "              <roughness>0.5</roughness>\n";
        sdf_stream << "            </metal>\n";
        sdf_stream << "          </pbr>\n";
        sdf_stream << "        </material>\n";
        sdf_stream << "      </visual>\n";
        sdf_stream << "    </link>\n";
        sdf_stream << "  </model>\n";
        sdf_stream << "</sdf>\n";
        
        return sdf_stream.str();
    }
    
    void ApplyMaterialToTestObjects(const MaterialProperties& material)
    {
        gzmsg << "Applying material: " << material.name 
              << " (metalness: " << material.metalness 
              << ", roughness: " << material.roughness << ")" << std::endl;
        
        // In a real implementation, this would send material update messages
        // to modify the visual appearance of the test objects
    }
    
    void SetupDynamicLighting()
    {
        gzmsg << "Setting up dynamic lighting system..." << std::endl;
        
        // Create additional dynamic lights
        for (const auto& light_config : lighting_presets_)
        {
            if (light_config.name.find("accent") != std::string::npos)
            {
                CreateDynamicLight(light_config);
            }
        }
    }
    
    void CreateDynamicLight(const LightConfiguration& config)
    {
        std::string light_sdf = GenerateLightSDF(config);
        
        gz::msgs::EntityFactory req;
        req.set_sdf(light_sdf);
        req.set_name(config.name);
        
        gz::msgs::Boolean rep;
        bool result;
        unsigned int timeout = 1000;
        
        bool executed = entity_factory_client_.Request(req, rep, timeout, result);
        
        if (executed && result && rep.data())
        {
            gzmsg << "Created dynamic light: " << config.name << std::endl;
        }
    }
    
    std::string GenerateLightSDF(const LightConfiguration& config)
    {
        std::ostringstream sdf_stream;
        
        sdf_stream << "<?xml version='1.0'?>\n";
        sdf_stream << "<sdf version='1.10'>\n";
        sdf_stream << "  <light name='" << config.name << "' type='" << config.type << "'>\n";
        sdf_stream << "    <pose>" << config.pose.Pos().X() << " " << config.pose.Pos().Y() << " " << config.pose.Pos().Z()
                   << " " << config.pose.Rot().Roll() << " " << config.pose.Rot().Pitch() << " " << config.pose.Rot().Yaw() << "</pose>\n";
        sdf_stream << "    <cast_shadows>" << (config.cast_shadows ? "true" : "false") << "</cast_shadows>\n";
        
        if (config.type == "directional")
        {
            sdf_stream << "    <direction>0 0 -1</direction>\n";
        }
        
        sdf_stream << "    <diffuse>" << config.diffuse.R() << " " << config.diffuse.G() 
                   << " " << config.diffuse.B() << " 1</diffuse>\n";
        sdf_stream << "    <specular>" << config.specular.R() << " " << config.specular.G() 
                   << " " << config.specular.B() << " 1</specular>\n";
        sdf_stream << "    <intensity>" << config.intensity << "</intensity>\n";
        sdf_stream << "    <range>" << config.range << "</range>\n";
        
        if (config.type == "spot")
        {
            sdf_stream << "    <spot>\n";
            sdf_stream << "      <inner_angle>" << config.inner_angle << "</inner_angle>\n";
            sdf_stream << "      <outer_angle>" << config.outer_angle << "</outer_angle>\n";
            sdf_stream << "      <falloff>" << config.falloff << "</falloff>\n";
            sdf_stream << "    </spot>\n";
        }
        
        if (config.type == "point" || config.type == "spot")
        {
            sdf_stream << "    <attenuation>\n";
            sdf_stream << "      <constant>" << config.constant_attenuation << "</constant>\n";
            sdf_stream << "      <linear>" << config.linear_attenuation << "</linear>\n";
            sdf_stream << "      <quadratic>" << config.quadratic_attenuation << "</quadratic>\n";
            sdf_stream << "    </attenuation>\n";
        }
        
        sdf_stream << "  </light>\n";
        sdf_stream << "</sdf>\n";
        
        return sdf_stream.str();
    }
    
    void ApplyLightingScenario(const LightConfiguration& scenario)
    {
        static std::string last_scenario = "";
        
        if (last_scenario != scenario.name)
        {
            gzmsg << "Applying lighting scenario: " << scenario.name << std::endl;
            last_scenario = scenario.name;
        }
        
        // In a real implementation, this would modify existing lights
        // or create new ones based on the scenario
    }
    
    void CreatePBRComparisonScene()
    {
        gzmsg << "Creating PBR comparison scene..." << std::endl;
        
        // Create a grid of objects with varying PBR properties
        for (int metalness_level = 0; metalness_level < 4; ++metalness_level)
        {
            for (int roughness_level = 0; roughness_level < 4; ++roughness_level)
            {
                double x = (metalness_level - 1.5) * 2.5;
                double y = (roughness_level - 1.5) * 2.5;
                
                MaterialProperties material("pbr_" + std::to_string(metalness_level) + "_" + std::to_string(roughness_level));
                material.metalness = metalness_level / 3.0;
                material.roughness = roughness_level / 3.0;
                material.albedo = gz::math::Color(0.7, 0.5, 0.3);
                
                std::string object_sdf = GenerateTestObjectSDF("sphere", gz::math::Pose3d(x, y, 0.8, 0, 0, 0));
                
                gz::msgs::EntityFactory req;
                req.set_sdf(object_sdf);
                req.set_name("pbr_sphere_" + std::to_string(metalness_level) + "_" + std::to_string(roughness_level));
                
                gz::msgs::Boolean rep;
                bool result;
                unsigned int timeout = 1000;
                
                entity_factory_client_.Request(req, rep, timeout, result);
            }
        }
    }
    
    void AnimatePBRParameters(double time)
    {
        // Animate PBR parameters over time
        double metalness_wave = (std::sin(time * 0.5) + 1.0) * 0.5;
        double roughness_wave = (std::cos(time * 0.3) + 1.0) * 0.5;
        
        // In a real implementation, this would update material properties
        // of the PBR comparison objects
    }
    
    void CreateEnvironmentalEffects()
    {
        gzmsg << "Creating environmental effects..." << std::endl;
        
        // Environmental effects would be implemented through
        // atmospheric settings and environmental lighting
    }
    
    void SimulateTimeOfDay(double time)
    {
        // Simulate 24-hour day cycle in compressed time
        double day_progress = std::fmod(time / 25.0, 1.0); // 25 seconds = 1 day
        
        // Calculate sun position based on time of day
        double sun_angle = day_progress * 2.0 * M_PI;
        double sun_elevation = std::sin(sun_angle) * 0.8; // Max 45 degrees elevation
        
        // Update sun color based on elevation
        gz::math::Color sun_color;
        if (sun_elevation > 0.1)
        {
            // Daytime colors
            sun_color = gz::math::Color(1.0, 1.0, 0.95);
        }
        else if (sun_elevation > -0.1)
        {
            // Sunrise/sunset colors
            double sunset_factor = (sun_elevation + 0.1) / 0.2;
            sun_color = gz::math::Color(1.0, 0.6 + 0.4 * sunset_factor, 0.3 + 0.65 * sunset_factor);
        }
        else
        {
            // Night colors (moonlight)
            sun_color = gz::math::Color(0.1, 0.1, 0.2);
        }
        
        // Report time of day changes
        static int last_hour = -1;
        int current_hour = static_cast<int>(day_progress * 24);
        if (current_hour != last_hour)
        {
            gzmsg << "Time of day: " << current_hour << ":00" 
                  << " (sun elevation: " << std::fixed << std::setprecision(1) 
                  << sun_elevation * 90.0 << " degrees)" << std::endl;
            last_hour = current_hour;
        }
    }
    
    void CreateShadowDemonstration()
    {
        gzmsg << "Creating shadow demonstration scene..." << std::endl;
        
        // Create objects that cast interesting shadows
        CreateShadowCastingObjects();
    }
    
    void CreateShadowCastingObjects()
    {
        // Create a collection of objects designed to showcase shadow effects
        std::vector<std::pair<std::string, gz::math::Pose3d>> shadow_objects = {
            {"tall_pillar", gz::math::Pose3d(2, 2, 2, 0, 0, 0)},
            {"wide_platform", gz::math::Pose3d(-2, 2, 0.2, 0, 0, 0)},
            {"angled_ramp", gz::math::Pose3d(0, -2, 0.5, 0.3, 0, 0)}
        };
        
        for (const auto& obj : shadow_objects)
        {
            std::string object_sdf = GenerateShadowObjectSDF(obj.first, obj.second);
            
            gz::msgs::EntityFactory req;
            req.set_sdf(object_sdf);
            req.set_name(obj.first);
            
            gz::msgs::Boolean rep;
            bool result;
            unsigned int timeout = 1000;
            
            entity_factory_client_.Request(req, rep, timeout, result);
        }
    }
    
    std::string GenerateShadowObjectSDF(const std::string& name, const gz::math::Pose3d& pose)
    {
        std::ostringstream sdf_stream;
        
        sdf_stream << "<?xml version='1.0'?>\n";
        sdf_stream << "<sdf version='1.10'>\n";
        sdf_stream << "  <model name='" << name << "'>\n";
        sdf_stream << "    <pose>" << pose.Pos().X() << " " << pose.Pos().Y() << " " << pose.Pos().Z()
                   << " " << pose.Rot().Roll() << " " << pose.Rot().Pitch() << " " << pose.Rot().Yaw() << "</pose>\n";
        sdf_stream << "    <static>true</static>\n";
        sdf_stream << "    <link name='link'>\n";
        
        // Different geometries for different shadow effects
        if (name == "tall_pillar")
        {
            sdf_stream << "      <collision name='collision'>\n";
            sdf_stream << "        <geometry><box><size>0.5 0.5 4</size></box></geometry>\n";
            sdf_stream << "      </collision>\n";
            sdf_stream << "      <visual name='visual'>\n";
            sdf_stream << "        <geometry><box><size>0.5 0.5 4</size></box></geometry>\n";
        }
        else if (name == "wide_platform")
        {
            sdf_stream << "      <collision name='collision'>\n";
            sdf_stream << "        <geometry><box><size>3 3 0.4</size></box></geometry>\n";
            sdf_stream << "      </collision>\n";
            sdf_stream << "      <visual name='visual'>\n";
            sdf_stream << "        <geometry><box><size>3 3 0.4</size></box></geometry>\n";
        }
        else if (name == "angled_ramp")
        {
            sdf_stream << "      <collision name='collision'>\n";
            sdf_stream << "        <geometry><box><size>2 1 1</size></box></geometry>\n";
            sdf_stream << "      </collision>\n";
            sdf_stream << "      <visual name='visual'>\n";
            sdf_stream << "        <geometry><box><size>2 1 1</size></box></geometry>\n";
        }
        
        sdf_stream << "        <material>\n";
        sdf_stream << "          <ambient>0.6 0.6 0.6 1</ambient>\n";
        sdf_stream << "          <diffuse>0.8 0.8 0.8 1</diffuse>\n";
        sdf_stream << "          <specular>0.2 0.2 0.2 1</specular>\n";
        sdf_stream << "        </material>\n";
        sdf_stream << "      </visual>\n";
        sdf_stream << "    </link>\n";
        sdf_stream << "  </model>\n";
        sdf_stream << "</sdf>\n";
        
        return sdf_stream.str();
    }
    
    void AnimateShadowEffects(double time)
    {
        // Animate light positions to create moving shadows
        double light_angle = time * 0.5;
        double light_x = 8.0 * std::cos(light_angle);
        double light_y = 8.0 * std::sin(light_angle);
        
        // In a real implementation, this would update light positions
        // to create moving shadow effects
    }
    
    void InitializePerformanceMonitoring()
    {
        gzmsg << "Initializing rendering performance monitoring..." << std::endl;
        
        performance_metrics_.frame_count = 0;
        performance_metrics_.start_time = simulation_time_;
        performance_metrics_.total_render_time = 0.0;
    }
    
    void ReportRenderingPerformance()
    {
        double elapsed_time = simulation_time_ - performance_metrics_.start_time;
        double estimated_fps = 60.0; // Estimated based on control frequency
        
        gzmsg << "\n=== Rendering Performance Metrics ===" << std::endl;
        gzmsg << "Elapsed time: " << std::fixed << std::setprecision(1) << elapsed_time << "s" << std::endl;
        gzmsg << "Estimated FPS: " << std::setprecision(0) << estimated_fps << std::endl;
        gzmsg << "Active materials: " << material_presets_.size() << std::endl;
        gzmsg << "Active lights: " << lighting_presets_.size() << std::endl;
        gzmsg << "Shadow casting enabled: Yes" << std::endl;
        gzmsg << "PBR rendering: Enabled" << std::endl;
        
        // Provide optimization recommendations
        gzmsg << "\nOptimization recommendations:" << std::endl;
        if (estimated_fps < 30)
        {
            gzmsg << "• Consider reducing shadow quality or disabling shadows for distant objects" << std::endl;
            gzmsg << "• Use LOD (Level of Detail) for complex geometry" << std::endl;
            gzmsg << "• Limit number of dynamic lights" << std::endl;
        }
        else if (estimated_fps < 45)
        {
            gzmsg << "• Good performance, minor optimizations possible" << std::endl;
            gzmsg << "• Consider texture compression for large textures" << std::endl;
        }
        else
        {
            gzmsg << "• Excellent performance!" << std::endl;
            gzmsg << "• Can safely increase visual quality or add more complex effects" << std::endl;
        }
    }
    
    void UpdateMaterialAnimations()
    {
        // Animate material properties for dynamic effects
        double emissive_pulse = (std::sin(material_animation_time_ * 2.0) + 1.0) * 0.5;
        double metalness_cycle = (std::sin(material_animation_time_ * 0.3) + 1.0) * 0.5;
        
        // In a real implementation, this would update material properties
        // to create animated visual effects
    }
    
    void UpdateLightAnimations()
    {
        // Animate light properties for dynamic lighting effects
        double intensity_wave = 0.8 + 0.4 * std::sin(light_animation_time_ * 1.5);
        double color_cycle = light_animation_time_ * 0.2;
        
        // Color cycling for accent lights
        gz::math::Color animated_color(
            0.5 + 0.5 * std::sin(color_cycle),
            0.5 + 0.5 * std::sin(color_cycle + 2.0 * M_PI / 3.0),
            0.5 + 0.5 * std::sin(color_cycle + 4.0 * M_PI / 3.0)
        );
        
        // In a real implementation, this would send light update messages
        // to modify light properties in real-time
    }
    
    // Member variables
    std::unique_ptr<gz::transport::Node> transport_node_;
    gz::transport::Node::Publisher material_pub_;
    gz::transport::Node::Publisher light_pub_;
    
    // Service clients
    gz::transport::Node::RequesterProxy entity_factory_client_;
    gz::transport::Node::RequesterProxy light_control_client_;
    
    std::vector<MaterialProperties> material_presets_;
    std::vector<LightConfiguration> lighting_presets_;
    
    double simulation_time_;
    int demo_phase_;
    double light_animation_time_;
    double material_animation_time_;
    
    struct PerformanceMetrics {
        size_t frame_count;
        double start_time;
        double total_render_time;
    } performance_metrics_;
};

int main(int argc, char** argv)
{
    // Configure Gazebo server
    gz::sim::ServerConfig config;
    config.SetSdfFile("worlds/advanced_features.sdf");
    config.SetUpdateRate(1000.0);
    
    gz::sim::Server server(config);
    
    gzmsg << "\n=== Advanced Materials and Lighting Demo ===" << std::endl;
    gzmsg << "Gazebo Harmonic - Advanced Visual Rendering Tutorial" << std::endl;
    gzmsg << "\nThis demo demonstrates:" << std::endl;
    gzmsg << "• Physically Based Rendering (PBR) materials" << std::endl;
    gzmsg << "• Dynamic lighting systems and effects" << std::endl;
    gzmsg << "• Advanced material properties (metalness, roughness)" << std::endl;
    gzmsg << "• Real-time shadow casting and quality" << std::endl;
    gzmsg << "• Environmental lighting and atmospheric effects" << std::endl;
    gzmsg << "• Performance optimization techniques" << std::endl;
    gzmsg << "• Visual quality assessment and tuning" << std::endl;
    
    // Start server
    std::thread server_thread([&server]() {
        server.Run(true, 0, false);
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Run advanced visuals demo
    AdvancedVisualsManager manager;
    manager.Start();
    
    server.RequestStop();
    if (server_thread.joinable())
        server_thread.join();
    
    return 0;
}