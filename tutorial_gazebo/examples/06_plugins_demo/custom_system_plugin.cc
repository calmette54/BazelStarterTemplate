/*
 * Custom System Plugin Implementation
 * 
 * This implementation demonstrates a complete system plugin with:
 * - Full lifecycle management (Configure, Update phases, Reset)
 * - Entity-Component-System integration
 * - Transport communication (topics and services)
 * - Performance monitoring and optimization
 * - Configurable behavior through SDF parameters
 */

#include "custom_system_plugin.h"

#include <algorithm>
#include <chrono>
#include <functional>

#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/WorldPose.hh>
#include <gz/sim/components/LinearVelocity.hh>
#include <gz/sim/components/AngularVelocity.hh>
#include <gz/sim/components/ExternalWorldWrenchCmd.hh>

#include <gz/msgs/Utility.hh>

#include <sdf/Element.hh>

using namespace gz;
using namespace sim;

namespace tutorial
{
    //////////////////////////////////////////////////
    CustomSystemPlugin::CustomSystemPlugin()
    {
        gzmsg << "CustomSystemPlugin: Constructor called\n";
        start_time_ = std::chrono::steady_clock::now();
        performance_metrics_.Reset();
    }

    //////////////////////////////////////////////////
    CustomSystemPlugin::~CustomSystemPlugin()
    {
        gzmsg << "CustomSystemPlugin: Destructor called\n";
        
        // Log final performance statistics
        if (performance_metrics_.update_count > 0)
        {
            gzmsg << "CustomSystemPlugin: Final Performance Statistics:\n";
            gzmsg << "  Total updates: " << performance_metrics_.update_count << "\n";
            gzmsg << "  Average update time: " << std::fixed << std::setprecision(3) 
                  << performance_metrics_.GetAverageUpdateTime() << " ms\n";
            gzmsg << "  Min update time: " << performance_metrics_.min_update_time << " ms\n";
            gzmsg << "  Max update time: " << performance_metrics_.max_update_time << " ms\n";
        }
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::Configure(
        const Entity &_entity,
        const std::shared_ptr<const sdf::Element> &_sdf,
        EntityComponentManager &_ecm,
        EventManager &_eventMgr)
    {
        gzmsg << "CustomSystemPlugin: Configure called for entity " << _entity << "\n";
        
        // Store the entity (typically a model or world)
        if (_ecm.Component<components::Model>(_entity))
        {
            model_entity_ = _entity;
            gzmsg << "CustomSystemPlugin: Attached to model entity\n";
        }
        else if (_ecm.Component<components::World>(_entity))
        {
            world_entity_ = _entity;
            gzmsg << "CustomSystemPlugin: Attached to world entity\n";
        }
        
        // Parse configuration parameters
        if (_sdf->HasElement("model_name"))
        {
            config_.model_name = _sdf->Get<std::string>("model_name");
        }
        
        if (_sdf->HasElement("link_name"))
        {
            config_.link_name = _sdf->Get<std::string>("link_name");
        }
        
        if (_sdf->HasElement("update_rate"))
        {
            config_.update_rate = _sdf->Get<double>("update_rate");
            update_period_ = 1.0 / config_.update_rate;
        }
        
        if (_sdf->HasElement("force_magnitude"))
        {
            config_.force_magnitude = _sdf->Get<double>("force_magnitude");
        }
        
        if (_sdf->HasElement("enable_control"))
        {
            config_.enable_control = _sdf->Get<bool>("enable_control");
        }
        
        if (_sdf->HasElement("enable_telemetry"))
        {
            config_.enable_telemetry = _sdf->Get<bool>("enable_telemetry");
        }
        
        if (_sdf->HasElement("command_topic"))
        {
            config_.command_topic = _sdf->Get<std::string>("command_topic");
        }
        
        if (_sdf->HasElement("telemetry_topic"))
        {
            config_.telemetry_topic = _sdf->Get<std::string>("telemetry_topic");
        }
        
        if (_sdf->HasElement("pose_service"))
        {
            config_.pose_service = _sdf->Get<std::string>("pose_service");
        }
        
        ProcessConfiguration();
        
        gzmsg << "CustomSystemPlugin: Configuration completed\n";
        gzmsg << "  Model: " << config_.model_name << "\n";
        gzmsg << "  Link: " << config_.link_name << "\n";
        gzmsg << "  Update rate: " << config_.update_rate << " Hz\n";
        gzmsg << "  Force magnitude: " << config_.force_magnitude << " N\n";
        gzmsg << "  Control enabled: " << (config_.enable_control ? "true" : "false") << "\n";
        gzmsg << "  Telemetry enabled: " << (config_.enable_telemetry ? "true" : "false") << "\n";
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::ProcessConfiguration()
    {
        // Setup transport communication
        if (config_.enable_control)
        {
            node_.Subscribe(config_.command_topic, &CustomSystemPlugin::OnCommandMessage, this);
            gzmsg << "CustomSystemPlugin: Subscribed to commands on " << config_.command_topic << "\n";
        }
        
        if (config_.enable_telemetry)
        {
            telemetry_pub_ = node_.Advertise<msgs::Pose>(config_.telemetry_topic);
            gzmsg << "CustomSystemPlugin: Publishing telemetry on " << config_.telemetry_topic << "\n";
        }
        
        // Advertise pose service
        node_.Advertise(config_.pose_service, &CustomSystemPlugin::OnPoseRequest, this);
        gzmsg << "CustomSystemPlugin: Advertising pose service on " << config_.pose_service << "\n";
        
        // Initialize timing
        last_control_update_ = std::chrono::steady_clock::now();
        last_telemetry_update_ = std::chrono::steady_clock::now();
        last_update_time_ = std::chrono::steady_clock::now();
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::PreUpdate(
        const UpdateInfo &_info,
        EntityComponentManager &_ecm)
    {
        auto update_start = std::chrono::steady_clock::now();
        
        // Initialize on first update
        if (!is_initialized_)
        {
            Initialize();
            UpdateEntityTracking(_ecm);
            is_initialized_ = true;
        }
        
        // Update entity tracking if needed
        if (!component_cache_.is_valid)
        {
            UpdateEntityTracking(_ecm);
        }
        
        UpdatePerformanceMetrics(_info);
        
        auto update_end = std::chrono::steady_clock::now();
        double update_time = std::chrono::duration<double, std::milli>(update_end - update_start).count();
        performance_metrics_.total_update_time += update_time;
        performance_metrics_.min_update_time = std::min(performance_metrics_.min_update_time, update_time);
        performance_metrics_.max_update_time = std::max(performance_metrics_.max_update_time, update_time);
        performance_metrics_.update_count++;
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::Update(
        const UpdateInfo &_info,
        EntityComponentManager &_ecm)
    {
        if (!is_initialized_ || !component_cache_.is_valid)
            return;
            
        // Apply control logic at specified rate
        auto now = std::chrono::steady_clock::now();
        double time_since_control = std::chrono::duration<double>(now - last_control_update_).count();
        
        if (time_since_control >= update_period_)
        {
            if (config_.enable_control)
            {
                ApplyControlLogic(_info, _ecm);
            }
            last_control_update_ = now;
        }
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::PostUpdate(
        const UpdateInfo &_info,
        const EntityComponentManager &_ecm)
    {
        if (!is_initialized_ || !component_cache_.is_valid)
            return;
            
        // Publish telemetry at specified rate
        auto now = std::chrono::steady_clock::now();
        double time_since_telemetry = std::chrono::duration<double>(now - last_telemetry_update_).count();
        
        if (time_since_telemetry >= (1.0 / 10.0))  // 10 Hz telemetry
        {
            if (config_.enable_telemetry)
            {
                PublishTelemetry(_info, _ecm);
            }
            last_telemetry_update_ = now;
            
            // Log diagnostics periodically
            double time_since_log = std::chrono::duration<double>(now - performance_metrics_.last_log_time).count();
            if (time_since_log >= 5.0)  // Every 5 seconds
            {
                LogDiagnostics();
                performance_metrics_.last_log_time = now;
            }
        }
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::Reset(
        const UpdateInfo &_info,
        EntityComponentManager &_ecm)
    {
        gzmsg << "CustomSystemPlugin: Reset called\n";
        
        // Reset state
        target_force_ = math::Vector3d::Zero;
        current_velocity_ = math::Vector3d::Zero;
        current_pose_ = math::Pose3d::Zero;
        
        // Reset performance metrics
        performance_metrics_.Reset();
        
        // Invalidate component cache
        component_cache_.Invalidate();
        
        // Reset timing
        last_control_update_ = std::chrono::steady_clock::now();
        last_telemetry_update_ = std::chrono::steady_clock::now();
        last_update_time_ = std::chrono::steady_clock::now();
        
        gzmsg << "CustomSystemPlugin: Reset completed\n";
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::Initialize()
    {
        gzmsg << "CustomSystemPlugin: Initialize called\n";
        // Additional initialization if needed
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::UpdateEntityTracking(EntityComponentManager &_ecm)
    {
        // Find target model if we don't have it
        if (model_entity_ == kNullEntity)
        {
            auto models = _ecm.EntitiesByComponents(components::Model(), components::Name());
            for (auto model : models)
            {
                auto nameComp = _ecm.Component<components::Name>(model);
                if (nameComp && nameComp->Data() == config_.model_name)
                {
                    model_entity_ = model;
                    gzmsg << "CustomSystemPlugin: Found model '" << config_.model_name << "'\n";
                    break;
                }
            }
        }
        
        // Find target link if we have the model
        if (model_entity_ != kNullEntity && link_entity_ == kNullEntity)
        {
            auto links = _ecm.EntitiesByComponents(components::ParentEntity(model_entity_), 
                                                  components::Name());
            for (auto link : links)
            {
                auto nameComp = _ecm.Component<components::Name>(link);
                if (nameComp && nameComp->Data() == config_.link_name)
                {
                    link_entity_ = link;
                    gzmsg << "CustomSystemPlugin: Found link '" << config_.link_name << "'\n";
                    break;
                }
            }
        }
        
        // Cache components for performance
        if (link_entity_ != kNullEntity)
        {
            component_cache_.pose = _ecm.Component<components::Pose>(link_entity_);
            component_cache_.linear_velocity = _ecm.Component<components::LinearVelocity>(link_entity_);
            component_cache_.angular_velocity = _ecm.Component<components::AngularVelocity>(link_entity_);
            component_cache_.world_pose = _ecm.Component<components::WorldPose>(link_entity_);
            
            // Create components if they don't exist
            if (!component_cache_.linear_velocity)
            {
                _ecm.CreateComponent(link_entity_, components::LinearVelocity());
                component_cache_.linear_velocity = _ecm.Component<components::LinearVelocity>(link_entity_);
            }
            
            if (!component_cache_.angular_velocity)
            {
                _ecm.CreateComponent(link_entity_, components::AngularVelocity());
                component_cache_.angular_velocity = _ecm.Component<components::AngularVelocity>(link_entity_);
            }
            
            if (!component_cache_.world_pose)
            {
                _ecm.CreateComponent(link_entity_, components::WorldPose());
                component_cache_.world_pose = _ecm.Component<components::WorldPose>(link_entity_);
            }
            
            component_cache_.is_valid = true;
            gzmsg << "CustomSystemPlugin: Component cache initialized\n";
        }
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::ApplyControlLogic(
        const UpdateInfo &_info,
        EntityComponentManager &_ecm)
    {
        if (link_entity_ == kNullEntity || !component_cache_.is_valid)
            return;
            
        // Get current state
        if (component_cache_.world_pose)
        {
            current_pose_ = component_cache_.world_pose->Data();
        }
        
        if (component_cache_.linear_velocity)
        {
            current_velocity_ = component_cache_.linear_velocity->Data();
        }
        
        // Simple control logic - apply target force
        if (target_force_.Length() > 0.0)
        {
            // Create or update external wrench command
            auto wrenchCmd = _ecm.Component<components::ExternalWorldWrenchCmd>(link_entity_);
            if (!wrenchCmd)
            {
                _ecm.CreateComponent(link_entity_, components::ExternalWorldWrenchCmd());
                wrenchCmd = _ecm.Component<components::ExternalWorldWrenchCmd>(link_entity_);
            }
            
            if (wrenchCmd)
            {
                msgs::EntityWrench wrench_msg;
                wrench_msg.set_id(link_entity_);
                msgs::Set(wrench_msg.mutable_wrench()->mutable_force(), target_force_);
                msgs::Set(wrench_msg.mutable_wrench()->mutable_torque(), math::Vector3d::Zero);
                
                wrenchCmd->Data().push_back(wrench_msg);
            }
        }
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::PublishTelemetry(
        const UpdateInfo &_info,
        const EntityComponentManager &_ecm)
    {
        if (!telemetry_pub_.Valid())
            return;
            
        msgs::Pose pose_msg;
        msgs::Set(&pose_msg, current_pose_);
        
        // Add timestamp
        auto stamp = pose_msg.mutable_header()->mutable_stamp();
        auto time_point = std::chrono::duration_cast<std::chrono::nanoseconds>(_info.simTime).count();
        stamp->set_sec(time_point / 1000000000);
        stamp->set_nsec(time_point % 1000000000);
        
        telemetry_pub_.Publish(pose_msg);
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::OnCommandMessage(const msgs::Vector3d &_msg)
    {
        target_force_ = msgs::Convert(_msg);
        
        // Limit force magnitude
        if (target_force_.Length() > config_.force_magnitude)
        {
            target_force_ = target_force_.Normalized() * config_.force_magnitude;
        }
        
        gzmsg << "CustomSystemPlugin: Received command force: " 
              << target_force_.X() << ", " << target_force_.Y() << ", " << target_force_.Z() << "\n";
    }

    //////////////////////////////////////////////////
    bool CustomSystemPlugin::OnPoseRequest(const msgs::Vector3d &_req, msgs::Pose &_rep)
    {
        msgs::Set(&_rep, current_pose_);
        
        gzmsg << "CustomSystemPlugin: Pose request served\n";
        return true;
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::UpdatePerformanceMetrics(const UpdateInfo &_info)
    {
        auto now = std::chrono::steady_clock::now();
        last_update_time_ = now;
    }

    //////////////////////////////////////////////////
    void CustomSystemPlugin::LogDiagnostics()
    {
        gzmsg << "CustomSystemPlugin: Diagnostics Report\n";
        gzmsg << "  Entity status - Model: " << (model_entity_ != kNullEntity ? "Found" : "Not found");
        gzmsg << ", Link: " << (link_entity_ != kNullEntity ? "Found" : "Not found") << "\n";
        gzmsg << "  Component cache: " << (component_cache_.is_valid ? "Valid" : "Invalid") << "\n";
        gzmsg << "  Current pose: " << current_pose_ << "\n";
        gzmsg << "  Current velocity: " << current_velocity_ << "\n";
        gzmsg << "  Target force: " << target_force_ << "\n";
        gzmsg << "  Performance - Updates: " << performance_metrics_.update_count;
        gzmsg << ", Avg time: " << std::fixed << std::setprecision(3) 
              << performance_metrics_.GetAverageUpdateTime() << " ms\n";
    }
}  // namespace tutorial

// Register the plugin
GZ_ADD_PLUGIN(tutorial::CustomSystemPlugin,
              gz::sim::System,
              tutorial::CustomSystemPlugin::ISystemConfigure,
              tutorial::CustomSystemPlugin::ISystemPreUpdate,
              tutorial::CustomSystemPlugin::ISystemUpdate,
              tutorial::CustomSystemPlugin::ISystemPostUpdate,
              tutorial::CustomSystemPlugin::ISystemReset)