/*
 * SceneViewer: Implementation of custom 3D visualization using gz-rendering + GLFW
 */

#include "SceneViewer.hh"

#include <gz/rendering/RenderingIface.hh>
#include <gz/rendering/Material.hh>
#include <gz/rendering/Light.hh>
#include <gz/rendering/Geometry.hh>
#include <gz/rendering/Mesh.hh>
#include <gz/math/Angle.hh>

#include <iostream>
#include <cmath>

//////////////////////////////////////////////////
SceneViewer::SceneViewer()
{
    // Initialize timing
    last_frame_time_ = glfwGetTime();
}

//////////////////////////////////////////////////
SceneViewer::~SceneViewer()
{
    Cleanup();
}

//////////////////////////////////////////////////
bool SceneViewer::Initialize(int width, int height, const std::string& title)
{
    window_width_ = width;
    window_height_ = height;

    if (!InitializeWindow(width, height, title))
    {
        gzerr << "Failed to initialize GLFW window" << std::endl;
        return false;
    }

    if (!InitializeRendering())
    {
        gzerr << "Failed to initialize gz-rendering" << std::endl;
        return false;
    }

    SetupLighting();

    gzmsg << "SceneViewer initialized successfully" << std::endl;
    gzmsg << "Controls:" << std::endl;
    gzmsg << "  - Mouse: Look around (right-click and drag)" << std::endl;
    gzmsg << "  - WASD: Move camera" << std::endl;
    gzmsg << "  - Mouse wheel: Zoom in/out" << std::endl;
    gzmsg << "  - ESC: Exit" << std::endl;

    return true;
}

//////////////////////////////////////////////////
bool SceneViewer::InitializeWindow(int width, int height, const std::string& title)
{
    // Initialize GLFW
    if (!glfwInit())
    {
        gzerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA

    // Create window
    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window_)
    {
        gzerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Make OpenGL context current
    glfwMakeContextCurrent(window_);

    // Set user pointer for callbacks
    glfwSetWindowUserPointer(window_, this);

    // Set callbacks
    glfwSetKeyCallback(window_, KeyCallback);
    glfwSetMouseButtonCallback(window_, MouseButtonCallback);
    glfwSetCursorPosCallback(window_, CursorPositionCallback);
    glfwSetScrollCallback(window_, ScrollCallback);
    glfwSetWindowSizeCallback(window_, WindowSizeCallback);

    // Enable V-sync
    glfwSwapInterval(1);

    return true;
}

//////////////////////////////////////////////////
bool SceneViewer::InitializeRendering()
{
    // Get rendering engine (prefer ogre2)
    std::vector<std::string> engines = {"ogre2", "ogre", "optix"};

    for (const auto& engineName : engines)
    {
        engine_ = gz::rendering::engine(engineName);
        if (engine_)
        {
            gzmsg << "Using rendering engine: " << engineName << std::endl;
            break;
        }
    }

    if (!engine_)
    {
        gzerr << "Failed to load any rendering engine" << std::endl;
        return false;
    }

    // Create scene
    scene_ = engine_->CreateScene("scene_viewer");
    if (!scene_)
    {
        gzerr << "Failed to create rendering scene" << std::endl;
        return false;
    }

    // Set ambient light
    scene_->SetAmbientLight(0.2, 0.2, 0.2);
    scene_->SetBackgroundColor(0.05, 0.05, 0.1);

    // Create camera
    camera_ = scene_->CreateCamera("camera");
    if (!camera_)
    {
        gzerr << "Failed to create camera" << std::endl;
        return false;
    }

    // Configure camera
    camera_->SetImageWidth(window_width_);
    camera_->SetImageHeight(window_height_);
    camera_->SetHFOV(gz::math::Angle(1.047)); // 60 degrees
    camera_->SetNearClipPlane(0.1);
    camera_->SetFarClipPlane(1000.0);

    // Set initial camera position
    SetCameraPose(camera_pos_, camera_target_);

    return true;
}

//////////////////////////////////////////////////
void SceneViewer::SetupLighting()
{
    // Key light - primary illumination
    auto keyLight = scene_->CreateDirectionalLight();
    keyLight->SetName("key_light");
    keyLight->SetDirection(0.3, 0.5, -1.0);
    keyLight->SetDiffuseColor(0.8, 0.8, 0.7);
    keyLight->SetSpecularColor(0.6, 0.6, 0.5);
    keyLight->SetCastShadows(true);
    scene_->RootVisual()->AddChild(keyLight);

    // Fill light - soften shadows
    auto fillLight = scene_->CreateDirectionalLight();
    fillLight->SetName("fill_light");
    fillLight->SetDirection(-0.3, 0.2, -0.8);
    fillLight->SetDiffuseColor(0.3, 0.3, 0.4);
    fillLight->SetSpecularColor(0.1, 0.1, 0.2);
    fillLight->SetCastShadows(false);
    scene_->RootVisual()->AddChild(fillLight);

    // Rim light - edge definition
    auto rimLight = scene_->CreateDirectionalLight();
    rimLight->SetName("rim_light");
    rimLight->SetDirection(0.0, -1.0, 0.5);
    rimLight->SetDiffuseColor(0.4, 0.3, 0.2);
    rimLight->SetSpecularColor(0.6, 0.5, 0.4);
    rimLight->SetCastShadows(false);
    scene_->RootVisual()->AddChild(rimLight);
}

//////////////////////////////////////////////////
bool SceneViewer::LoadSdfWorld(const std::string& sdf_file)
{
    // TODO: Implement SDF world loading
    // This would parse the SDF file and create corresponding rendering objects
    gzmsg << "SDF world loading not yet implemented. Using demo scene." << std::endl;

    CreateDemoScene("basic");
    return true;
}

//////////////////////////////////////////////////
void SceneViewer::CreateDemoScene(const std::string& scene_type)
{
    // Create ground plane
    auto plane = scene_->CreateMesh("ground_plane");
    if (plane)
    {
        plane->SetMesh("unit_plane");
        auto planeVis = scene_->CreateVisual("ground");
        planeVis->SetLocalScale(20, 20, 1);
        planeVis->SetLocalPosition(0, 0, 0);
        planeVis->AddGeometry(plane);

        // Ground material
        auto planeMat = scene_->CreateMaterial();
        planeMat->SetAmbient(0.3, 0.3, 0.3);
        planeMat->SetDiffuse(0.6, 0.6, 0.6);
        planeMat->SetSpecular(0.1, 0.1, 0.1);
        planeMat->SetRoughness(0.8f);
        planeMat->SetMetalness(0.0f);
        plane->SetMaterial(planeMat);

        scene_->RootVisual()->AddChild(planeVis);
    }

    if (scene_type == "material_showcase" || scene_type == "basic")
    {
        // Create test objects with different materials

        // Metallic sphere
        auto sphere = scene_->CreateMesh("metal_sphere");
        if (sphere)
        {
            sphere->SetMesh("unit_sphere");
            auto sphereVis = scene_->CreateVisual("metal_sphere_vis");
            sphereVis->SetLocalPosition(0, 0, 1.5);
            sphereVis->AddGeometry(sphere);

            auto metalMat = scene_->CreateMaterial();
            metalMat->SetAmbient(0.3, 0.3, 0.3);
            metalMat->SetDiffuse(0.7, 0.7, 0.7);
            metalMat->SetSpecular(0.9, 0.9, 0.9);
            metalMat->SetRoughness(0.1f);
            metalMat->SetMetalness(0.9f);
            sphere->SetMaterial(metalMat);

            scene_->RootVisual()->AddChild(sphereVis);
        }

        // Plastic cube
        auto cube = scene_->CreateMesh("plastic_cube");
        if (cube)
        {
            cube->SetMesh("unit_box");
            auto cubeVis = scene_->CreateVisual("plastic_cube_vis");
            cubeVis->SetLocalPosition(-3, 0, 1);
            cubeVis->AddGeometry(cube);

            auto plasticMat = scene_->CreateMaterial();
            plasticMat->SetAmbient(0.1, 0.3, 0.1);
            plasticMat->SetDiffuse(0.2, 0.8, 0.2);
            plasticMat->SetSpecular(0.3, 0.3, 0.3);
            plasticMat->SetRoughness(0.6f);
            plasticMat->SetMetalness(0.0f);
            cube->SetMaterial(plasticMat);

            scene_->RootVisual()->AddChild(cubeVis);
        }

        // Glossy cylinder
        auto cylinder = scene_->CreateMesh("glossy_cylinder");
        if (cylinder)
        {
            cylinder->SetMesh("unit_cylinder");
            auto cylinderVis = scene_->CreateVisual("glossy_cylinder_vis");
            cylinderVis->SetLocalPosition(3, 0, 1);
            cylinderVis->SetLocalScale(1, 1, 2);
            cylinderVis->AddGeometry(cylinder);

            auto glossyMat = scene_->CreateMaterial();
            glossyMat->SetAmbient(0.1, 0.1, 0.3);
            glossyMat->SetDiffuse(0.2, 0.2, 0.8);
            glossyMat->SetSpecular(0.8, 0.8, 0.8);
            glossyMat->SetRoughness(0.2f);
            glossyMat->SetMetalness(0.1f);
            cylinder->SetMaterial(glossyMat);

            scene_->RootVisual()->AddChild(cylinderVis);
        }
    }

    gzmsg << "Demo scene '" << scene_type << "' created" << std::endl;
}

//////////////////////////////////////////////////
void SceneViewer::SetCameraPose(const gz::math::Vector3d& pos, const gz::math::Vector3d& target)
{
    camera_pos_ = pos;
    camera_target_ = target;

    if (camera_)
    {
        camera_->SetWorldPose(gz::math::Pose3d(pos, gz::math::Quaterniond()));
        camera_->SetWorldPosition(pos);

        // Calculate look direction
        auto direction = (target - pos).Normalized();
        camera_->SetWorldRotation(gz::math::Quaterniond());

        // For now, just set position. Full look-at will be implemented in UpdateCamera
        camera_->SetWorldPosition(pos);
    }
}

//////////////////////////////////////////////////
void SceneViewer::Run()
{
    gzmsg << "Starting render loop..." << std::endl;

    while (!glfwWindowShouldClose(window_))
    {
        double current_time = glfwGetTime();
        double delta_time = current_time - last_frame_time_;
        last_frame_time_ = current_time;

        // Process input
        ProcessInput(delta_time);

        // Update camera
        UpdateCamera(delta_time);

        // Render frame
        RenderFrame();

        // Swap buffers and poll events
        glfwSwapBuffers(window_);
        glfwPollEvents();
    }

    gzmsg << "Render loop ended" << std::endl;
}

//////////////////////////////////////////////////
bool SceneViewer::IsRunning() const
{
    return window_ && !glfwWindowShouldClose(window_);
}

//////////////////////////////////////////////////
void SceneViewer::ProcessInput(double deltaTime)
{
    if (keys_[GLFW_KEY_ESCAPE])
    {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }

    // Camera movement
    double speed = movement_speed_ * deltaTime;

    if (keys_[GLFW_KEY_W])
    {
        camera_distance_ -= speed;
        if (camera_distance_ < 1.0) camera_distance_ = 1.0;
    }
    if (keys_[GLFW_KEY_S])
    {
        camera_distance_ += speed;
        if (camera_distance_ > 50.0) camera_distance_ = 50.0;
    }

    // Strafe movements
    if (keys_[GLFW_KEY_A])
    {
        camera_yaw_ -= 30.0 * deltaTime;
    }
    if (keys_[GLFW_KEY_D])
    {
        camera_yaw_ += 30.0 * deltaTime;
    }

    if (keys_[GLFW_KEY_Q])
    {
        camera_pitch_ -= 30.0 * deltaTime;
    }
    if (keys_[GLFW_KEY_E])
    {
        camera_pitch_ += 30.0 * deltaTime;
    }

    // Clamp pitch
    if (camera_pitch_ > 89.0) camera_pitch_ = 89.0;
    if (camera_pitch_ < -89.0) camera_pitch_ = -89.0;
}

//////////////////////////////////////////////////
void SceneViewer::UpdateCamera(double deltaTime)
{
    // Convert spherical coordinates to cartesian
    double pitch_rad = gz::math::Angle::DegToRad(camera_pitch_);
    double yaw_rad = gz::math::Angle::DegToRad(camera_yaw_);

    gz::math::Vector3d offset;
    offset.X() = camera_distance_ * cos(pitch_rad) * cos(yaw_rad);
    offset.Y() = camera_distance_ * cos(pitch_rad) * sin(yaw_rad);
    offset.Z() = camera_distance_ * sin(pitch_rad);

    camera_pos_ = camera_target_ + offset;

    if (camera_)
    {
        camera_->SetWorldPosition(camera_pos_);

        // Look at target
        gz::math::Vector3d direction = (camera_target_ - camera_pos_).Normalized();
        gz::math::Vector3d up(0, 0, 1);
        gz::math::Vector3d right = direction.Cross(up).Normalized();
        up = right.Cross(direction).Normalized();

        // Create rotation matrix and convert to quaternion
        gz::math::Matrix3d rotMat;
        rotMat.Set(right.X(), up.X(), -direction.X(),
                   right.Y(), up.Y(), -direction.Y(),
                   right.Z(), up.Z(), -direction.Z());

        camera_->SetWorldRotation(gz::math::Quaterniond(rotMat));
    }
}

//////////////////////////////////////////////////
void SceneViewer::RenderFrame()
{
    if (camera_)
    {
        camera_->Capture();

        // Get the image and display it
        auto image = camera_->CreateImage();
        if (image && image->Width() > 0 && image->Height() > 0)
        {
            // For now, just clear the OpenGL buffer
            glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // TODO: Implement proper image display using OpenGL texture
            // This would involve converting the gz-rendering image to OpenGL texture
        }
    }
}

//////////////////////////////////////////////////
void SceneViewer::Cleanup()
{
    if (scene_)
    {
        scene_->DestroySensor(camera_);
        engine_->DestroyScene(scene_);
    }

    if (window_)
    {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }
}

//////////////////////////////////////////////////
// Static GLFW callbacks
//////////////////////////////////////////////////

void SceneViewer::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    SceneViewer* viewer = static_cast<SceneViewer*>(glfwGetWindowUserPointer(window));
    if (!viewer) return;

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            viewer->keys_[key] = true;
        else if (action == GLFW_RELEASE)
            viewer->keys_[key] = false;
    }
}

//////////////////////////////////////////////////
void SceneViewer::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    SceneViewer* viewer = static_cast<SceneViewer*>(glfwGetWindowUserPointer(window));
    if (!viewer) return;

    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            viewer->mouse_captured_ = true;
            viewer->first_mouse_ = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if (action == GLFW_RELEASE)
        {
            viewer->mouse_captured_ = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

//////////////////////////////////////////////////
void SceneViewer::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    SceneViewer* viewer = static_cast<SceneViewer*>(glfwGetWindowUserPointer(window));
    if (!viewer || !viewer->mouse_captured_) return;

    if (viewer->first_mouse_)
    {
        viewer->last_mouse_x_ = xpos;
        viewer->last_mouse_y_ = ypos;
        viewer->first_mouse_ = false;
    }

    double xoffset = xpos - viewer->last_mouse_x_;
    double yoffset = viewer->last_mouse_y_ - ypos; // Reversed since y-coordinates go from bottom to top

    viewer->last_mouse_x_ = xpos;
    viewer->last_mouse_y_ = ypos;

    viewer->camera_yaw_ += xoffset * viewer->mouse_sensitivity_;
    viewer->camera_pitch_ += yoffset * viewer->mouse_sensitivity_;

    // Clamp pitch
    if (viewer->camera_pitch_ > 89.0) viewer->camera_pitch_ = 89.0;
    if (viewer->camera_pitch_ < -89.0) viewer->camera_pitch_ = -89.0;
}

//////////////////////////////////////////////////
void SceneViewer::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    SceneViewer* viewer = static_cast<SceneViewer*>(glfwGetWindowUserPointer(window));
    if (!viewer) return;

    viewer->camera_distance_ -= yoffset * viewer->scroll_sensitivity_;
    if (viewer->camera_distance_ < 1.0) viewer->camera_distance_ = 1.0;
    if (viewer->camera_distance_ > 50.0) viewer->camera_distance_ = 50.0;
}

//////////////////////////////////////////////////
void SceneViewer::WindowSizeCallback(GLFWwindow* window, int width, int height)
{
    SceneViewer* viewer = static_cast<SceneViewer*>(glfwGetWindowUserPointer(window));
    if (!viewer) return;

    viewer->window_width_ = width;
    viewer->window_height_ = height;

    if (viewer->camera_)
    {
        viewer->camera_->SetImageWidth(width);
        viewer->camera_->SetImageHeight(height);
    }

    glViewport(0, 0, width, height);
}
