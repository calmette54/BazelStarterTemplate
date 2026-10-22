#include "lib/visualization/gazebo_launcher.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

using lidar3d::GazeboLauncher;

namespace {

// test/fake_gz.sh from the runfiles: a `gz` that just sleeps.
std::string FakeGz() {
    return std::string(std::getenv("TEST_SRCDIR")) + "/_main/test/fake_gz.sh";
}

}  // namespace

TEST(GazeboLauncherTest, NotRunningInitially) {
    GazeboLauncher launcher(FakeGz());
    EXPECT_FALSE(launcher.IsRunning());
    EXPECT_FALSE(launcher.WaitForReady(1));
}

TEST(GazeboLauncherTest, LaunchFailsWhenExecutableIsMissing) {
    GazeboLauncher launcher("/nonexistent/gz");
    EXPECT_FALSE(launcher.Launch("world.sdf"));
    EXPECT_FALSE(launcher.IsRunning());
}

TEST(GazeboLauncherTest, LaunchAndShutdown) {
    GazeboLauncher launcher(FakeGz());
    ASSERT_TRUE(launcher.Launch("world.sdf"));
    EXPECT_TRUE(launcher.IsRunning());

    // Only one simulation at a time
    EXPECT_FALSE(launcher.Launch("world.sdf"));

    launcher.Shutdown();
    EXPECT_FALSE(launcher.IsRunning());
    launcher.Shutdown();  // no-op

    // Can be started again
    EXPECT_TRUE(launcher.Launch("world.sdf"));
}

TEST(GazeboLauncherTest, ShutdownWithoutLaunchIsANoOp) {
    GazeboLauncher launcher(FakeGz());
    launcher.Shutdown();
    EXPECT_FALSE(launcher.IsRunning());
}
