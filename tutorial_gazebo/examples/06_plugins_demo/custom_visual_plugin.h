/*
 * Custom Visual Plugin Header
 * 
 * This header defines a custom visual plugin that demonstrates:
 * - Visual plugin interface implementation
 * - Rendering system integration and material manipulation
 * - Dynamic visual effects and animations
 * - Performance-optimized rendering updates
 */

#ifndef TUTORIAL_CUSTOM_VISUAL_PLUGIN_H_
#define TUTORIAL_CUSTOM_VISUAL_PLUGIN_H_

#include <memory>
#include <string>
#include <vector>

#include <gz/sim/System.hh>
#include <gz/sim/Entity.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components.hh>

#include <gz/rendering/RenderEngine.hh>
#include <gz/rendering/Scene.hh>
#include <gz/rendering/Visual.hh>
#include <gz/rendering/Material.hh>
#include <gz/rendering/Geometry.hh>

#include <gz/common/Console.hh>
#include <gz/math/Vector3.hh>
#include <gz/math/Color.hh>

#include <gz/plugin/Register.hh>

namespace tutorial
{
    /**
     * Custom Visual Plugin for Rendering Effects
     * 
     * This plugin demonstrates comprehensive visual plugin development:
     * - Material property manipulation and animation
     * - Dynamic color and transparency effects
     * - Texture coordinate transformation
     * - Lighting interaction and visual feedback
     * - Performance-optimized rendering updates
     */
    class CustomVisualPlugin :
        public gz::sim::System,
        public gz::sim::ISystemConfigure,
        public gz::sim::ISystemPreUpdate,
        public gz::sim::ISystemUpdate,
        public gz::sim::ISystemPostUpdate
    {
    public:
        /// Constructor
        CustomVisualPlugin();
        
        /// Destructor
        ~CustomVisualPlugin() override;
        
        // System interface implementations
        
        /// Configure the plugin with SDF parameters
        /// \param[in] _entity Entity associated with this plugin
        /// \param[in] _sdf SDF element containing configuration parameters
        /// \param[in] _ecm EntityComponentManager for accessing components
        /// \param[in] _eventMgr EventManager for handling simulation events
        void Configure(const gz::sim::Entity &_entity,
                      const std::shared_ptr<const sdf::Element> &_sdf,
                      gz::sim::EntityComponentManager &_ecm,
                      gz::sim::EventManager &_eventMgr) override;
        
        /// Called before physics update
        /// \param[in] _info Update information
        /// \param[in] _ecm EntityComponentManager for accessing components
        void PreUpdate(const gz::sim::UpdateInfo &_info,
                      gz::sim::EntityComponentManager &_ecm) override;
        
        /// Called during physics update
        /// \param[in] _info Update information
        /// \param[in] _ecm EntityComponentManager for accessing components
        void Update(const gz::sim::UpdateInfo &_info,
                   gz::sim::EntityComponentManager &_ecm) override;
        
        /// Called after physics update
        /// \param[in] _info Update information
        /// \param[in] _ecm EntityComponentManager for accessing components
        void PostUpdate(const gz::sim::UpdateInfo &_info,
                       const gz::sim::EntityComponentManager &_ecm) override;

    private:
        /// Initialize rendering system connections
        void InitializeRendering();
        
        /// Find and cache visual components
        void CacheVisualComponents(gz::sim::EntityComponentManager &_ecm);
        
        /// Update visual effects based on time and parameters
        void UpdateVisualEffects(const gz::sim::UpdateInfo &_info);
        
        /// Apply color animation effects
        void ApplyColorAnimation(double progress);
        
        /// Apply transparency animation effects
        void ApplyTransparencyAnimation(double progress);
        
        /// Apply material property animations
        void ApplyMaterialPropertyAnimation(double progress);
        
        /// Apply texture coordinate transformations
        void ApplyTextureAnimation(double progress);
        
        /// Convert HSV color to RGB
        /// \param[in] h Hue (0-360)
        /// \param[in] s Saturation (0-1)
        /// \param[in] v Value (0-1)
        /// \return RGB color
        gz::math::Color ColorFromHSV(double h, double s, double v);
        
        /// Calculate smooth animation curves
        /// \param[in] t Time parameter (0-1)
        /// \param[in] curve_type Animation curve type
        /// \return Smoothed value (0-1)
        double SmoothCurve(double t, const std::string& curve_type = "sine");
        
        /// Log visual plugin diagnostics
        void LogDiagnostics(const gz::sim::UpdateInfo &_info);

    private:
        /// Plugin configuration
        struct Configuration
        {
            std::string effect_type{"color_animation"};
            double animation_speed{1.0};
            double animation_intensity{1.0};
            bool enable_color_animation{true};
            bool enable_transparency_effects{false};
            bool enable_material_animation{false};
            bool enable_texture_animation{false};
            
            // Color animation parameters
            double color_cycle_duration{5.0};
            gz::math::Color base_color{1.0, 0.0, 0.0, 1.0};
            
            // Transparency parameters
            double min_alpha{0.2};
            double max_alpha{1.0};
            double transparency_cycle_duration{3.0};
            
            // Material parameters
            double metalness_range{0.8};
            double roughness_range{0.6};
            double material_cycle_duration{7.0};
            
            // Texture parameters
            double uv_scroll_speed{0.5};
            double texture_scale_factor{2.0};
            double texture_cycle_duration{4.0};
        } config_;
        
        /// Rendering system components
        gz::rendering::RenderEngine* render_engine_{nullptr};
        gz::rendering::ScenePtr scene_{nullptr};
        gz::rendering::VisualPtr visual_{nullptr};
        gz::rendering::MaterialPtr material_{nullptr};
        gz::rendering::GeometryPtr geometry_{nullptr};
        
        /// Entity tracking
        gz::sim::Entity visual_entity_{gz::sim::kNullEntity};
        
        /// Animation state
        struct AnimationState
        {
            double start_time{0.0};
            double current_time{0.0};
            double last_update_time{0.0};
            
            // Effect-specific state
            double color_phase{0.0};
            double transparency_phase{0.0};
            double material_phase{0.0};
            double texture_phase{0.0};
            
            // Original material properties for restoration
            gz::math::Color original_ambient{0.5, 0.5, 0.5, 1.0};
            gz::math::Color original_diffuse{0.8, 0.8, 0.8, 1.0};
            gz::math::Color original_specular{0.2, 0.2, 0.2, 1.0};
            gz::math::Color original_emissive{0.0, 0.0, 0.0, 1.0};
            double original_transparency{0.0};
            double original_metalness{0.0};
            double original_roughness{0.5};
            
            bool properties_cached{false};
        } animation_state_;
        
        /// Performance monitoring
        struct PerformanceMetrics
        {
            size_t update_count{0};
            double total_render_time{0.0};
            double max_render_time{0.0};
            double min_render_time{std::numeric_limits<double>::max()};
            std::chrono::steady_clock::time_point last_log_time;
            
            void Reset()
            {
                update_count = 0;
                total_render_time = 0.0;
                max_render_time = 0.0;
                min_render_time = std::numeric_limits<double>::max();
                last_log_time = std::chrono::steady_clock::now();
            }
            
            double GetAverageRenderTime() const
            {
                return update_count > 0 ? total_render_time / update_count : 0.0;
            }
        } performance_metrics_;
        
        /// Update rate control
        double update_period_{0.033};  // 30 Hz default for visual updates
        std::chrono::steady_clock::time_point last_visual_update_;
        
        /// Initialization flags
        bool is_initialized_{false};
        bool rendering_initialized_{false};
        bool visual_cached_{false};
    };
}  // namespace tutorial

#endif  // TUTORIAL_CUSTOM_VISUAL_PLUGIN_H_