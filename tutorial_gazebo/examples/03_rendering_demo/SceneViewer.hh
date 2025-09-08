/*
 * SceneViewer: Custom 3D Scene Visualization using gz-rendering + GLFW
 *
 * This class provides a lightweight alternative to gz-gui for 3D visualization.
 * It creates an OpenGL window using GLFW and renders 3D scenes using gz-rendering,
 * providing camera navigation and real-time 3D visualization without the complexity
 * of the full gz-gui framework.
 *
 * Features:
 * - GLFW window management with OpenGL context
 * - gz-rendering scene management and camera control
 * - Mouse look camera (FPS-style navigation)
 * - WASD movement controls
 * - Real-time rendering loop
 * - SDF world loading and visualization
 * - Hermetic build compatibility (no external GUI dependencies)
 */

#ifndef TUTORIAL_GAZEBO_SCENE_VIEWER_HH_
#define TUTORIAL_GAZEBO_SCENE_VIEWER_HH_

#include <GLFW/glfw3.h>
#include <gz/rendering/RenderEngine.hh>
#include <gz/rendering/Scene.hh>
#include <gz/rendering/Camera.hh>
#include <gz/rendering/Visual.hh>
#include <gz/common/Console.hh>
#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>

#include <string>
#include <memory>
#include <thread>
#include <atomic>

class SceneViewer
{
public:
    /// \brief Constructor
    SceneViewer();

    /// \brief Destructor
    ~SceneViewer();

    /// \brief Initialize the viewer with window dimensions and title
    /// \param[in] width Window width
    /// \param[in] height Window height
    /// \param[in] title Window title
    /// \return True if initialization succeeded
    bool Initialize(int width = 1024, int height = 768, const std::string& title = "Gazebo Scene Viewer");

    /// \brief Load an SDF world file for visualization
    /// \param[in] sdf_file Path to SDF file
    /// \return True if world loaded successfully
    bool LoadSdfWorld(const std::string& sdf_file);

    /// \brief Create a basic demo scene programmatically
    /// \param[in] scene_type Type of demo scene to create
    void CreateDemoScene(const std::string& scene_type = "basic");

    /// \brief Run the main rendering loop
    /// This function blocks until the window is closed
    void Run();

    /// \brief Check if the viewer should continue running
    /// \return True if window is still open
    bool IsRunning() const;

    /// \brief Set camera position and orientation
    /// \param[in] pos Camera position
    /// \param[in] target Look-at target point
    void SetCameraPose(const gz::math::Vector3d& pos, const gz::math::Vector3d& target);

    /// \brief Get the rendering scene
    /// \return Pointer to rendering scene
    gz::rendering::ScenePtr GetScene() { return scene_; }

    /// \brief Get the camera
    /// \return Pointer to camera
    gz::rendering::CameraPtr GetCamera() { return camera_; }

private:
    /// \brief Initialize GLFW and create window
    bool InitializeWindow(int width, int height, const std::string& title);

    /// \brief Initialize gz-rendering engine and scene
    bool InitializeRendering();

    /// \brief Setup basic lighting for the scene
    void SetupLighting();

    /// \brief Update camera based on input
    /// \param[in] deltaTime Time since last update
    void UpdateCamera(double deltaTime);

    /// \brief Process keyboard input
    /// \param[in] deltaTime Time since last update
    void ProcessInput(double deltaTime);

    /// \brief Render one frame
    void RenderFrame();

    /// \brief Cleanup resources
    void Cleanup();

    // GLFW callbacks (static functions)
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void WindowSizeCallback(GLFWwindow* window, int width, int height);

    // Window and rendering
    GLFWwindow* window_ = nullptr;
    int window_width_ = 1024;
    int window_height_ = 768;

    // Rendering components
    gz::rendering::RenderEnginePtr engine_ = nullptr;
    gz::rendering::ScenePtr scene_ = nullptr;
    gz::rendering::CameraPtr camera_ = nullptr;

    // Camera control
    gz::math::Vector3d camera_pos_{-6, 6, 4};
    gz::math::Vector3d camera_target_{0, 0, 0};
    double camera_yaw_ = -45.0;      // Horizontal rotation (degrees)
    double camera_pitch_ = -15.0;    // Vertical rotation (degrees)
    double camera_distance_ = 10.0;  // Distance from target

    // Input state
    bool keys_[1024] = {false};
    bool mouse_captured_ = false;
    double last_mouse_x_ = 0.0;
    double last_mouse_y_ = 0.0;
    bool first_mouse_ = true;

    // Movement settings
    double movement_speed_ = 5.0;
    double mouse_sensitivity_ = 0.1;
    double scroll_sensitivity_ = 1.0;

    // Timing
    double last_frame_time_ = 0.0;
};

#endif // TUTORIAL_GAZEBO_SCENE_VIEWER_HH_
