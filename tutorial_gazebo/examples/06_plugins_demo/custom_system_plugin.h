/*
 * Custom System Plugin Header
 * 
 * This header defines a custom system plugin that demonstrates:
 * - System plugin interface implementation
 * - Entity-Component-System integration
 * - Configuration parameter handling
 * - Performance-optimized update patterns
 */

#ifndef TUTORIAL_CUSTOM_SYSTEM_PLUGIN_H_
#define TUTORIAL_CUSTOM_SYSTEM_PLUGIN_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <gz/sim/System.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/Link.hh>
#include <gz/sim/Entity.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components.hh>

#include <gz/common/Console.hh>
#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>

#include <gz/msgs/vector3d.pb.h>
#include <gz/msgs/pose.pb.h>

#include <gz/transport/Node.hh>

#include <gz/plugin/Register.hh>

namespace tutorial
{
    /**
     * Custom System Plugin for Educational Demonstrations
     * 
     * This plugin demonstrates comprehensive system plugin development:
     * - Multi-interface implementation (Configure, PreUpdate, Update, PostUpdate)
     * - Entity tracking and component management
     * - Transport integration for external communication
     * - Performance monitoring and optimization
     * - Configurable behavior through SDF parameters
     */
    class CustomSystemPlugin :
        public gz::sim::System,
        public gz::sim::ISystemConfigure,
        public gz::sim::ISystemPreUpdate,
        public gz::sim::ISystemUpdate,
        public gz::sim::ISystemPostUpdate,
        public gz::sim::ISystemReset
    {
    public:
        /// Constructor
        CustomSystemPlugin();
        
        /// Destructor  
        ~CustomSystemPlugin() override;
        
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
        
        /// Called when simulation is reset
        /// \param[in] _info Update information
        /// \param[in] _ecm EntityComponentManager for accessing components
        void Reset(const gz::sim::UpdateInfo &_info,
                  gz::sim::EntityComponentManager &_ecm) override;

    private:
        /// Initialize plugin components and systems
        void Initialize();
        
        /// Process configuration parameters
        void ProcessConfiguration();
        
        /// Update entity tracking and component caching
        void UpdateEntityTracking(gz::sim::EntityComponentManager &_ecm);
        
        /// Apply forces and control logic
        void ApplyControlLogic(const gz::sim::UpdateInfo &_info,
                              gz::sim::EntityComponentManager &_ecm);
        
        /// Publish status and telemetry data
        void PublishTelemetry(const gz::sim::UpdateInfo &_info,
                             const gz::sim::EntityComponentManager &_ecm);
        
        /// Handle incoming command messages
        /// \param[in] _msg Command message
        void OnCommandMessage(const gz::msgs::Vector3d &_msg);
        
        /// Handle pose requests
        /// \param[in] _req Pose request message
        /// \param[out] _rep Pose response message
        bool OnPoseRequest(const gz::msgs::Vector3d &_req, gz::msgs::Pose &_rep);
        
        /// Calculate performance metrics
        void UpdatePerformanceMetrics(const gz::sim::UpdateInfo &_info);
        
        /// Log plugin status and diagnostics
        void LogDiagnostics();

    private:
        /// Plugin configuration parameters
        struct Configuration
        {
            std::string model_name{"custom_model"};
            std::string link_name{"link"};
            double update_rate{100.0};
            double force_magnitude{10.0};
            bool enable_control{true};
            bool enable_telemetry{true};
            std::string command_topic{"/custom_plugin/commands"};
            std::string telemetry_topic{"/custom_plugin/telemetry"};
            std::string pose_service{"/custom_plugin/pose"};
        } config_;
        
        /// Entity tracking
        gz::sim::Entity world_entity_{gz::sim::kNullEntity};
        gz::sim::Entity model_entity_{gz::sim::kNullEntity};  
        gz::sim::Entity link_entity_{gz::sim::kNullEntity};
        
        /// Component caching for performance
        struct ComponentCache
        {
            gz::sim::components::Pose* pose{nullptr};
            gz::sim::components::LinearVelocity* linear_velocity{nullptr};
            gz::sim::components::AngularVelocity* angular_velocity{nullptr};
            gz::sim::components::WorldPose* world_pose{nullptr};
            bool is_valid{false};
            
            void Invalidate() 
            {
                pose = nullptr;
                linear_velocity = nullptr;
                angular_velocity = nullptr;
                world_pose = nullptr;
                is_valid = false;
            }
        } component_cache_;
        
        /// Transport communication
        gz::transport::Node node_;
        gz::transport::Node::Publisher telemetry_pub_;
        
        /// Control state
        gz::math::Vector3d target_force_{gz::math::Vector3d::Zero};
        gz::math::Vector3d current_velocity_{gz::math::Vector3d::Zero};
        gz::math::Pose3d current_pose_{gz::math::Pose3d::Zero};
        
        /// Performance monitoring
        struct PerformanceMetrics
        {
            size_t update_count{0};
            double total_update_time{0.0};
            double max_update_time{0.0};
            double min_update_time{std::numeric_limits<double>::max()};
            std::chrono::steady_clock::time_point last_log_time;
            
            void Reset()
            {
                update_count = 0;
                total_update_time = 0.0;
                max_update_time = 0.0;
                min_update_time = std::numeric_limits<double>::max();
                last_log_time = std::chrono::steady_clock::now();
            }
            
            double GetAverageUpdateTime() const
            {
                return update_count > 0 ? total_update_time / update_count : 0.0;
            }
        } performance_metrics_;
        
        /// Timing and synchronization
        std::chrono::steady_clock::time_point last_update_time_;
        std::chrono::steady_clock::time_point start_time_;
        bool is_initialized_{false};
        
        /// Update rate control
        double update_period_{0.01}; // 100 Hz default
        std::chrono::steady_clock::time_point last_control_update_;
        std::chrono::steady_clock::time_point last_telemetry_update_;
    };
}  // namespace tutorial

#endif  // TUTORIAL_CUSTOM_SYSTEM_PLUGIN_H_