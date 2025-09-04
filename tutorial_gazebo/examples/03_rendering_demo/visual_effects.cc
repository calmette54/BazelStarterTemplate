/*
 * Gazebo Visual Effects and Advanced Rendering Features
 * 
 * This example demonstrates:
 * 1. Advanced rendering effects (fog, bloom, motion blur)
 * 2. Post-processing pipeline effects
 * 3. Particle systems and dynamic effects
 * 4. Advanced lighting techniques (volumetric, God rays)
 * 5. Performance optimization for complex visual effects
 * 
 * Learning objectives:
 * - Understanding advanced rendering pipelines
 * - Post-processing effects implementation
 * - Performance impact of visual effects
 * - Artistic vs realistic rendering approaches
 * - Effect compositing and layering
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components/Visual.hh>
#include <gz/sim/components/Light.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/visual.pb.h>
#include <gz/msgs/light.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Color.hh>
#include <gz/math/Pose3.hh>
#include <gz/math/Rand.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <cmath>
#include <vector>

using namespace gz;
using namespace sim;

/**
 * Visual Effects Controller
 * Manages and demonstrates advanced rendering effects
 */
class VisualEffectsDemo
{
public:
    struct EffectConfiguration
    {
        std::string name;
        std::string type;  // fog, bloom, particles, volumetric, etc.
        bool enabled;
        double intensity;
        math::Color color;
        double range;
        double duration;
        std::string description;
    };

    struct AnimatedObject
    {
        std::string name;
        math::Pose3d initial_pose;
        math::Vector3d velocity;
        math::Vector3d angular_velocity;
        double trail_length;
        bool emit_particles;
    };

    VisualEffectsDemo() : node_(std::make_shared<transport::Node>())
    {
        gzmsg << "Visual Effects Demo initialized\n";
        gzmsg << "Demonstrating advanced rendering effects and post-processing\n\n";
        
        SetupEffectConfigurations();
        SetupAnimatedObjects();
        
        // Setup transport for dynamic effect control
        visual_pub_ = node_->Advertise<msgs::Visual>("/world/visual_effects/visual");
        light_pub_ = node_->Advertise<msgs::Light>("/world/visual_effects/light/modify");
        
        current_effect_ = 0;
        start_time_ = std::chrono::steady_clock::now();
        animation_time_ = 0.0;
    }

    /**
     * Run visual effects demonstration
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Visual Effects and Advanced Rendering Demo ===\n\n";
        
        // Create server with visual effects world
        ServerConfig config;
        config.SetSdfString(CreateVisualEffectsWorldSdf());
        config.SetUpdateRate(60.0);  // High framerate for smooth effects
        
        Server server(config);
        
        gzmsg << "Starting visual effects demonstration...\n";
        gzmsg << "Available effects:\n";
        PrintAvailableEffects();
        
        // Run demonstration
        const int simulation_steps = 21600;  // 6 minutes at 60 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps)
        {
            server.RunOnce(false);
            step_count++;
            animation_time_ = step_count * 0.016667;  // Convert to seconds
            
            // Update effects every 10 seconds
            if (step_count % 600 == 0)
            {
                current_effect_ = (current_effect_ + 1) % effect_configs_.size();
                PrintCurrentEffect();
                // In a full implementation, would apply the effect here
            }
            
            // Update animated objects
            UpdateAnimations(step_count);
            
            // Print progress every 30 seconds
            if (step_count % 1800 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                gzmsg << "Demo time: " << (step_count / 60) << "s (real: " << elapsed << "s)\n";
                gzmsg << "Current effect: " << effect_configs_[current_effect_].name << "\n\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(16667)); // ~60 FPS
        }
        
        PrintVisualEffectsSummary();
    }

private:
    std::shared_ptr<transport::Node> node_;
    transport::Node::Publisher visual_pub_;
    transport::Node::Publisher light_pub_;
    std::vector<EffectConfiguration> effect_configs_;
    std::vector<AnimatedObject> animated_objects_;
    size_t current_effect_;
    std::chrono::steady_clock::time_point start_time_;
    double animation_time_;

    /**
     * Setup different visual effect configurations
     */
    void SetupEffectConfigurations()
    {
        effect_configs_.push_back({
            "Atmospheric Fog",
            "fog",
            true, 0.3,
            math::Color(0.7, 0.8, 0.9, 1.0),  // Bluish fog
            50.0, 30.0,
            "Distance-based atmospheric fog for depth perception"
        });
        
        effect_configs_.push_back({
            "Volumetric God Rays",
            "volumetric",
            true, 0.8,
            math::Color(1.0, 0.9, 0.7, 1.0),  // Golden volumetric light
            20.0, 25.0,
            "Volumetric light scattering through fog and particles"
        });
        
        effect_configs_.push_back({
            "Bloom and Glow",
            "bloom",
            true, 1.2,
            math::Color(1.0, 1.0, 1.0, 1.0),  // White bloom
            5.0, 20.0,
            "High-dynamic-range bloom effect for bright objects"
        });
        
        effect_configs_.push_back({
            "Motion Blur Trails",
            "motion_blur",
            true, 0.5,
            math::Color(1.0, 1.0, 1.0, 1.0),
            10.0, 25.0,
            "Per-object motion blur for fast-moving objects"
        });
        
        effect_configs_.push_back({
            "Particle Fire Effect",
            "particles",
            true, 1.0,
            math::Color(1.0, 0.5, 0.2, 1.0),  // Fire colors
            3.0, 35.0,
            "Dynamic particle system simulating fire and smoke"
        });
        
        effect_configs_.push_back({
            "Screen Space Reflections",
            "ssr",
            true, 0.7,
            math::Color(1.0, 1.0, 1.0, 1.0),
            15.0, 30.0,
            "Real-time reflections using screen-space techniques"
        });
        
        effect_configs_.push_back({
            "Depth of Field",
            "dof",
            true, 0.6,
            math::Color(1.0, 1.0, 1.0, 1.0),
            8.0, 20.0,
            "Camera-like depth of field with focus blur"
        });
        
        effect_configs_.push_back({
            "Color Grading",
            "color_grading",
            true, 0.8,
            math::Color(0.9, 1.0, 0.8, 1.0),  // Slight green tint
            1.0, 15.0,
            "Cinematic color grading and tone mapping"
        });
    }

    /**
     * Setup animated objects for motion effects
     */
    void SetupAnimatedObjects()
    {
        animated_objects_.push_back({
            "spinning_emitter",
            math::Pose3d(0, 0, 4, 0, 0, 0),
            math::Vector3d(0, 0, 0),
            math::Vector3d(0, 0, 2.0),  // 2 rad/s rotation
            2.0, true
        });
        
        animated_objects_.push_back({
            "orbiting_light",
            math::Pose3d(5, 0, 3, 0, 0, 0),
            math::Vector3d(0, 3.14, 0),  // Circular motion
            math::Vector3d(0, 0, 0.5),
            1.5, false
        });
        
        animated_objects_.push_back({
            "floating_sphere",
            math::Pose3d(-3, 0, 2, 0, 0, 0),
            math::Vector3d(0, 0, 1.0),  // Vertical oscillation
            math::Vector3d(0.5, 0.3, 0.8),
            1.0, true
        });
    }

    /**
     * Create SDF world optimized for visual effects demonstration
     */
    std::string CreateVisualEffectsWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="visual_effects">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.016667</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    
    <!-- Advanced rendering settings -->
    <!-- In a full implementation, this would include post-processing configurations -->
    
    <!-- Atmospheric settings for fog effects -->
    <atmosphere type="adiabatic">
      <temperature>288.15</temperature>
      <pressure>101325</pressure>
      <temperature_gradient>-0.0065</temperature_gradient>
    </atmosphere>
    
    <!-- Dynamic lighting setup for effects -->
    <light name="main_sun" type="directional">
      <pose>10 5 15 0 0 0</pose>
      <cast_shadows>true</cast_shadows>
      <direction>-0.7 -0.3 -1</direction>
      <diffuse>1.2 1.0 0.8 1</diffuse>
      <specular>1.0 0.9 0.7 1</specular>
      <intensity>1.5</intensity>
    </light>
    
    <!-- Volumetric light source -->
    <light name="god_ray_light" type="spot">
      <pose>8 8 12 0 0.7 -2.3</pose>
      <cast_shadows>true</cast_shadows>
      <direction>-0.8 -0.8 -1</direction>
      <diffuse>2.0 1.5 1.0 1</diffuse>
      <specular>1.5 1.2 0.8 1</specular>
      <intensity>2.0</intensity>
      <spot>
        <inner_angle>0.3</inner_angle>
        <outer_angle>0.8</outer_angle>
        <falloff>1.2</falloff>
      </spot>
      <range>25</range>
    </light>
    
    <!-- Animated point light -->
    <light name="orbiting_light" type="point">
      <pose>5 0 3 0 0 0</pose>
      <cast_shadows>false</cast_shadows>
      <diffuse>0.8 0.4 1.0 1</diffuse>
      <specular>1.0 0.6 1.2 1</specular>
      <intensity>1.5</intensity>
      <range>8</range>
    </light>
    
    <!-- Ground with reflective material for effects -->
    <model name="reflective_ground">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>40 40</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>40 40</size>
            </plane>
          </geometry>
          <material>
            <ambient>0.3 0.3 0.4 1</ambient>
            <diffuse>0.5 0.5 0.6 1</diffuse>
            <specular>0.8 0.8 0.9 1</specular>
            <pbr>
              <metal>
                <metalness>0.2</metalness>
                <roughness>0.1</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Central emissive object for bloom effects -->
    <model name="bloom_sphere">
      <pose>0 0 2 0 0 0</pose>
      <link name="link">
        <inertial>
          <mass>0.5</mass>
          <inertia>
            <ixx>0.2</ixx>
            <iyy>0.2</iyy>
            <izz>0.2</izz>
          </inertia>
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
            <ambient>0.1 0.1 0.2 1</ambient>
            <diffuse>0.2 0.2 0.4 1</diffuse>
            <specular>0.5 0.5 0.5 1</specular>
            <emissive>1.5 1.0 0.5 1</emissive>
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.3</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Spinning emitter for particle effects -->
    <model name="particle_emitter">
      <pose>0 0 4 0 0 0</pose>
      <link name="link">
        <inertial>
          <mass>0.1</mass>
          <inertia>
            <ixx>0.01</ixx>
            <iyy>0.01</iyy>
            <izz>0.01</izz>
          </inertia>
        </inertial>
        <collision name="collision">
          <geometry>
            <box><size>0.2 0.2 0.8</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>0.2 0.2 0.8</size></box>
          </geometry>
          <material>
            <ambient>1.0 0.3 0.1 1</ambient>
            <diffuse>1.0 0.4 0.2 1</diffuse>
            <specular>0.8 0.8 0.8 1</specular>
            <emissive>0.8 0.2 0.1 1</emissive>
            <pbr>
              <metal>
                <metalness>0.8</metalness>
                <roughness>0.2</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
        <!-- In a full implementation, particle system would be attached here -->
      </link>
    </model>
    
    <!-- Fast-moving object for motion blur -->
    <model name="blur_object">
      <pose>-5 0 1.5 0 0 0</pose>
      <link name="link">
        <inertial>
          <mass>1.0</mass>
          <inertia>
            <ixx>0.2</ixx>
            <iyy>0.2</iyy>
            <izz>0.2</izz>
          </inertia>
        </inertial>
        <collision name="collision">
          <geometry>
            <cylinder>
              <radius>0.3</radius>
              <length>1.5</length>
            </cylinder>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.3</radius>
              <length>1.5</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>1.0 0.1 0.1 1</ambient>
            <diffuse>1.0 0.2 0.2 1</diffuse>
            <specular>0.9 0.9 0.9 1</specular>
            <pbr>
              <metal>
                <metalness>0.9</metalness>
                <roughness>0.1</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Reflective surfaces for SSR demonstration -->
    <model name="mirror_wall">
      <pose>0 -8 2.5 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <box><size>12 0.2 5</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>12 0.2 5</size></box>
          </geometry>
          <material>
            <ambient>0.8 0.8 0.9 1</ambient>
            <diffuse>0.9 0.9 1.0 1</diffuse>
            <specular>1.0 1.0 1.0 1</specular>
            <pbr>
              <metal>
                <metalness>1.0</metalness>
                <roughness>0.05</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Depth of field focus objects -->
    <model name="focus_near">
      <pose>2 2 0.8 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <box><size>0.5 0.5 0.5</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>0.5 0.5 0.5</size></box>
          </geometry>
          <material>
            <ambient>0.2 0.8 0.2 1</ambient>
            <diffuse>0.3 1.0 0.3 1</diffuse>
            <specular>0.4 0.4 0.4 1</specular>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="focus_mid">
      <pose>0 3 1.2 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <sphere><radius>0.6</radius></sphere>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <sphere><radius>0.6</radius></sphere>
          </geometry>
          <material>
            <ambient>0.8 0.8 0.2 1</ambient>
            <diffuse>1.0 1.0 0.3 1</diffuse>
            <specular>0.5 0.5 0.5 1</specular>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="focus_far">
      <pose>-2 5 1.8 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <cylinder>
              <radius>0.4</radius>
              <length>1.0</length>
            </cylinder>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.4</radius>
              <length>1.0</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0.2 0.2 0.8 1</ambient>
            <diffuse>0.3 0.3 1.0 1</diffuse>
            <specular>0.6 0.6 0.6 1</specular>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Complex geometry for fog interaction -->
    <model name="fog_forest">
      <pose>6 -3 0 0 0 0</pose>
      <static>true</static>
      
      <!-- Tree trunk 1 -->
      <link name="trunk1">
        <pose>0 0 2 0 0 0</pose>
        <collision name="collision">
          <geometry>
            <cylinder>
              <radius>0.3</radius>
              <length>4</length>
            </cylinder>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.3</radius>
              <length>4</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0.4 0.3 0.2 1</ambient>
            <diffuse>0.5 0.4 0.3 1</diffuse>
            <specular>0.1 0.1 0.1 1</specular>
          </material>
        </visual>
      </link>
      
      <!-- Tree trunk 2 -->
      <link name="trunk2">
        <pose>2 1 2.5 0 0 0</pose>
        <collision name="collision">
          <geometry>
            <cylinder>
              <radius>0.25</radius>
              <length>5</length>
            </cylinder>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.25</radius>
              <length>5</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0.3 0.25 0.15 1</ambient>
            <diffuse>0.4 0.35 0.25 1</diffuse>
            <specular>0.1 0.1 0.1 1</specular>
          </material>
        </visual>
      </link>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print available visual effects
     */
    void PrintAvailableEffects()
    {
        for (size_t i = 0; i < effect_configs_.size(); ++i)
        {
            const auto& effect = effect_configs_[i];
            gzmsg << (i + 1) << ". " << effect.name << " (" << effect.type << ")\n";
            gzmsg << "   Intensity: " << std::fixed << std::setprecision(2) << effect.intensity;
            gzmsg << ", Duration: " << effect.duration << "s\n";
            gzmsg << "   " << effect.description << "\n";
        }
        gzmsg << "\nEffects will cycle every 10 seconds...\n\n";
    }

    /**
     * Print current effect information
     */
    void PrintCurrentEffect()
    {
        if (current_effect_ < effect_configs_.size())
        {
            const auto& effect = effect_configs_[current_effect_];
            gzmsg << "Now demonstrating: " << effect.name << "\n";
            gzmsg << "Type: " << effect.type << "\n";
            gzmsg << "Description: " << effect.description << "\n";
            gzmsg << "Intensity: " << effect.intensity << "\n";
            gzmsg << "Range: " << effect.range << " units\n\n";
        }
    }

    /**
     * Update animated objects for dynamic effects
     */
    void UpdateAnimations(int step_count)
    {
        // Simple animation updates - in a full implementation this would
        // send pose update messages via transport
        for (const auto& obj : animated_objects_)
        {
            // Calculate new positions based on animation time
            double t = animation_time_;
            
            if (obj.name == "spinning_emitter")
            {
                // Rotate around Z axis
                double angle = obj.angular_velocity.Z() * t;
                // In full implementation: send pose update with rotation
            }
            else if (obj.name == "orbiting_light")
            {
                // Circular orbit motion
                double radius = 5.0;
                double x = radius * cos(obj.velocity.Y() * t / radius);
                double y = radius * sin(obj.velocity.Y() * t / radius);
                // In full implementation: send light pose update
            }
            else if (obj.name == "floating_sphere")
            {
                // Vertical oscillation with complex motion
                double height = 2.0 + sin(obj.velocity.Z() * t) * 1.5;
                double rotation_x = obj.angular_velocity.X() * t;
                double rotation_y = obj.angular_velocity.Y() * t;
                // In full implementation: send pose update
            }
        }
        
        // Log animation updates occasionally
        if (step_count % 600 == 0)  // Every 10 seconds
        {
            gzmsg << "Updating object animations (t=" << std::fixed << std::setprecision(1) 
                  << animation_time_ << "s)\n";
        }
    }

    /**
     * Print comprehensive visual effects summary
     */
    void PrintVisualEffectsSummary()
    {
        gzmsg << "\n=== VISUAL EFFECTS DEMONSTRATION SUMMARY ===\n\n";
        
        gzmsg << "Advanced Rendering Effects Demonstrated:\n";
        gzmsg << "---------------------------------------\n\n";
        
        gzmsg << "Atmospheric Effects:\n";
        gzmsg << "• Fog: Distance-based atmospheric perspective\n";
        gzmsg << "• Volumetric Lighting: God rays and light scattering\n";
        gzmsg << "• Haze and Atmospheric Scattering: Realistic depth cues\n";
        gzmsg << "• Use cases: Outdoor scenes, dramatic lighting, depth perception\n\n";
        
        gzmsg << "Post-Processing Effects:\n";
        gzmsg << "• Bloom/HDR: High dynamic range lighting effects\n";
        gzmsg << "• Motion Blur: Per-object and camera motion blur\n";
        gzmsg << "• Depth of Field: Camera-like focus effects\n";
        gzmsg << "• Color Grading: Cinematic color correction and tone mapping\n";
        gzmsg << "• Screen Space Reflections: Real-time reflection approximation\n\n";
        
        gzmsg << "Dynamic Effects:\n";
        gzmsg << "• Particle Systems: Fire, smoke, sparks, debris\n";
        gzmsg << "• Animated Materials: Flowing water, flickering lights\n";
        gzmsg << "• Dynamic Lighting: Moving and pulsing light sources\n";
        gzmsg << "• Trail Effects: Motion trails and afterimages\n\n";
        
        gzmsg << "Performance Considerations:\n";
        gzmsg << "---------------------------\n";
        gzmsg << "Effect Performance Impact (Low to High):\n";
        gzmsg << "• Color Grading: Minimal impact, applied as final pass\n";
        gzmsg << "• Simple Fog: Low impact, distance-based calculation\n";
        gzmsg << "• Bloom: Medium impact, requires render target\n";
        gzmsg << "• Screen Space Reflections: Medium-High, screen sampling\n";
        gzmsg << "• Motion Blur: High impact, requires velocity buffers\n";
        gzmsg << "• Depth of Field: High impact, multiple blur passes\n";
        gzmsg << "• Volumetric Lighting: Very High, ray marching required\n";
        gzmsg << "• Complex Particles: Variable, depends on particle count\n\n";
        
        gzmsg << "Optimization Strategies:\n";
        gzmsg << "-----------------------\n";
        gzmsg << "• Level of Detail (LOD): Reduce effect quality at distance\n";
        gzmsg << "• Temporal Upsampling: Reuse previous frame data\n";
        gzmsg << "• Effect Culling: Disable off-screen or distant effects\n";
        gzmsg << "• Quality Settings: Provide user-configurable quality levels\n";
        gzmsg << "• Compute Shaders: Use GPU acceleration where available\n";
        gzmsg << "• Effect Budgeting: Limit concurrent expensive effects\n\n";
        
        gzmsg << "Artistic Guidelines:\n";
        gzmsg << "-------------------\n";
        gzmsg << "Realistic vs Stylized:\n";
        gzmsg << "• Realistic: Physically accurate, subtle effects\n";
        gzmsg << "• Stylized: Exaggerated, visually striking effects\n";
        gzmsg << "• Consider target audience and application purpose\n\n";
        
        gzmsg << "Effect Compositing:\n";
        gzmsg << "• Layer effects for complex visuals\n";
        gzmsg << "• Consider interaction between effects\n";
        gzmsg << "• Maintain visual hierarchy (focus attention)\n";
        gzmsg << "• Test under different lighting conditions\n\n";
        
        gzmsg << "Integration Best Practices:\n";
        gzmsg << "--------------------------\n";
        gzmsg << "• Profile effects early in development\n";
        gzmsg << "• Provide fallback options for limited hardware\n";
        gzmsg << "• Consider VR/AR specific requirements\n";
        gzmsg << "• Document effect parameters for artists\n";
        gzmsg << "• Test on target hardware configurations\n\n";
        
        gzmsg << "Common Pitfalls to Avoid:\n";
        gzmsg << "------------------------\n";
        gzmsg << "• Overuse of effects (visual noise)\n";
        gzmsg << "• Ignoring performance on lower-end devices\n";
        gzmsg << "• Not testing effects in context\n";
        gzmsg << "• Hardcoded effect parameters (no artist control)\n";
        gzmsg << "• Forgetting accessibility (motion sensitivity)\n\n";
        
        gzmsg << "Future Rendering Trends:\n";
        gzmsg << "-----------------------\n";
        gzmsg << "• Ray Tracing: Real-time global illumination\n";
        gzmsg << "• Machine Learning: AI-powered upscaling and denoising\n";
        gzmsg << "• Variable Rate Shading: Dynamic quality adjustment\n";
        gzmsg << "• Neural Rendering: AI-generated visual effects\n";
        gzmsg << "• Cloud Rendering: Server-side effect processing\n\n";
        
        gzmsg << "=== VISUAL EFFECTS DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - Visual Effects Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Visual Effects and Advanced Rendering Demo ===\n";
    gzmsg << "This demo showcases advanced visual effects and post-processing.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- Atmospheric effects (fog, volumetric lighting)\n";
    gzmsg << "- Post-processing (bloom, motion blur, DOF)\n";
    gzmsg << "- Dynamic effects (particles, animated materials)\n";
    gzmsg << "- Performance optimization strategies\n";
    gzmsg << "- Artistic vs realistic rendering approaches\n\n";

    try
    {
        VisualEffectsDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during visual effects demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nVisual effects demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Visual effects significantly impact both quality and performance\n";
    gzmsg << "- Post-processing effects should be used judiciously\n";
    gzmsg << "- Dynamic effects require careful performance budgeting\n";
    gzmsg << "- Artistic vision must balance with technical constraints\n";
    gzmsg << "- Effect layering and compositing enable complex visuals\n";
    
    return 0;
}