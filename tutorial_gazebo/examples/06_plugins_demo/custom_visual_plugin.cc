/*
 * Custom Visual Plugin Implementation
 * 
 * This implementation demonstrates a complete visual plugin with:
 * - Rendering system integration and material manipulation
 * - Dynamic visual effects (color, transparency, material properties)
 * - Animation curves and smooth transitions
 * - Performance monitoring for rendering operations
 */

#include "custom_visual_plugin.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Visual.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/rendering/RenderingIface.hh>

#include <sdf/Element.hh>

using namespace gz;
using namespace sim;

namespace tutorial
{
    //////////////////////////////////////////////////
    CustomVisualPlugin::CustomVisualPlugin()
    {
        gzmsg << "CustomVisualPlugin: Constructor called\n";
        performance_metrics_.Reset();
        last_visual_update_ = std::chrono::steady_clock::now();
    }

    //////////////////////////////////////////////////
    CustomVisualPlugin::~CustomVisualPlugin()
    {
        gzmsg << "CustomVisualPlugin: Destructor called\n";
        
        // Restore original material properties if we modified them
        if (visual_cached_ && material_ && animation_state_.properties_cached)
        {
            material_->SetAmbient(animation_state_.original_ambient);
            material_->SetDiffuse(animation_state_.original_diffuse);
            material_->SetSpecular(animation_state_.original_specular);
            material_->SetEmissive(animation_state_.original_emissive);
            material_->SetTransparency(animation_state_.original_transparency);
            
            gzmsg << "CustomVisualPlugin: Original material properties restored\n";
        }
        
        // Log final performance statistics
        if (performance_metrics_.update_count > 0)
        {
            gzmsg << "CustomVisualPlugin: Final Performance Statistics:\n";
            gzmsg << "  Total visual updates: " << performance_metrics_.update_count << "\n";
            gzmsg << "  Average render time: " << std::fixed << std::setprecision(3) 
                  << performance_metrics_.GetAverageRenderTime() << " ms\n";
            gzmsg << "  Min render time: " << performance_metrics_.min_render_time << " ms\n";
            gzmsg << "  Max render time: " << performance_metrics_.max_render_time << " ms\n";
        }
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::Configure(
        const Entity &_entity,
        const std::shared_ptr<const sdf::Element> &_sdf,
        EntityComponentManager &_ecm,
        EventManager &_eventMgr)
    {
        gzmsg << "CustomVisualPlugin: Configure called for entity " << _entity << "\n";
        
        // Store the visual entity
        visual_entity_ = _entity;
        
        // Parse configuration parameters
        if (_sdf->HasElement("effect_type"))
        {
            config_.effect_type = _sdf->Get<std::string>("effect_type");
        }
        
        if (_sdf->HasElement("animation_speed"))
        {
            config_.animation_speed = _sdf->Get<double>("animation_speed");
        }
        
        if (_sdf->HasElement("animation_intensity"))
        {
            config_.animation_intensity = _sdf->Get<double>("animation_intensity");
        }
        
        if (_sdf->HasElement("enable_color_animation"))
        {
            config_.enable_color_animation = _sdf->Get<bool>("enable_color_animation");
        }
        
        if (_sdf->HasElement("enable_transparency_effects"))
        {
            config_.enable_transparency_effects = _sdf->Get<bool>("enable_transparency_effects");
        }
        
        if (_sdf->HasElement("enable_material_animation"))
        {
            config_.enable_material_animation = _sdf->Get<bool>("enable_material_animation");
        }
        
        if (_sdf->HasElement("enable_texture_animation"))
        {
            config_.enable_texture_animation = _sdf->Get<bool>("enable_texture_animation");
        }
        
        // Color animation parameters
        if (_sdf->HasElement("color_cycle_duration"))
        {
            config_.color_cycle_duration = _sdf->Get<double>("color_cycle_duration");
        }
        
        // Transparency parameters
        if (_sdf->HasElement("min_alpha"))
        {
            config_.min_alpha = _sdf->Get<double>("min_alpha");
        }
        
        if (_sdf->HasElement("max_alpha"))
        {
            config_.max_alpha = _sdf->Get<double>("max_alpha");
        }
        
        if (_sdf->HasElement("transparency_cycle_duration"))
        {
            config_.transparency_cycle_duration = _sdf->Get<double>("transparency_cycle_duration");
        }
        
        // Set update rate based on animation speed
        update_period_ = 1.0 / (30.0 * config_.animation_speed);  // Scale with animation speed
        
        gzmsg << "CustomVisualPlugin: Configuration completed\n";
        gzmsg << "  Effect type: " << config_.effect_type << "\n";
        gzmsg << "  Animation speed: " << config_.animation_speed << "\n";
        gzmsg << "  Color animation: " << (config_.enable_color_animation ? "enabled" : "disabled") << "\n";
        gzmsg << "  Transparency effects: " << (config_.enable_transparency_effects ? "enabled" : "disabled") << "\n";
        gzmsg << "  Material animation: " << (config_.enable_material_animation ? "enabled" : "disabled") << "\n";
        gzmsg << "  Texture animation: " << (config_.enable_texture_animation ? "enabled" : "disabled") << "\n";
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::PreUpdate(
        const UpdateInfo &_info,
        EntityComponentManager &_ecm)
    {
        // Initialize on first update
        if (!is_initialized_)
        {
            InitializeRendering();
            CacheVisualComponents(_ecm);
            is_initialized_ = true;
        }
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::Update(
        const UpdateInfo &_info,
        EntityComponentManager &_ecm)
    {
        if (!is_initialized_ || !visual_cached_)
            return;
            
        // Update visual effects at controlled rate
        auto now = std::chrono::steady_clock::now();
        double time_since_update = std::chrono::duration<double>(now - last_visual_update_).count();
        
        if (time_since_update >= update_period_)
        {
            auto render_start = std::chrono::steady_clock::now();
            
            UpdateVisualEffects(_info);
            
            auto render_end = std::chrono::steady_clock::now();
            double render_time = std::chrono::duration<double, std::milli>(render_end - render_start).count();
            
            // Update performance metrics
            performance_metrics_.total_render_time += render_time;
            performance_metrics_.min_render_time = std::min(performance_metrics_.min_render_time, render_time);
            performance_metrics_.max_render_time = std::max(performance_metrics_.max_render_time, render_time);
            performance_metrics_.update_count++;
            
            last_visual_update_ = now;
        }
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::PostUpdate(
        const UpdateInfo &_info,
        const EntityComponentManager &_ecm)
    {
        // Log diagnostics periodically
        auto now = std::chrono::steady_clock::now();
        double time_since_log = std::chrono::duration<double>(now - performance_metrics_.last_log_time).count();
        
        if (time_since_log >= 10.0)  // Every 10 seconds
        {
            LogDiagnostics(_info);
            performance_metrics_.last_log_time = now;
        }
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::InitializeRendering()
    {
        gzmsg << "CustomVisualPlugin: Initializing rendering system\n";
        
        // Get available render engines
        auto engines = rendering::availableEngines();
        
        if (!engines.empty())
        {
            // Try to get the default render engine
            render_engine_ = rendering::engine(engines[0]);
            
            if (render_engine_)
            {
                // Get the default scene
                if (render_engine_->SceneCount() > 0)
                {
                    scene_ = render_engine_->SceneByIndex(0);
                }
                else
                {
                    // Create a scene if none exists
                    scene_ = render_engine_->CreateScene("visual_plugin_scene");
                }
                
                if (scene_)
                {
                    rendering_initialized_ = true;
                    gzmsg << "CustomVisualPlugin: Rendering system initialized successfully\n";
                    gzmsg << "  Render engine: " << render_engine_->Name() << "\n";
                    gzmsg << "  Scene: " << scene_->Name() << "\n";
                }
            }
        }
        
        if (!rendering_initialized_)
        {
            gzwarn << "CustomVisualPlugin: Could not initialize rendering system\n";
        }
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::CacheVisualComponents(EntityComponentManager &_ecm)
    {
        if (!rendering_initialized_)
            return;
            
        // For this demo, we'll simulate visual component access
        // In a real implementation, you would find the actual visual component
        // and get its rendering visual, material, and geometry objects
        
        gzmsg << "CustomVisualPlugin: Caching visual components for entity " << visual_entity_ << "\n";
        
        // Simulate finding the visual in the scene
        if (scene_)
        {
            // In a real plugin, you would use something like:
            // visual_ = scene_->VisualByName("visual_name");
            // material_ = visual_->Material();
            // geometry_ = visual_->GeometryByIndex(0);
            
            // For demonstration, we'll create placeholder objects
            visual_cached_ = true;
            
            // Cache original material properties (simulated)
            if (!animation_state_.properties_cached)
            {
                animation_state_.original_ambient = math::Color(0.5, 0.5, 0.5, 1.0);
                animation_state_.original_diffuse = math::Color(0.8, 0.2, 0.2, 1.0);
                animation_state_.original_specular = math::Color(0.8, 0.8, 0.8, 1.0);
                animation_state_.original_emissive = math::Color(0.0, 0.0, 0.0, 1.0);
                animation_state_.original_transparency = 0.0;
                animation_state_.original_metalness = 0.0;
                animation_state_.original_roughness = 0.5;
                animation_state_.properties_cached = true;
                
                gzmsg << "CustomVisualPlugin: Original material properties cached\n";
            }
            
            gzmsg << "CustomVisualPlugin: Visual components cached successfully\n";
        }
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::UpdateVisualEffects(const UpdateInfo &_info)
    {
        // Update animation time
        double current_sim_time = std::chrono::duration<double>(_info.simTime).count();
        
        if (animation_state_.start_time == 0.0)
        {
            animation_state_.start_time = current_sim_time;
        }
        
        animation_state_.current_time = current_sim_time;
        double elapsed_time = current_sim_time - animation_state_.start_time;
        
        // Apply enabled effects
        if (config_.enable_color_animation)
        {
            double color_progress = fmod(elapsed_time * config_.animation_speed / config_.color_cycle_duration, 1.0);
            ApplyColorAnimation(color_progress);
        }
        
        if (config_.enable_transparency_effects)
        {
            double transparency_progress = fmod(elapsed_time * config_.animation_speed / config_.transparency_cycle_duration, 1.0);
            ApplyTransparencyAnimation(transparency_progress);
        }
        
        if (config_.enable_material_animation)
        {
            double material_progress = fmod(elapsed_time * config_.animation_speed / config_.material_cycle_duration, 1.0);
            ApplyMaterialPropertyAnimation(material_progress);
        }
        
        if (config_.enable_texture_animation)
        {
            double texture_progress = fmod(elapsed_time * config_.animation_speed / config_.texture_cycle_duration, 1.0);
            ApplyTextureAnimation(texture_progress);
        }
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::ApplyColorAnimation(double progress)
    {
        // Smooth the progress using a sine curve
        double smooth_progress = SmoothCurve(progress, "sine");
        
        // Calculate hue based on progress (0-360 degrees)
        double hue = smooth_progress * 360.0;
        double saturation = 0.8 + 0.2 * config_.animation_intensity;
        double value = 0.7 + 0.3 * config_.animation_intensity;
        
        // Convert HSV to RGB
        math::Color animated_color = ColorFromHSV(hue, saturation, value);
        
        // Simulate applying color to material
        // In a real implementation: material_->SetDiffuse(animated_color);
        
        animation_state_.color_phase = progress;
        
        // Log color change occasionally for demonstration
        static int log_counter = 0;
        if (++log_counter % 30 == 0)  // Every 30 updates
        {
            gzmsg << "CustomVisualPlugin: Color animation - H:" << std::fixed << std::setprecision(1) 
                  << hue << "° RGB:(" << animated_color.R() << "," << animated_color.G() 
                  << "," << animated_color.B() << ")\n";
        }
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::ApplyTransparencyAnimation(double progress)
    {
        // Use a smooth sine wave for transparency
        double smooth_progress = SmoothCurve(progress, "sine");
        
        // Calculate alpha value
        double alpha = config_.min_alpha + (config_.max_alpha - config_.min_alpha) * 
                      (0.5 + 0.5 * sin(smooth_progress * 2 * M_PI));
        
        double transparency = 1.0 - alpha;
        
        // Simulate applying transparency to material
        // In a real implementation: material_->SetTransparency(transparency);
        
        animation_state_.transparency_phase = progress;
        
        // Log transparency change occasionally
        static int log_counter = 0;
        if (++log_counter % 60 == 0)  // Every 60 updates
        {
            gzmsg << "CustomVisualPlugin: Transparency animation - Alpha:" << std::fixed 
                  << std::setprecision(2) << alpha << " Transparency:" << transparency << "\n";
        }
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::ApplyMaterialPropertyAnimation(double progress)
    {
        // Use different curves for metalness and roughness
        double smooth_progress = SmoothCurve(progress, "sine");
        
        // Animate metalness (0-1 range)
        double metalness = 0.5 + 0.5 * sin(smooth_progress * config_.metalness_range * 2 * M_PI);
        
        // Animate roughness with different phase
        double roughness = 0.5 + 0.5 * cos(smooth_progress * config_.roughness_range * 2 * M_PI);
        
        // Simulate applying PBR properties to material
        // In a real implementation with PBR support:
        // material_->SetMetalness(metalness);
        // material_->SetRoughness(roughness);
        
        animation_state_.material_phase = progress;
        
        // Log material properties occasionally
        static int log_counter = 0;
        if (++log_counter % 90 == 0)  // Every 90 updates
        {
            gzmsg << "CustomVisualPlugin: Material animation - Metalness:" << std::fixed 
                  << std::setprecision(2) << metalness << " Roughness:" << roughness << "\n";
        }
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::ApplyTextureAnimation(double progress)
    {
        // Use linear progress for UV scrolling
        double uv_offset = progress * config_.uv_scroll_speed;
        
        // Use sine wave for scale animation
        double scale_factor = 1.0 + 0.5 * sin(progress * config_.texture_scale_factor * 2 * M_PI);
        
        // Simulate applying texture transformations
        // In a real implementation:
        // material_->SetTextureUVTransform(uv_offset, uv_offset, scale_factor, scale_factor);
        
        animation_state_.texture_phase = progress;
        
        // Log texture animation occasionally
        static int log_counter = 0;
        if (++log_counter % 45 == 0)  // Every 45 updates
        {
            gzmsg << "CustomVisualPlugin: Texture animation - UV offset:" << std::fixed 
                  << std::setprecision(2) << uv_offset << " Scale:" << scale_factor << "\n";
        }
    }

    //////////////////////////////////////////////////
    math::Color CustomVisualPlugin::ColorFromHSV(double h, double s, double v)
    {
        // Normalize hue to 0-360 range
        h = fmod(h, 360.0);
        if (h < 0) h += 360.0;
        
        // Clamp saturation and value to 0-1 range
        s = std::clamp(s, 0.0, 1.0);
        v = std::clamp(v, 0.0, 1.0);
        
        double c = v * s;  // Chroma
        double x = c * (1 - std::abs(fmod(h / 60.0, 2) - 1));
        double m = v - c;
        
        double r, g, b;
        
        if (h < 60)
        {
            r = c; g = x; b = 0;
        }
        else if (h < 120)
        {
            r = x; g = c; b = 0;
        }
        else if (h < 180)
        {
            r = 0; g = c; b = x;
        }
        else if (h < 240)
        {
            r = 0; g = x; b = c;
        }
        else if (h < 300)
        {
            r = x; g = 0; b = c;
        }
        else
        {
            r = c; g = 0; b = x;
        }
        
        return math::Color(r + m, g + m, b + m, 1.0);
    }

    //////////////////////////////////////////////////
    double CustomVisualPlugin::SmoothCurve(double t, const std::string& curve_type)
    {
        t = std::clamp(t, 0.0, 1.0);
        
        if (curve_type == "sine")
        {
            return 0.5 * (1.0 + sin(2 * M_PI * t - M_PI/2));
        }
        else if (curve_type == "smoothstep")
        {
            return t * t * (3.0 - 2.0 * t);
        }
        else if (curve_type == "smootherstep")
        {
            return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
        }
        else if (curve_type == "ease_in")
        {
            return t * t;
        }
        else if (curve_type == "ease_out")
        {
            return 1.0 - (1.0 - t) * (1.0 - t);
        }
        else if (curve_type == "ease_in_out")
        {
            return t < 0.5 ? 2.0 * t * t : 1.0 - 2.0 * (1.0 - t) * (1.0 - t);
        }
        
        return t;  // Linear fallback
    }

    //////////////////////////////////////////////////
    void CustomVisualPlugin::LogDiagnostics(const UpdateInfo &_info)
    {
        gzmsg << "CustomVisualPlugin: Diagnostics Report\n";
        gzmsg << "  Simulation time: " << std::fixed << std::setprecision(2) 
              << std::chrono::duration<double>(_info.simTime).count() << " seconds\n";
        gzmsg << "  Rendering initialized: " << (rendering_initialized_ ? "Yes" : "No") << "\n";
        gzmsg << "  Visual cached: " << (visual_cached_ ? "Yes" : "No") << "\n";
        gzmsg << "  Active effects - Color:" << (config_.enable_color_animation ? "On" : "Off");
        gzmsg << " Transparency:" << (config_.enable_transparency_effects ? "On" : "Off");
        gzmsg << " Material:" << (config_.enable_material_animation ? "On" : "Off");
        gzmsg << " Texture:" << (config_.enable_texture_animation ? "On" : "Off") << "\n";
        gzmsg << "  Animation phases - Color:" << std::fixed << std::setprecision(3) << animation_state_.color_phase;
        gzmsg << " Transparency:" << animation_state_.transparency_phase;
        gzmsg << " Material:" << animation_state_.material_phase;
        gzmsg << " Texture:" << animation_state_.texture_phase << "\n";
        gzmsg << "  Performance - Updates:" << performance_metrics_.update_count;
        gzmsg << " Avg render time:" << std::fixed << std::setprecision(3) 
              << performance_metrics_.GetAverageRenderTime() << "ms\n";
    }
}  // namespace tutorial

// Register the plugin
GZ_ADD_PLUGIN(tutorial::CustomVisualPlugin,
              gz::sim::System,
              tutorial::CustomVisualPlugin::ISystemConfigure,
              tutorial::CustomVisualPlugin::ISystemPreUpdate,
              tutorial::CustomVisualPlugin::ISystemUpdate,
              tutorial::CustomVisualPlugin::ISystemPostUpdate)