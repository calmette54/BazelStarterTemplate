/*
 * CustomViewer: 3D Scene Visualization for Gazebo Demos
 *
 * This class creates a 3D rendering window that connects to gz::sim::Server
 * via gz-transport to receive and render simulation scenes in real-time.
 *
 * Key features:
 * - Static Ogre2 engine integration (no BCR plugin loading)
 * - Direct gz-transport scene subscription
 * - Interactive 3D camera controls
 * - Material visualization support
 */

#pragma once

#include <gz/rendering.hh>
#include <gz/common/Console.hh>
#include <gz/transport/Node.hh>
#include <gz/msgs/scene.pb.h>

#include <memory>
#include <string>
#include <thread>
#include <atomic>

// Forward declarations for window management
struct GLFWwindow;

namespace gz
{
namespace tutorial
{

/**
 * @brief 3D Scene Viewer with static rendering engine integration
 *
 * This class creates a native 3D visualization window without relying on
 * BCR plugin loading. It uses static Ogre2 integration and gz-transport
 * to receive and render simulation scenes from gz::sim::Server.
 */
class CustomViewer
{
public:
    CustomViewer();
    ~CustomViewer();

    /**
     * @brief Initialize the 3D viewer with specified window dimensions
     * @param width Window width in pixels
     * @param height Window height in pixels
     * @param window_title Title for the 3D window
     * @return True if initialization succeeds
     */
    bool Initialize(int width, int height, const std::string& window_title = "Gazebo Tutorial Viewer");

    /**
     * @brief Start the rendering loop in a separate thread
     * @return True if rendering thread starts successfully
     */
    bool StartRenderingThread();

    /**
     * @brief Stop the rendering loop and cleanup
     */
    void Stop();

    /**
     * @brief Check if the viewer is running
     * @return True if the viewer is active
     */
    bool IsRunning() const { return running_ && render_thread_.joinable(); }

    /**
     * @brief Wait for the viewer to finish (blocking)
     */
    void Join();

private:
    /**
     * @brief Initialize static rendering engine (Ogre2)
     */
    bool InitializeRenderingEngine();

    /**
     * @brief Setup GLFW window and OpenGL context
     */
    bool SetupWindow();

    /**
     * @brief Create the 3D scene and camera
     */
    bool CreateScene();

    /**
     * @brief Setup lighting for material visualization
     */
    void SetupLighting();

    /**
     * @brief Main rendering loop (runs in separate thread)
     */
    void RenderLoop();

    /**
     * @brief Handle window events and input
     */
    void HandleInput();

    /**
     * @brief Scene update callback from gz-transport
     */
    void OnSceneUpdate(const gz::msgs::Scene& scene_msg);

    // Rendering components
    gz::rendering::RenderEnginePtr engine_;
    gz::rendering::ScenePtr scene_;
    gz::rendering::CameraPtr camera_;
    GLFWwindow* window_;

    // Transport for scene updates
    std::shared_ptr<gz::transport::Node> transport_node_;

    // Window properties
    int width_;
    int height_;
    std::string window_title_;

    // Threading
    std::thread render_thread_;
    std::atomic<bool> running_;

    // Camera control
    double camera_yaw_;
    double camera_pitch_;
    double camera_distance_;
    bool mouse_pressed_;
    double last_mouse_x_;
    double last_mouse_y_;
};

} // namespace tutorial
} // namespace gz
