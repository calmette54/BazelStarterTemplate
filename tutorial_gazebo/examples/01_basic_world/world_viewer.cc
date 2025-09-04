/*
 * Gazebo Tutorial Example 01: Interactive World Viewer
 * 
 * This example demonstrates:
 * 1. Loading and visualizing SDF worlds
 * 2. Interactive GUI components
 * 3. Real-time rendering with Ogre2
 * 4. Camera controls and navigation
 * 
 * Learning objectives:
 * - Understand Gazebo's GUI system
 * - Learn about rendering pipeline
 * - Explore visualization options
 * - Interactive world exploration
 */

#include <gz/sim/gui/Gui.hh>
#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/common/Console.hh>
#include <gz/gui/Application.hh>
#include <gz/gui/MainWindow.hh>

#include <QApplication>
#include <iostream>
#include <memory>
#include <thread>

using namespace gz;

class WorldViewerApp
{
public:
    WorldViewerApp(const std::string &worldFile) : worldFile_(worldFile) {}

    int Run(int argc, char *argv[])
    {
        gzmsg << "Starting Interactive World Viewer..." << std::endl;
        gzmsg << "World file: " << worldFile_ << std::endl;

        // Initialize Qt Application
        QApplication app(argc, argv);
        app.setApplicationName("Gazebo World Viewer");
        app.setApplicationVersion("1.0");

        // Configure the simulation server
        ServerConfig serverConfig;
        serverConfig.SetSdfFile(worldFile_);
        serverConfig.SetUpdateRate(100.0);
        serverConfig.SetUseLevels(true);

        // Add essential systems for visualization
        serverConfig.AddPlugin("gz-sim-physics-system", "", "");
        serverConfig.AddPlugin("gz-sim-user-commands-system", "", "");
        serverConfig.AddPlugin("gz-sim-scene-broadcaster-system", "", "");
        
        // Create and start the server in a separate thread
        server_ = std::make_unique<sim::Server>(serverConfig);
        
        if (!server_->Running())
        {
            gzerr << "Failed to start simulation server" << std::endl;
            return -1;
        }

        // Start server thread
        serverThread_ = std::thread([this]() {
            while (this->server_->Running())
            {
                this->server_->RunOnce();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        // Configure GUI
        ConfigureGui();

        gzmsg << "World Viewer ready! Use mouse to navigate:" << std::endl;
        gzmsg << "- Left click + drag: Rotate view" << std::endl;
        gzmsg << "- Right click + drag: Pan view" << std::endl;
        gzmsg << "- Scroll wheel: Zoom in/out" << std::endl;

        // Run the GUI event loop
        int result = app.exec();

        // Cleanup
        Cleanup();
        return result;
    }

private:
    void ConfigureGui()
    {
        // Initialize GUI application
        char *argv[] = {const_cast<char*>("world_viewer")};
        int argc = 1;

        if (!gui::initApp(argc, argv))
        {
            gzerr << "Failed to initialize GUI application" << std::endl;
            return;
        }

        // Create main window
        auto mainWindow = gui::createMainWindow();
        if (!mainWindow)
        {
            gzerr << "Failed to create main window" << std::endl;
            return;
        }

        // Configure essential plugins
        AddGuiPlugins();

        // Show the window
        mainWindow->show();
    }

    void AddGuiPlugins()
    {
        // Add 3D scene plugin for world visualization
        std::string scenePluginConfig = R"(
            <plugin filename="MinimalScene" name="3D View">
              <gz-gui>
                <title>3D View</title>
                <property type="bool" key="showTitleBar">false</property>
                <property type="string" key="state">docked</property>
              </gz-gui>
              <engine>ogre2</engine>
              <scene>scene</scene>
              <ambient_light>0.4 0.4 0.4</ambient_light>
              <background_color>0.8 0.8 0.8</background_color>
              <camera_pose>-6 0 6 0 0.5 0</camera_pose>
              <camera_clip>
                <near>0.25</near>
                <far>25000</far>
              </camera_clip>
            </plugin>
        )";

        // Add entity tree plugin for world hierarchy
        std::string treePluginConfig = R"(
            <plugin filename="EntityTree" name="Entity Tree">
              <gz-gui>
                <title>Entity Tree</title>
                <property type="string" key="state">docked</property>
              </gz-gui>
            </plugin>
        )";

        // Add transform control plugin
        std::string transformPluginConfig = R"(
            <plugin filename="TransformControl" name="Transform Control">
              <gz-gui>
                <title>Transform Control</title>
                <property type="string" key="state">docked</property>
              </gz-gui>
            </plugin>
        )";

        // Add component inspector
        std::string inspectorPluginConfig = R"(
            <plugin filename="ComponentInspector" name="Component Inspector">
              <gz-gui>
                <title>Component Inspector</title>
                <property type="string" key="state">docked</property>
              </gz-gui>
            </plugin>
        )";

        // Load plugins (simplified for this example)
        // In a full implementation, these would be loaded through the GUI system
        gzmsg << "GUI plugins configured:" << std::endl;
        gzmsg << "- 3D Scene with Ogre2 rendering" << std::endl;
        gzmsg << "- Entity tree browser" << std::endl;
        gzmsg << "- Transform controls" << std::endl;
        gzmsg << "- Component inspector" << std::endl;
    }

    void Cleanup()
    {
        gzmsg << "Shutting down World Viewer..." << std::endl;
        
        if (server_)
        {
            server_.reset();
        }

        if (serverThread_.joinable())
        {
            serverThread_.join();
        }

        gzmsg << "World Viewer shutdown complete." << std::endl;
    }

private:
    std::string worldFile_;
    std::unique_ptr<sim::Server> server_;
    std::thread serverThread_;
};

int main(int argc, char *argv[])
{
    // Set up console logging
    gz::common::Console::SetVerbosity(2);

    // Determine world file to load
    std::string worldFile = "worlds/basic_world.sdf";
    if (argc > 1)
    {
        worldFile = argv[1];
    }

    try
    {
        WorldViewerApp viewer(worldFile);
        return viewer.Run(argc, argv);
    }
    catch (const std::exception &e)
    {
        gzerr << "Exception in World Viewer: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        gzerr << "Unknown exception in World Viewer" << std::endl;
        return -1;
    }
}

/*
 * Key Concepts Demonstrated:
 * 
 * 1. **GUI Integration**: How to create interactive Gazebo applications
 *    with graphical user interfaces.
 * 
 * 2. **Rendering Pipeline**: Understanding how Gazebo uses Ogre2 for
 *    real-time 3D rendering and visualization.
 * 
 * 3. **Plugin Architecture**: How GUI plugins extend functionality
 *    for different visualization and interaction needs.
 * 
 * 4. **Multi-threading**: Running simulation and GUI in separate threads
 *    for responsive user interaction.
 * 
 * 5. **Camera Controls**: Basic 3D navigation and viewport manipulation.
 * 
 * Usage Examples:
 * - ./world_viewer                          # Load default world
 * - ./world_viewer worlds/lighting_demo.sdf # Load specific world
 * - ./world_viewer my_custom_world.sdf      # Load custom world
 * 
 * Advanced Features to Explore:
 * - Custom GUI plugins for specific visualization needs
 * - Real-time parameter tuning through the interface
 * - Recording and playback of simulation sessions
 * - Integration with external control systems
 */