/*
 * Gazebo Visual Plugins Demo
 * 
 * This example demonstrates:
 * 1. Visual plugin architecture and rendering integration
 * 2. Custom rendering effects and shaders
 * 3. Dynamic material and texture manipulation
 * 4. Visual component lifecycle management
 * 5. Rendering performance optimization techniques
 * 
 * Learning objectives:
 * - Understanding visual plugin development patterns
 * - Rendering system integration and customization
 * - Material and texture management in Gazebo
 * - Visual effects and shader programming concepts
 * - Performance considerations for rendering plugins
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/rendering/RenderEngine.hh>
#include <gz/rendering/RenderingIface.hh>
#include <gz/rendering/Scene.hh>
#include <gz/rendering/Camera.hh>
#include <gz/rendering/Visual.hh>
#include <gz/rendering/Material.hh>
#include <gz/rendering/Geometry.hh>
#include <gz/rendering/Light.hh>

#include <gz/plugin/Register.hh>
#include <gz/plugin/Loader.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Color.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <memory>

using namespace gz;

/**
 * Visual Effect Information Structure
 */
struct VisualEffect
{
    std::string name;
    std::string description;
    bool is_active{false};
    std::chrono::steady_clock::time_point start_time;
    double duration{5.0};  // seconds
    std::map<std::string, double> parameters;
};

/**
 * Visual Plugins Demonstration
 */
class VisualPluginsDemo
{
public:
    VisualPluginsDemo()
    {
        gzmsg << "Visual Plugins Demo initialized\n";
        gzmsg << "Rendering system and visual effects demonstration\n\n";
        
        // Initialize rendering system
        InitializeRenderingSystem();
        
        // Setup visual effects
        SetupVisualEffects();
    }

    /**
     * Run comprehensive visual plugins demonstration
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Visual Plugins Demo ===\n\n";
        
        // Create server configuration
        ServerConfig config;
        config.SetSdfString(CreateVisualDemoWorldSdf());
        config.SetUpdateRate(60.0);
        
        Server server(config);
        
        gzmsg << "Starting visual plugins demonstration...\n";
        PrintRenderingArchitecture();
        
        // Demonstrate visual effects
        DemonstrateVisualEffects(server);
        
        PrintVisualPluginSummary();
    }

private:
    std::map<std::string, VisualEffect> visual_effects_;
    rendering::RenderEngine* render_engine_{nullptr};
    rendering::ScenePtr scene_{nullptr};

    /**
     * Initialize rendering system
     */
    void InitializeRenderingSystem()
    {
        gzmsg << "Initializing rendering system...\n";
        
        // Get available render engines
        auto engines = rendering::availableEngines();
        
        gzmsg << "Available render engines:\n";
        for (const auto& engine : engines)
        {
            gzmsg << "  - " << engine << "\n";
        }
        
        // Try to load a render engine (preferably Ogre2)
        std::vector<std::string> preferred_engines = {"ogre2", "ogre", "optix"};
        
        for (const auto& engine_name : preferred_engines)
        {
            if (std::find(engines.begin(), engines.end(), engine_name) != engines.end())
            {
                render_engine_ = rendering::engine(engine_name);
                if (render_engine_)
                {
                    gzmsg << "Successfully loaded render engine: " << engine_name << "\n";
                    break;
                }
            }
        }
        
        if (!render_engine_)
        {
            gzwarn << "Could not load preferred render engine, using default\n";
            if (!engines.empty())
            {
                render_engine_ = rendering::engine(engines[0]);
                gzmsg << "Using render engine: " << engines[0] << "\n";
            }
        }
        
        if (render_engine_)
        {
            // Create a scene
            scene_ = render_engine_->CreateScene("visual_demo_scene");
            if (scene_)
            {
                gzmsg << "Created rendering scene: visual_demo_scene\n";
            }
        }
        
        gzmsg << "Rendering system initialization complete\n\n";
    }

    /**
     * Setup visual effects demonstrations
     */
    void SetupVisualEffects()
    {
        // Color animation effect
        VisualEffect color_animation;
        color_animation.name = "color_animation";
        color_animation.description = "Dynamic color cycling of materials";
        color_animation.duration = 10.0;
        color_animation.parameters["speed"] = 2.0;
        color_animation.parameters["intensity"] = 0.8;
        visual_effects_[color_animation.name] = color_animation;
        
        // Transparency effects
        VisualEffect transparency_fade;
        transparency_fade.name = "transparency_fade";
        transparency_fade.description = "Alpha blending and transparency animation";
        transparency_fade.duration = 8.0;
        transparency_fade.parameters["min_alpha"] = 0.2;
        transparency_fade.parameters["max_alpha"] = 1.0;
        visual_effects_[transparency_fade.name] = transparency_fade;
        
        // Material property animation
        VisualEffect material_properties;
        material_properties.name = "material_properties";
        material_properties.description = "Animated material properties (metallic, roughness)";
        material_properties.duration = 12.0;
        material_properties.parameters["metallic_range"] = 0.8;
        material_properties.parameters["roughness_range"] = 0.9;
        visual_effects_[material_properties.name] = material_properties;
        
        // Lighting effects
        VisualEffect dynamic_lighting;
        dynamic_lighting.name = "dynamic_lighting";
        dynamic_lighting.description = "Dynamic light properties and positioning";
        dynamic_lighting.duration = 15.0;
        dynamic_lighting.parameters["light_radius"] = 5.0;
        dynamic_lighting.parameters["intensity_variation"] = 0.5;
        visual_effects_[dynamic_lighting.name] = dynamic_lighting;
        
        // Texture animation effects
        VisualEffect texture_animation;
        texture_animation.name = "texture_animation";
        texture_animation.description = "UV coordinate animation and texture transformation";
        texture_animation.duration = 6.0;
        texture_animation.parameters["uv_speed"] = 1.5;
        texture_animation.parameters["scale_factor"] = 2.0;
        visual_effects_[texture_animation.name] = texture_animation;
        
        gzmsg << "Visual effects configured: " << visual_effects_.size() << " effects\n\n";
    }

    /**
     * Create world SDF for visual demonstration
     */
    std::string CreateVisualDemoWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="visual_plugins_demo">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.01667</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    <plugin filename="gz-sim-sensors-system" name="gz::sim::systems::Sensors"/>
    
    <!-- Advanced lighting setup -->
    <light name="main_sun" type="directional">
      <pose>5 5 10 -0.5 -0.5 -0.785</pose>
      <cast_shadows>true</cast_shadows>
      <direction>-0.5 -0.5 -1</direction>
      <diffuse>0.9 0.9 0.8 1</diffuse>
      <specular>0.7 0.7 0.6 1</specular>
      <intensity>1.2</intensity>
    </light>
    
    <!-- Dynamic point light -->
    <light name="dynamic_light" type="point">
      <pose>3 0 3 0 0 0</pose>
      <cast_shadows>false</cast_shadows>
      <diffuse>1 0.3 0.3 1</diffuse>
      <specular>0.8 0.2 0.2 1</specular>
      <intensity>0.8</intensity>
      <range>8</range>
      <attenuation>
        <constant>0.1</constant>
        <linear>0.05</linear>
        <quadratic>0.01</quadratic>
      </attenuation>
    </light>
    
    <!-- Spot light for dramatic effects -->
    <light name="spot_light" type="spot">
      <pose>0 5 4 0 -0.5 -1.57</pose>
      <cast_shadows>true</cast_shadows>
      <diffuse>0.2 0.7 1 1</diffuse>
      <specular>0.1 0.5 0.8 1</specular>
      <intensity>1.0</intensity>
      <range>10</range>
      <spot>
        <inner_angle>0.3</inner_angle>
        <outer_angle>0.6</outer_angle>
        <falloff>1.0</falloff>
      </spot>
    </light>
    
    <!-- Ground plane with advanced material -->
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
          <material>
            <ambient>0.5 0.5 0.5 1</ambient>
            <diffuse>0.8 0.8 0.8 1</diffuse>
            <specular>0.2 0.2 0.2 1</specular>
            <pbr>
              <metal>
                <metalness>0.1</metalness>
                <roughness>0.8</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Visual effects test objects -->
    
    <!-- Color animation sphere -->
    <model name="color_sphere">
      <pose>-3 -3 1 0 0 0</pose>
      <static>false</static>
      <link name="link">
        <inertial>
          <mass>1.0</mass>
        </inertial>
        <collision name="collision">
          <geometry>
            <sphere><radius>0.8</radius></sphere>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <sphere><radius>0.8</radius></sphere>
          </geometry>
          <material>
            <ambient>1 0 0 1</ambient>
            <diffuse>1 0 0 1</diffuse>
            <specular>0.8 0.8 0.8 1</specular>
            <pbr>
              <metal>
                <metalness>0.3</metalness>
                <roughness>0.2</roughness>
              </metal>
            </pbr>
          </material>
          
          <!-- Custom visual plugin would be attached here -->
          <!-- <plugin filename="libcustom_visual_plugin.so" name="tutorial::ColorAnimationPlugin">
            <effect>color_animation</effect>
            <speed>2.0</speed>
          </plugin> -->
        </visual>
      </link>
    </model>
    
    <!-- Transparency test box -->
    <model name="transparency_box">
      <pose>3 -3 0.5 0 0 0</pose>
      <static>false</static>
      <link name="link">
        <inertial>
          <mass>2.0</mass>
        </inertial>
        <collision name="collision">
          <geometry>
            <box><size>1.5 1.5 1.0</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>1.5 1.5 1.0</size></box>
          </geometry>
          <material>
            <ambient>0 1 1 0.8</ambient>
            <diffuse>0 1 1 0.8</diffuse>
            <specular>1 1 1 1</specular>
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.1</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Metallic/roughness test cylinder -->
    <model name="material_cylinder">
      <pose>0 3 1 0 0 0</pose>
      <static>false</static>
      <link name="link">
        <inertial>
          <mass>1.5</mass>
        </inertial>
        <collision name="collision">
          <geometry>
            <cylinder>
              <radius>0.6</radius>
              <length>1.8</length>
            </cylinder>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.6</radius>
              <length>1.8</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0.8 0.6 0.2 1</ambient>
            <diffuse>0.9 0.7 0.3 1</diffuse>
            <specular>1 1 1 1</specular>
            <pbr>
              <metal>
                <metalness>0.8</metalness>
                <roughness>0.1</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Complex geometry for advanced effects -->
    <model name="effect_object">
      <pose>-2 2 0.75 0 0 0</pose>
      <static>false</static>
      <link name="link">
        <inertial>
          <mass>3.0</mass>
        </inertial>
        <collision name="collision">
          <geometry>
            <mesh>
              <uri>model://shapes/meshes/monkey.dae</uri>
              <scale>1.5 1.5 1.5</scale>
            </mesh>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <mesh>
              <uri>model://shapes/meshes/monkey.dae</uri>
              <scale>1.5 1.5 1.5</scale>
            </mesh>
          </geometry>
          <material>
            <ambient>0.4 0.2 0.8 1</ambient>
            <diffuse>0.5 0.3 0.9 1</diffuse>
            <specular>0.9 0.9 0.9 1</specular>
            <emissive>0.1 0.05 0.2 1</emissive>
            <pbr>
              <metal>
                <metalness>0.6</metalness>
                <roughness>0.3</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print rendering architecture information
     */
    void PrintRenderingArchitecture()
    {
        gzmsg << "Gazebo Rendering Architecture:\n";
        gzmsg << "=============================\n";
        
        if (render_engine_)
        {
            gzmsg << "Active Render Engine: " << render_engine_->Name() << "\n";
            gzmsg << "Engine Capabilities:\n";
            gzmsg << "- Scene management and spatial organization\n";
            gzmsg << "- Material and shader system integration\n";
            gzmsg << "- Lighting and shadow computation\n";
            gzmsg << "- Camera and viewport management\n";
            gzmsg << "- Texture and resource loading\n\n";
        }
        
        gzmsg << "Visual Plugin Architecture:\n";
        gzmsg << "- Plugin attachment to visual components\n";
        gzmsg << "- Direct rendering system access\n";
        gzmsg << "- Material property manipulation\n";
        gzmsg << "- Shader parameter control\n";
        gzmsg << "- Animation and effect management\n\n";
        
        gzmsg << "Rendering Pipeline:\n";
        gzmsg << "1. Scene graph construction and culling\n";
        gzmsg << "2. Material and shader binding\n";
        gzmsg << "3. Lighting calculation and shadow mapping\n";
        gzmsg << "4. Geometry transformation and rendering\n";
        gzmsg << "5. Post-processing effects and composition\n\n";
        
        gzmsg << "Material System:\n";
        gzmsg << "- Physically Based Rendering (PBR) support\n";
        gzmsg << "- Metallic-roughness workflow\n";
        gzmsg << "- Normal and displacement mapping\n";
        gzmsg << "- Emissive and transparency handling\n";
        gzmsg << "- Dynamic material property updates\n\n";
    }

    /**
     * Demonstrate visual effects and rendering capabilities
     */
    void DemonstrateVisualEffects(Server& server)
    {
        gzmsg << "Visual Effects Demonstration:\n";
        gzmsg << "============================\n";
        
        // Run simulation with visual effects
        const int simulation_steps = 9000;  // 2.5 minutes at 60 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        // Activate first effect
        auto effect_it = visual_effects_.begin();
        if (effect_it != visual_effects_.end())
        {
            effect_it->second.is_active = true;
            effect_it->second.start_time = start_time;
            gzmsg << "Starting effect: " << effect_it->second.name << "\n";
        }
        
        while (step_count < simulation_steps)
        {
            server.RunOnce(false);
            step_count++;
            
            auto current_time = std::chrono::steady_clock::now();
            
            // Update visual effects
            UpdateVisualEffects(current_time);
            
            // Progress reporting every 30 seconds
            if (step_count % 1800 == 0)
            {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                gzmsg << "Visual demo progress: " << elapsed << " seconds\n";
                
                PrintActiveEffects(current_time);
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(16667)); // ~60 FPS
        }
        
        gzmsg << "Visual effects demonstration completed\n\n";
    }

    /**
     * Update visual effects states and transitions
     */
    void UpdateVisualEffects(std::chrono::steady_clock::time_point current_time)
    {
        // Check for effect completions and start new ones
        for (auto& pair : visual_effects_)
        {
            auto& effect = pair.second;
            
            if (effect.is_active)
            {
                double elapsed = std::chrono::duration<double>(current_time - effect.start_time).count();
                
                if (elapsed >= effect.duration)
                {
                    effect.is_active = false;
                    gzmsg << "Completed effect: " << effect.name << "\n";
                    
                    // Start next effect
                    StartNextEffect(current_time);
                }
                else
                {
                    // Update effect parameters based on time
                    ApplyEffectUpdate(effect, elapsed / effect.duration);
                }
            }
        }
    }

    /**
     * Start the next available visual effect
     */
    void StartNextEffect(std::chrono::steady_clock::time_point current_time)
    {
        // Find next inactive effect
        for (auto& pair : visual_effects_)
        {
            auto& effect = pair.second;
            if (!effect.is_active)
            {
                effect.is_active = true;
                effect.start_time = current_time;
                gzmsg << "Starting effect: " << effect.name << " - " << effect.description << "\n";
                return;
            }
        }
        
        // If all effects completed, restart from beginning
        auto& first_effect = visual_effects_.begin()->second;
        first_effect.is_active = true;
        first_effect.start_time = current_time;
        gzmsg << "Restarting effects cycle with: " << first_effect.name << "\n";
    }

    /**
     * Apply effect updates (simulated - would manipulate actual materials/shaders)
     */
    void ApplyEffectUpdate(const VisualEffect& effect, double progress)
    {
        // Simulate visual effect updates
        if (effect.name == "color_animation")
        {
            // Simulate color cycling
            double hue = fmod(progress * effect.parameters.at("speed") * 360.0, 360.0);
            // Would apply to actual material: material->SetDiffuse(ColorFromHSV(hue, 1.0, 1.0));
        }
        else if (effect.name == "transparency_fade")
        {
            // Simulate alpha animation
            double min_alpha = effect.parameters.at("min_alpha");
            double max_alpha = effect.parameters.at("max_alpha");
            double alpha = min_alpha + (max_alpha - min_alpha) * (0.5 + 0.5 * sin(progress * 2 * M_PI));
            // Would apply to actual material: material->SetTransparency(1.0 - alpha);
        }
        else if (effect.name == "material_properties")
        {
            // Simulate PBR property animation
            double metallic = 0.5 + 0.5 * sin(progress * effect.parameters.at("metallic_range") * 2 * M_PI);
            double roughness = 0.5 + 0.5 * cos(progress * effect.parameters.at("roughness_range") * 2 * M_PI);
            // Would apply to actual material: material->SetMetalness(metallic), material->SetRoughness(roughness);
        }
        else if (effect.name == "dynamic_lighting")
        {
            // Simulate light movement and intensity changes
            double radius = effect.parameters.at("light_radius");
            double x = radius * cos(progress * 2 * M_PI);
            double y = radius * sin(progress * 2 * M_PI);
            double intensity_variation = effect.parameters.at("intensity_variation");
            double intensity = 0.8 + intensity_variation * sin(progress * 4 * M_PI);
            // Would apply to actual light: light->SetWorldPosition(x, y, 3), light->SetIntensity(intensity);
        }
        else if (effect.name == "texture_animation")
        {
            // Simulate UV coordinate animation
            double uv_offset = progress * effect.parameters.at("uv_speed");
            double scale = 1.0 + 0.5 * sin(progress * effect.parameters.at("scale_factor") * 2 * M_PI);
            // Would apply to actual texture: material->SetTextureUVTransform(uv_offset, uv_offset, scale, scale);
        }
    }

    /**
     * Print currently active effects
     */
    void PrintActiveEffects(std::chrono::steady_clock::time_point current_time)
    {
        gzmsg << "Active Visual Effects:\n";
        bool any_active = false;
        
        for (const auto& pair : visual_effects_)
        {
            const auto& effect = pair.second;
            if (effect.is_active)
            {
                double elapsed = std::chrono::duration<double>(current_time - effect.start_time).count();
                double progress = elapsed / effect.duration;
                
                gzmsg << "  - " << effect.name << ": " << std::fixed << std::setprecision(1) 
                      << (progress * 100.0) << "% complete\n";
                any_active = true;
            }
        }
        
        if (!any_active)
        {
            gzmsg << "  No active effects\n";
        }
        gzmsg << "\n";
    }

    /**
     * Print comprehensive visual plugin summary
     */
    void PrintVisualPluginSummary()
    {
        gzmsg << "\n=== VISUAL PLUGINS SUMMARY ===\n\n";
        
        gzmsg << "Gazebo Visual System Architecture:\n";
        gzmsg << "--------------------------------\n";
        gzmsg << "Rendering Integration:\n";
        gzmsg << "• Direct access to rendering engine and scene graph\n";
        gzmsg << "• Material and shader system manipulation\n";
        gzmsg << "• Real-time visual property updates\n";
        gzmsg << "• Custom rendering pipeline extensions\n\n";
        
        gzmsg << "Visual Plugin Capabilities:\n";
        gzmsg << "• Dynamic material property animation\n";
        gzmsg << "• Custom shader effects and post-processing\n";
        gzmsg << "• Procedural texture generation and manipulation\n";
        gzmsg << "• Lighting system integration and control\n";
        gzmsg << "• Particle system and special effects\n\n";
        
        gzmsg << "Material System Features:\n";
        gzmsg << "------------------------\n";
        gzmsg << "Physically Based Rendering (PBR):\n";
        gzmsg << "• Metallic-roughness workflow for realistic materials\n";
        gzmsg << "• Energy-conserving BRDF (Bidirectional Reflectance Distribution Function)\n";
        gzmsg << "• HDR (High Dynamic Range) lighting support\n";
        gzmsg << "• Image-based lighting with environment maps\n\n";
        
        gzmsg << "Texture and Mapping Support:\n";
        gzmsg << "• Albedo (diffuse) texture mapping\n";
        gzmsg << "• Normal mapping for surface detail\n";
        gzmsg << "• Metallic and roughness texture channels\n";
        gzmsg << "• Emissive mapping for self-illumination\n";
        gzmsg << "• Ambient occlusion and cavity mapping\n";
        gzmsg << "• UV coordinate transformation and animation\n\n";
        
        gzmsg << "Advanced Rendering Techniques:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "Lighting and Shadows:\n";
        gzmsg << "• Shadow mapping with multiple cascade levels\n";
        gzmsg << "• Screen-space ambient occlusion (SSAO)\n";
        gzmsg << "• Volumetric lighting and fog effects\n";
        gzmsg << "• Global illumination approximations\n\n";
        
        gzmsg << "Post-Processing Effects:\n";
        gzmsg << "• Bloom and tone mapping for HDR rendering\n";
        gzmsg << "• Screen-space reflections and refractions\n";
        gzmsg << "• Depth-of-field and motion blur\n";
        gzmsg << "• Color grading and gamma correction\n\n";
        
        gzmsg << "Performance Optimization:\n";
        gzmsg << "-----------------------\n";
        gzmsg << "Rendering Efficiency:\n";
        gzmsg << "• Level-of-detail (LOD) systems for geometry\n";
        gzmsg << "• Frustum and occlusion culling\n";
        gzmsg << "• Batching and instancing for similar objects\n";
        gzmsg << "• Shader optimization and caching\n\n";
        
        gzmsg << "Memory Management:\n";
        gzmsg << "• Texture compression and streaming\n";
        gzmsg << "• Geometry buffer optimization\n";
        gzmsg << "• Resource pooling and reuse\n";
        gzmsg << "• Garbage collection for unused assets\n\n";
        
        gzmsg << "Development Best Practices:\n";
        gzmsg << "-------------------------\n";
        gzmsg << "Plugin Architecture:\n";
        gzmsg << "• Clean separation of rendering logic from simulation\n";
        gzmsg << "• Event-driven updates for visual changes\n";
        gzmsg << "• Configurable parameters for customization\n";
        gzmsg << "• Thread-safe access to rendering resources\n\n";
        
        gzmsg << "Debugging and Profiling:\n";
        gzmsg << "• Visual debugging tools for material properties\n";
        gzmsg << "• Rendering pipeline visualization\n";
        gzmsg << "• Performance profiling and bottleneck analysis\n";
        gzmsg << "• Memory usage monitoring and optimization\n\n";
        
        gzmsg << "Cross-Platform Considerations:\n";
        gzmsg << "• OpenGL and Vulkan API compatibility\n";
        gzmsg << "• Shader language abstraction (HLSL/GLSL)\n";
        gzmsg << "• Hardware capability detection and fallbacks\n";
        gzmsg << "• Platform-specific optimization techniques\n\n";
        
        gzmsg << "=== VISUAL PLUGINS DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - Visual Plugins Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Visual Plugins Demo ===\n";
    gzmsg << "This demo showcases rendering system integration and visual effects.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- Visual plugin architecture and rendering integration\n";
    gzmsg << "- Material system and PBR (Physically Based Rendering)\n";
    gzmsg << "- Dynamic visual effects and animations\n";
    gzmsg << "- Lighting system control and manipulation\n";
    gzmsg << "- Performance optimization for rendering plugins\n\n";

    try
    {
        VisualPluginsDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during visual plugins demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nVisual plugins demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Visual plugins provide direct access to rendering systems\n";
    gzmsg << "- PBR materials enable photorealistic visual quality\n";
    gzmsg << "- Dynamic effects require careful performance consideration\n";
    gzmsg << "- Shader programming extends visual capabilities significantly\n";
    gzmsg << "- Proper resource management is crucial for visual plugins\n";
    
    return 0;
}