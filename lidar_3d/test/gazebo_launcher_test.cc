#include <gtest/gtest.h>
#include "../lib/visualization/gazebo_launcher.h"
#include <thread>

using namespace lidar3d;

class GazeboLauncherTest : public ::testing::Test {
protected:
    void SetUp() override {
        launcher_ = std::make_unique<GazeboLauncher>();
    }

    void TearDown() override {
        if (launcher_) {
            launcher_->Shutdown();
        }
    }

    std::unique_ptr<GazeboLauncher> launcher_;
};

TEST_F(GazeboLauncherTest, DefaultConstruction) {
    // Launcher should be created successfully
    EXPECT_TRUE(launcher_ != nullptr);
}

TEST_F(GazeboLauncherTest, MultipleShutdownCalls) {
    // Multiple shutdown calls should be safe
    launcher_->Shutdown();
    launcher_->Shutdown();
    launcher_->Shutdown();

    SUCCEED(); // No exceptions expected
}

TEST_F(GazeboLauncherTest, ShutdownWithoutLaunch) {
    // Should be safe to shutdown without launching
    EXPECT_NO_THROW(launcher_->Shutdown());
}

TEST_F(GazeboLauncherTest, WaitForReadyWithoutLaunch) {
    // Should return false if not launched
    bool ready = launcher_->WaitForReady();
    EXPECT_FALSE(ready);
}

TEST_F(GazeboLauncherTest, MultipleInstances) {
    auto launcher2 = std::make_unique<GazeboLauncher>();
    auto launcher3 = std::make_unique<GazeboLauncher>();

    // All should be created successfully
    EXPECT_TRUE(launcher2 != nullptr);
    EXPECT_TRUE(launcher3 != nullptr);

    // All should shutdown safely
    launcher2->Shutdown();
    launcher3->Shutdown();
}

TEST_F(GazeboLauncherTest, MethodCallsWithoutCrash) {
    // Test that methods can be called without crashing
    EXPECT_NO_THROW(launcher_->Launch("nonexistent.sdf"));
    EXPECT_NO_THROW(launcher_->WaitForReady());
    EXPECT_NO_THROW(launcher_->Shutdown());

    // Test with empty path
    EXPECT_NO_THROW(launcher_->Launch(""));
    EXPECT_NO_THROW(launcher_->Shutdown());
}

TEST_F(GazeboLauncherTest, DestructorCleanup) {
    // Test that destructor properly cleans up
    {
        auto temp_launcher = std::make_unique<GazeboLauncher>();
        // Destructor should be called here
    }

    SUCCEED(); // Should complete without issues
}

TEST_F(GazeboLauncherTest, ThreadSafetyBasic) {
    // Basic thread safety test without actually launching
    std::thread t1([this]() {
        launcher_->Shutdown();
    });

    std::thread t2([this]() {
        launcher_->WaitForReady();
    });

    t1.join();
    t2.join();

    SUCCEED();
}
