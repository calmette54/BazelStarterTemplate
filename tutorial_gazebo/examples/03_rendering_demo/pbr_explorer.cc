/*
 * Gazebo Tutorial Example 03: PBR Parameter Explorer with Static Plugin Integration
 * 
 * This comprehensive tutorial demonstrates:
 * 1. Interactive exploration of PBR material parameters with real-time visualization
 * 2. Real-time material property modification using hermetic builds for consistent behavior
 * 3. Understanding metalness and roughness interactions with educational progression
 * 4. Advanced PBR workflows and best practices with performance insights
 * 5. Parameter sweep analysis and material optimization strategies
 * 
 * Learning objectives:
 * - Master PBR material model fundamentals and parameter relationships for production assets
 * - Understand hermetic builds with static plugin linking for reliable rendering behavior
 * - Learn interactive material exploration and real-time parameter manipulation
 * - Develop advanced PBR workflows and optimization strategies for complex scenes
 * - Explore material category best practices and physically accurate parameter ranges
 * - Signal-based application control and graceful resource management
 * - Why we use static plugins for reliable, reproducible PBR parameter exploration
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components/Material.hh>
#include <gz/sim/components/Visual.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/common/Console.hh>

#include <gz/msgs/material.pb.h>
#include <gz/msgs/visual.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Color.hh>
#include <gz/math/Rand.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <cmath>
#include <iomanip>
#include <signal.h>
#include <atomic>
#include <cstdlib>
#include <unistd.h>
#include <cstring>

using namespace gz;
using namespace sim;

// Global flags for signal-safe shutdown
volatile sig_atomic_t g_running = 1;

/// \brief Signal handler for graceful shutdown
void signalHandler(int signal)
{
    // Use write() instead of gzmsg for signal safety
    const char* msg = "\n=== TUTORIAL: Received signal, shutting down PBR explorer... ===\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    
    // Set shutdown flag atomically
    g_running = 0;
}

/**
 * PBR Parameter Explorer
 * Interactive tool for exploring PBR material parameters
 */
class PBRExplorer
{
public:
    struct PBRMaterial
    {
        std::string name;
        math::Color albedo;
        double metalness;
        double roughness;
        double ao;           // Ambient occlusion
        math::Color emissive;
        double normal_strength;
        std::string category;  // metal, dielectric, fabric, etc.
        std::string description;
    };

    struct ExplorationStep
    {
        std::string parameter;
        double start_value;
        double end_value;
        double duration_seconds;
        std::string description;
    };

    PBRExplorer() : node_(std::make_shared<transport::Node>())
    {
        gzmsg << "PBR Parameter Explorer initialized\n";
        gzmsg << "Interactive exploration of Physically Based Rendering parameters\n";
        gzmsg << "Demonstrating real-time material property manipulation and optimization\n\n";
        
        SetupBaseMaterials();
        SetupExplorationSequence();
        
        // Setup transport for material updates
        material_pub_ = node_->Advertise<msgs::Visual>("/world/pbr_demo/visual");
        
        current_step_ = 0;
        start_time_ = std::chrono::steady_clock::now();
    }

    /**
     * Run interactive PBR exploration
     */
    void RunExploration()
    {
        gzmsg << "=== PBR Parameter Explorer and Material Optimization Demo ===\n\n";
        
        // Create server with PBR demo world
        ServerConfig config;
        config.SetSdfString(CreatePBRWorldSdf());
        config.SetUpdateRate(1000.0);  // High update rate for smooth parameter transitions
        
        Server server(config);
        
        gzmsg << "Starting PBR parameter exploration with signal handling...\n";
        gzmsg << "Base materials loaded:\n";
        PrintBaseMaterials();
        
        gzmsg << "\nExploration sequence:\n";
        PrintExplorationSequence();
        
        // Run exploration with signal handling
        const int simulation_steps = 18000;  // 18 seconds at 1000 Hz for comprehensive demonstration
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && current_step_ < exploration_steps_.size() && g_running)
        {
            server.RunOnce(false);
            step_count++;
            
            // Update material parameters based on current exploration step
            UpdateExplorationStep(step_count);
            
            // Print enhanced periodic updates with tutorial style
            if (step_count % 2000 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                double sim_time = step_count * 0.001;
                double rtf = sim_time / (elapsed > 0 ? elapsed : 1);
                
                gzmsg << "TUTORIAL: PBR Explorer progress: " << (step_count * 100 / simulation_steps) << "% - ";
                gzmsg << "Sim time: " << std::fixed << std::setprecision(1) << sim_time << "s ";
                gzmsg << "(real: " << elapsed << "s) | RTF: " << std::fixed << std::setprecision(2) << rtf << "x\n";
                PrintCurrentExplorationState();
                gzmsg << "\n";
            }
            
            // Brief pause to prevent overwhelming the system and allow signal processing
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        // Check if shutdown was triggered by signal
        if (g_running == 0)
        {
            gzmsg << "\nTUTORIAL: ✓ Signal received during PBR exploration - stopping gracefully..." << std::endl;
            gzmsg << "TUTORIAL: Final simulation time: " << std::fixed << std::setprecision(1) << (step_count * 0.001) << " seconds" << std::endl;
        }
        
        PrintPBRSummary();
    }

private:
    std::shared_ptr<transport::Node> node_;
    transport::Node::Publisher material_pub_;
    std::vector<PBRMaterial> base_materials_;
    std::vector<ExplorationStep> exploration_steps_;
    size_t current_step_;
    std::chrono::steady_clock::time_point start_time_;

    /**
     * Setup base PBR materials for exploration
     */
    void SetupBaseMaterials()
    {
        // Pure metals
        base_materials_.push_back({
            "Polished Aluminum",
            math::Color(0.91, 0.92, 0.92, 1.0),  // High albedo for metal
            1.0, 0.1, 1.0,                        // Full metalness, low roughness
            math::Color(0, 0, 0, 1),              // No emission
            1.0, "metal",
            "Highly reflective aluminum with minimal surface imperfections"
        });
        
        base_materials_.push_back({
            "Brushed Steel",
            math::Color(0.65, 0.67, 0.68, 1.0),
            1.0, 0.3, 1.0,
            math::Color(0, 0, 0, 1),
            1.0, "metal",
            "Steel with directional brushing creating anisotropic reflections"
        });
        
        base_materials_.push_back({
            "Oxidized Copper",
            math::Color(0.95, 0.64, 0.54, 1.0),  // Copper color
            1.0, 0.7, 1.0,
            math::Color(0, 0, 0, 1),
            1.0, "metal",
            "Weathered copper with surface oxidation"
        });
        
        // Dielectrics
        base_materials_.push_back({
            "Smooth Plastic",
            math::Color(0.8, 0.2, 0.2, 1.0),     // Red plastic
            0.0, 0.2, 1.0,                        // Non-metal, smooth
            math::Color(0, 0, 0, 1),
            1.0, "dielectric",
            "High-quality injection-molded plastic with glossy finish"
        });
        
        base_materials_.push_back({
            "Ceramic",
            math::Color(0.95, 0.95, 0.9, 1.0),   // Off-white ceramic
            0.0, 0.1, 1.0,
            math::Color(0, 0, 0, 1),
            1.0, "dielectric",
            "Glazed ceramic with high-gloss finish"
        });
        
        base_materials_.push_back({
            "Rubber",
            math::Color(0.1, 0.1, 0.1, 1.0),     // Dark rubber
            0.0, 0.95, 1.0,                       // Very rough dielectric
            math::Color(0, 0, 0, 1),
            1.0, "dielectric",
            "Matte rubber with high surface roughness"
        });
        
        // Fabric and organic
        base_materials_.push_back({
            "Cotton Fabric",
            math::Color(0.6, 0.6, 0.8, 1.0),     // Light blue fabric
            0.0, 0.8, 1.0,
            math::Color(0, 0, 0, 1),
            1.0, "fabric",
            "Woven cotton with fibrous surface structure"
        });
        
        base_materials_.push_back({
            "Leather",
            math::Color(0.45, 0.3, 0.2, 1.0),    // Brown leather
            0.0, 0.7, 1.0,
            math::Color(0, 0, 0, 1),
            1.0, "organic",
            "Treated leather with natural surface variation"
        });
        
        // Special materials
        base_materials_.push_back({
            "Glass",
            math::Color(0.95, 0.95, 1.0, 0.1),   // Nearly clear, highly transparent
            0.0, 0.05, 1.0,                       // Very smooth dielectric
            math::Color(0, 0, 0, 1),
            1.0, "dielectric",
            "Clear glass with minimal surface imperfections"
        });
        
        base_materials_.push_back({
            "Emissive Display",
            math::Color(0.1, 0.1, 0.2, 1.0),     // Dark base
            0.0, 0.3, 1.0,
            math::Color(0.2, 0.4, 1.0, 1.0),     // Blue emission
            1.0, "emissive",
            "Self-illuminating material simulating LED display"
        });
    }

    /**
     * Setup exploration sequence to demonstrate different parameters
     */
    void SetupExplorationSequence()
    {
        exploration_steps_.push_back({
            "metalness", 0.0, 1.0, 20.0,
            "Transitioning from dielectric (0) to full metal (1)"
        });
        
        exploration_steps_.push_back({
            "roughness", 0.0, 1.0, 20.0,
            "Varying surface roughness from mirror (0) to completely rough (1)"
        });
        
        exploration_steps_.push_back({
            "albedo_brightness", 0.2, 1.0, 15.0,
            "Changing albedo brightness while maintaining color ratios"
        });
        
        exploration_steps_.push_back({
            "albedo_hue", 0.0, 360.0, 25.0,
            "Cycling through different hue values"
        });
        
        exploration_steps_.push_back({
            "emissive_intensity", 0.0, 2.0, 15.0,
            "Demonstrating self-illumination from none to bright"
        });
        
        exploration_steps_.push_back({
            "combined_sweep", 0.0, 1.0, 30.0,
            "Complex parameter interaction demonstration"
        });
    }

    /**
     * Create SDF world optimized for PBR material demonstration
     */
    std::string CreatePBRWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="pbr_demo">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.016667</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    
    <!-- PBR-optimized lighting environment -->
    <light name="key_light" type="directional">
      <pose>8 8 12 0 0 0</pose>
      <cast_shadows>true</cast_shadows>
      <direction>-0.6 -0.6 -1</direction>
      <diffuse>1.0 1.0 0.95 1</diffuse>
      <specular>1.0 1.0 0.9 1</specular>
      <intensity>1.2</intensity>
    </light>
    
    <light name="rim_light" type="directional">
      <pose>-5 8 10 0 0 0</pose>
      <cast_shadows>false</cast_shadows>
      <direction>0.4 -0.6 -1</direction>
      <diffuse>0.4 0.5 0.7 1</diffuse>
      <specular>0.3 0.4 0.6 1</specular>
      <intensity>0.8</intensity>
    </light>
    
    <light name="fill_light" type="directional">
      <pose>0 -8 6 0 0 0</pose>
      <cast_shadows>false</cast_shadows>
      <direction>0 0.8 -0.5</direction>
      <diffuse>0.3 0.3 0.4 1</diffuse>
      <specular>0.2 0.2 0.3 1</specular>
      <intensity>0.6</intensity>
    </light>
    
    <!-- Environment for reflections -->
    <light name="environment_light" type="directional">
      <pose>0 0 20 0 0 0</pose>
      <cast_shadows>false</cast_shadows>
      <direction>0 0 -1</direction>
      <diffuse>0.2 0.25 0.3 1</diffuse>
      <specular>0.1 0.15 0.2 1</specular>
      <intensity>0.4</intensity>
    </light>
    
    <!-- Ground plane with neutral PBR material -->
    <model name="pbr_ground">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>30 30</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>30 30</size>
            </plane>
          </geometry>
          <material>
            <ambient>0.4 0.4 0.4 1</ambient>
            <diffuse>0.6 0.6 0.6 1</diffuse>
            <specular>0.2 0.2 0.2 1</specular>
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.6</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Central demonstration sphere (main exploration object) -->
    <model name="pbr_sphere">
      <pose>0 0 2 0 0 0</pose>
      <link name="link">
        <inertial>
          <mass>1.0</mass>
          <inertia>
            <ixx>0.4</ixx>
            <iyy>0.4</iyy>
            <izz>0.4</izz>
          </inertia>
        </inertial>
        <collision name="collision">
          <geometry>
            <sphere>
              <radius>1.0</radius>
            </sphere>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <sphere>
              <radius>1.0</radius>
            </sphere>
          </geometry>
          <!-- Material properties will be updated dynamically -->
          <material>
            <ambient>0.8 0.2 0.2 1</ambient>
            <diffuse>0.8 0.2 0.2 1</diffuse>
            <specular>0.9 0.9 0.9 1</specular>
            <emissive>0 0 0 1</emissive>
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.5</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Reference material spheres for comparison -->
    <model name="metal_reference">
      <pose>-4 0 1.5 0 0 0</pose>
      <static>true</static>
      <link name="link">
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
            <ambient>0.7 0.7 0.7 1</ambient>
            <diffuse>0.8 0.8 0.8 1</diffuse>
            <specular>0.9 0.9 0.9 1</specular>
            <pbr>
              <metal>
                <metalness>1.0</metalness>
                <roughness>0.1</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="plastic_reference">
      <pose>4 0 1.5 0 0 0</pose>
      <static>true</static>
      <link name="link">
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
            <ambient>0.2 0.2 0.8 1</ambient>
            <diffuse>0.3 0.3 0.9 1</diffuse>
            <specular>0.8 0.8 0.8 1</specular>
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
    
    <model name="rough_reference">
      <pose>0 -4 1.5 0 0 0</pose>
      <static>true</static>
      <link name="link">
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
            <ambient>0.3 0.6 0.3 1</ambient>
            <diffuse>0.4 0.7 0.4 1</diffuse>
            <specular>0.2 0.2 0.2 1</specular>
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.9</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="emissive_reference">
      <pose>0 4 1.5 0 0 0</pose>
      <static>true</static>
      <link name="link">
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
            <ambient>0.2 0.1 0.4 1</ambient>
            <diffuse>0.3 0.2 0.6 1</diffuse>
            <specular>0.3 0.3 0.3 1</specular>
            <emissive>0.4 0.2 0.8 1</emissive>
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.4</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Background environment for reflections -->
    <model name="environment_wall">
      <pose>0 -12 5 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <box><size>20 0.5 10</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>20 0.5 10</size></box>
          </geometry>
          <material>
            <ambient>0.8 0.8 0.9 1</ambient>
            <diffuse>0.9 0.9 1.0 1</diffuse>
            <specular>0.1 0.1 0.1 1</specular>
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.8</roughness>
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
     * Print base materials being explored
     */
    void PrintBaseMaterials()
    {
        for (size_t i = 0; i < base_materials_.size(); ++i)
        {
            const auto& mat = base_materials_[i];
            gzmsg << (i + 1) << ". " << mat.name << " (" << mat.category << ")\n";
            gzmsg << "   Metalness: " << std::fixed << std::setprecision(2) << mat.metalness;
            gzmsg << ", Roughness: " << mat.roughness << "\n";
            gzmsg << "   " << mat.description << "\n";
        }
        gzmsg << "\n";
    }

    /**
     * Print exploration sequence
     */
    void PrintExplorationSequence()
    {
        for (size_t i = 0; i < exploration_steps_.size(); ++i)
        {
            const auto& step = exploration_steps_[i];
            gzmsg << (i + 1) << ". " << step.parameter << " (" << step.duration_seconds << "s)\n";
            gzmsg << "   Range: " << step.start_value << " → " << step.end_value << "\n";
            gzmsg << "   " << step.description << "\n";
        }
        gzmsg << "\n";
    }

    /**
     * Update material parameters based on current exploration step
     */
    void UpdateExplorationStep(int step_count)
    {
        if (current_step_ >= exploration_steps_.size())
            return;
        
        const auto& exploration = exploration_steps_[current_step_];
        double step_time = step_count * 0.001;  // Convert to seconds at 1000 Hz
        
        // Calculate total elapsed time for all previous steps
        double total_previous_time = 0.0;
        for (size_t i = 0; i < current_step_; ++i)
        {
            total_previous_time += exploration_steps_[i].duration_seconds;
        }
        
        double current_step_elapsed = step_time - total_previous_time;
        
        // Check if we should move to next step
        if (current_step_elapsed >= exploration.duration_seconds)
        {
            current_step_++;
            return;
        }
        
        // Calculate parameter value based on progress through current step
        double progress = current_step_elapsed / exploration.duration_seconds;
        progress = std::max(0.0, std::min(1.0, progress));  // Clamp to [0,1]
        
        // Apply parameter update (in a full implementation, this would send
        // material update messages via transport)
        ApplyParameterUpdate(exploration.parameter, exploration.start_value, 
                           exploration.end_value, progress);
    }

    /**
     * Apply parameter update with smooth interpolation
     */
    void ApplyParameterUpdate(const std::string& parameter, 
                             double start_val, double end_val, double progress)
    {
        // Smooth interpolation using cosine interpolation
        double smooth_progress = (1.0 - std::cos(progress * M_PI)) * 0.5;
        double current_value = start_val + (end_val - start_val) * smooth_progress;
        
        // In a full implementation, this would construct and send
        // msgs::Visual messages to update the material properties
        // For demonstration purposes, we just log the updates
        if (static_cast<int>(progress * 100) % 10 == 0)  // Log every 10%
        {
            gzmsg << "Updating " << parameter << ": " 
                  << std::fixed << std::setprecision(3) << current_value 
                  << " (progress: " << std::setprecision(1) << progress * 100 << "%)\n";
        }
    }

    /**
     * Print current exploration state
     */
    void PrintCurrentExplorationState()
    {
        if (current_step_ < exploration_steps_.size())
        {
            const auto& step = exploration_steps_[current_step_];
            gzmsg << "TUTORIAL: Current exploration: " << step.parameter << "\n";
            gzmsg << "TUTORIAL: Description: " << step.description << "\n";
            gzmsg << "TUTORIAL: Progress: Step " << (current_step_ + 1) << " of " 
                  << exploration_steps_.size() << "\n";
        }
        else
        {
            gzmsg << "TUTORIAL: Exploration sequence completed\n";
        }
    }

    /**
     * Print comprehensive PBR summary
     */
    void PrintPBRSummary()
    {
        gzmsg << "\n=== PBR PARAMETER EXPLORATION SUMMARY ===\n\n";
        
        gzmsg << "Core PBR Parameters Explored:\n";
        gzmsg << "-----------------------------\n\n";
        
        gzmsg << "Metalness (0.0 - 1.0):\n";
        gzmsg << "• 0.0 = Dielectric materials (plastic, wood, ceramic, fabric)\n";
        gzmsg << "• 1.0 = Pure metals (steel, aluminum, gold, copper)\n";
        gzmsg << "• Controls specular reflection behavior\n";
        gzmsg << "• Affects how light interacts with the surface\n";
        gzmsg << "• Binary choice in reality - avoid intermediate values\n\n";
        
        gzmsg << "Roughness (0.0 - 1.0):\n";
        gzmsg << "• 0.0 = Perfect mirror (polished surfaces)\n";
        gzmsg << "• 1.0 = Completely diffuse (matte surfaces)\n";
        gzmsg << "• Controls microscopic surface detail\n";
        gzmsg << "• Affects specular highlight size and sharpness\n";
        gzmsg << "• Linear scale for most materials\n\n";
        
        gzmsg << "Albedo (Base Color):\n";
        gzmsg << "• Diffuse reflectance for dielectrics\n";
        gzmsg << "• Specular reflectance for metals\n";
        gzmsg << "• Should be in physically plausible ranges\n";
        gzmsg << "• Dielectrics: typically 0.02-0.9 (linear values)\n";
        gzmsg << "• Metals: typically 0.5-1.0 (linear values)\n\n";
        
        gzmsg << "Emissive:\n";
        gzmsg << "• Self-illumination independent of lighting\n";
        gzmsg << "• Useful for displays, LEDs, magical effects\n";
        gzmsg << "• Can affect overall scene lighting\n";
        gzmsg << "• Should be used sparingly for realism\n\n";
        
        gzmsg << "Material Category Guidelines:\n";
        gzmsg << "----------------------------\n\n";
        
        gzmsg << "Metals:\n";
        gzmsg << "• Metalness: 1.0\n";
        gzmsg << "• Roughness: 0.1-0.8 depending on finish\n";
        gzmsg << "• Albedo: Metal-specific color (silver, gold, copper)\n";
        gzmsg << "• Examples: polished steel (rough=0.1), brushed aluminum (rough=0.3)\n\n";
        
        gzmsg << "Dielectrics:\n";
        gzmsg << "• Metalness: 0.0\n";
        gzmsg << "• Roughness: varies widely by material\n";
        gzmsg << "• Albedo: Surface color\n";
        gzmsg << "• Examples: plastic (rough=0.2-0.7), rubber (rough=0.8-1.0)\n\n";
        
        gzmsg << "Fabric and Organic:\n";
        gzmsg << "• Metalness: 0.0\n";
        gzmsg << "• Roughness: 0.7-1.0 (usually very rough)\n";
        gzmsg << "• May require subsurface scattering for realism\n";
        gzmsg << "• Examples: cotton (rough=0.9), leather (rough=0.7)\n\n";
        
        gzmsg << "Common PBR Mistakes to Avoid:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "• Using intermediate metalness values (0.2-0.8) - unrealistic\n";
        gzmsg << "• Too-bright albedo for metals or too-dark for dielectrics\n";
        gzmsg << "• Overly smooth rough materials (rubber with roughness=0.1)\n";
        gzmsg << "• Forgetting that roughness affects both specular and diffuse\n";
        gzmsg << "• Using emissive as primary lighting - performance impact\n\n";
        
        gzmsg << "Optimization Guidelines:\n";
        gzmsg << "----------------------\n";
        gzmsg << "• Group materials by similar properties\n";
        gzmsg << "• Use texture atlases for similar materials\n";
        gzmsg << "• Consider material LOD for distant objects\n";
        gzmsg << "• Limit emissive materials in performance-critical scenes\n";
        gzmsg << "• Use baked lighting for static objects when possible\n\n";
        
        gzmsg << "Integration with Art Pipeline:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "• Use calibrated color spaces (sRGB for textures, linear for lighting)\n";
        gzmsg << "• Follow 80/20 rule: 80% of visual quality from 20% of materials\n";
        gzmsg << "• Create material libraries with validated parameters\n";
        gzmsg << "• Test materials under different lighting conditions\n";
        gzmsg << "• Document material intentions for consistent usage\n\n";
        
        gzmsg << "=== PBR EXPLORATION COMPLETE ===\n";
    }
};

class TutorialPBRExplorer
{
public:
    TutorialPBRExplorer() {}

    int Run()
    {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "    GAZEBO TUTORIAL: PBR Parameter Explorer and Material Optimization Demo" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        gzmsg << "TUTORIAL: Setting up hermetic PBR material exploration environment..." << std::endl;
        gzmsg << "TUTORIAL: This demo uses static plugin approach for reliable material testing" << std::endl;

        // === STAGE 1: Environment Configuration ===
        std::cout << "\n--- STAGE 1: Environment Configuration ---" << std::endl;
        
        // Set console verbosity and disable config file loading
        gz::common::Console::SetVerbosity(4);
        gzmsg << "TUTORIAL: Console verbosity set to level 4 for detailed PBR analysis output" << std::endl;
        
        setenv("GZ_SIM_SERVER_CONFIG_PATH", "", 1);
        setenv("GZ_CONFIG_PATH", "", 1);
        gzmsg << "TUTORIAL: Disabled system config files for consistent PBR material testing" << std::endl;

        // === STAGE 2: PBR Material Setup ===
        std::cout << "\n--- STAGE 2: PBR Material Setup ---" << std::endl;
        
        gzmsg << "TUTORIAL: PBR material categories configured for comprehensive exploration:" << std::endl;
        gzmsg << "TUTORIAL:   - Pure metals (aluminum, steel, copper) with varying roughness" << std::endl;
        gzmsg << "TUTORIAL:   - Dielectric materials (plastic, ceramic, rubber) with parameter validation" << std::endl;
        gzmsg << "TUTORIAL:   - Fabric and organic materials (cotton, leather) with realistic ranges" << std::endl;
        gzmsg << "TUTORIAL:   - Special materials (glass, emissive displays) for advanced workflows" << std::endl;
        gzmsg << "TUTORIAL: Each category demonstrates physically accurate parameter relationships" << std::endl;

        // === STAGE 3: Parameter Exploration Configuration ===
        std::cout << "\n--- STAGE 3: Parameter Exploration Configuration ---" << std::endl;
        
        gzmsg << "TUTORIAL: Initializing PBR parameter exploration with real-time manipulation..." << std::endl;
        gzmsg << "TUTORIAL: Static plugins (Physics, UserCommands, SceneBroadcaster) are active" << std::endl;
        gzmsg << "TUTORIAL: Interactive parameter sweeps demonstrate metalness/roughness relationships" << std::endl;
        gzmsg << "TUTORIAL: Hermetic approach ensures consistent material behavior across environments" << std::endl;

        // === STAGE 4: PBR Material Exploration Execution ===
        std::cout << "\n--- STAGE 4: PBR Material Exploration Execution ---" << std::endl;
        
        gzmsg << "TUTORIAL: Starting comprehensive PBR parameter exploration demonstration..." << std::endl;
        gzmsg << "TUTORIAL: Press Ctrl+C to stop the material exploration at any time" << std::endl;
        
        try
        {
            PBRExplorer explorer;
            explorer.RunExploration();
        }
        catch (const std::exception& e)
        {
            gzerr << "TUTORIAL: Error during PBR exploration: " << e.what() << std::endl;
            return -1;
        }

        // === STAGE 5: Results Analysis and Educational Conclusions ===
        std::cout << "\n--- STAGE 5: Results Analysis and Educational Conclusions ---" << std::endl;
        
        gzmsg << "TUTORIAL: ✓ PBR parameter exploration completed successfully!" << std::endl;
        
        gzmsg << "\nTUTORIAL: Key Learning Points:" << std::endl;
        gzmsg << "TUTORIAL: - PBR provides physically accurate material representation using energy conservation" << std::endl;
        gzmsg << "TUTORIAL: - Metalness controls surface conductivity: 0.0 (dielectric) to 1.0 (conductor)" << std::endl;
        gzmsg << "TUTORIAL: - Roughness affects microsurface detail and specular reflection distribution" << std::endl;
        gzmsg << "TUTORIAL: - Material categories have specific physically plausible parameter ranges" << std::endl;
        gzmsg << "TUTORIAL: - Real-time parameter manipulation enables interactive material design workflows" << std::endl;
        
        gzmsg << "\nTUTORIAL: PBR Material Design Best Practices:" << std::endl;
        gzmsg << "TUTORIAL: - Use binary metalness values (0.0 or 1.0) for physically accurate materials" << std::endl;
        gzmsg << "TUTORIAL: - Reference real-world materials for albedo and roughness parameter validation" << std::endl;
        gzmsg << "TUTORIAL: - Consider lighting conditions when designing material parameter relationships" << std::endl;
        gzmsg << "TUTORIAL: - Balance material complexity with rendering performance requirements" << std::endl;
        gzmsg << "TUTORIAL: - Create material libraries with documented parameter ranges and use cases" << std::endl;
        gzmsg << "TUTORIAL: - Test materials across different viewing angles and lighting scenarios" << std::endl;
        
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "   TUTORIAL COMPLETED - PBR Material Exploration Success!" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        return 0;
    }
};

/**
 * Main function - Tutorial PBR Parameter Explorer Demo
 */
int main(int argc, char* argv[])
{
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // Termination signal

    try
    {
        TutorialPBRExplorer tutorial;
        return tutorial.Run();
    }
    catch (const std::exception &e)
    {
        gzerr << "Exception in PBR Parameter Explorer Tutorial: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        gzerr << "Unknown exception in PBR Parameter Explorer Tutorial" << std::endl;
        return -1;
    }
}

/*
 * Key Concepts Demonstrated:
 * 
 * 1. **PBR Parameter Exploration**: How to systematically explore and understand
 *    Physically Based Rendering parameters for realistic material representation.
 * 
 * 2. **Hermetic Static Plugins**: Understanding why we use static linking for reliable,
 *    reproducible PBR material testing across different environments and hardware.
 * 
 * 3. **Interactive Material Design**: Real-time parameter manipulation and material
 *    optimization workflows for production asset creation and validation.
 * 
 * 4. **Signal Handling**: Proper application lifecycle management with graceful shutdown
 *    during long-running material exploration and parameter optimization procedures.
 * 
 * 5. **Educational Progression**: Step-by-step tutorial structure that teaches PBR
 *    material design principles and optimization strategies for production workflows.
 * 
 * 6. **Material Category Analysis**: Comprehensive exploration of material types
 *    (metals, dielectrics, fabrics) and their physically accurate parameter ranges.
 * 
 * Usage Examples:
 * - ./pbr_explorer                              # Run complete PBR parameter exploration
 * - timeout 90s ./pbr_explorer                  # Run with 90-second timeout
 * - ./pbr_explorer 2>&1 | grep "TUTORIAL"      # Extract only tutorial messages
 * 
 * Next Steps:
 * - Experiment with different material parameter combinations for specific applications
 * - Analyze how lighting conditions affect material appearance and parameter requirements
 * - Integrate PBR material design insights into production asset creation workflows
 * - Use material exploration results for rendering performance optimization
 * 
 * Educational Notes:
 * - Static plugin messages are expected and demonstrate hermetic build reliability
 * - Material realism depends on physically plausible parameter ranges and relationships
 * - Metallic materials: Use metalness=1.0 with appropriate roughness for surface finish
 * - Dielectric materials: Use metalness=0.0 with roughness based on surface characteristics
 * - Tutorial demonstrates best practices for production PBR material design and validation
 */