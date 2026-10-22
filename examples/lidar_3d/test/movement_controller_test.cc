#include <gtest/gtest.h>
#include "lib/control/movement_controller.h"
#include <thread>
#include <chrono>

using namespace lidar3d;

class MovementControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        controller_ = std::make_unique<MovementController>();
    }

    void TearDown() override {
        if (controller_) {
            controller_->StopKeyboardControl();
        }
    }

    std::unique_ptr<MovementController> controller_;
};

TEST_F(MovementControllerTest, DefaultConstruction) {
    EXPECT_FALSE(controller_->IsListening());
}

TEST_F(MovementControllerTest, Initialize) {
    bool result = controller_->Initialize("test_world", "test_model");
    EXPECT_TRUE(result);
}

TEST_F(MovementControllerTest, ConfigurationSetters) {
    controller_->SetLinearStep(1.5);
    controller_->SetAngularStep(0.2);

    // We can't directly test the values but can verify no crashes
    SUCCEED();
}

TEST_F(MovementControllerTest, KeyboardControlLifecycle) {
    // Initialize first
    ASSERT_TRUE(controller_->Initialize("test_world", "test_model"));

    // Should not be listening initially
    EXPECT_FALSE(controller_->IsListening());

    // Start keyboard control
    controller_->StartKeyboardControl();
    EXPECT_TRUE(controller_->IsListening());

    // Starting again should be safe
    controller_->StartKeyboardControl();
    EXPECT_TRUE(controller_->IsListening());

    // Stop keyboard control
    controller_->StopKeyboardControl();
    EXPECT_FALSE(controller_->IsListening());

    // Stopping again should be safe
    controller_->StopKeyboardControl();
    EXPECT_FALSE(controller_->IsListening());
}

TEST_F(MovementControllerTest, MovementCommandsWithoutCrash) {
    // Initialize
    ASSERT_TRUE(controller_->Initialize("test_world", "test_model"));

    // Test that movement commands don't crash
    // Note: These will fail to actually move anything in test environment,
    // but we're testing that the methods can be called safely
    EXPECT_NO_THROW(controller_->Move(MovementController::Direction::FORWARD));
    EXPECT_NO_THROW(controller_->Move(MovementController::Direction::BACKWARD));
    EXPECT_NO_THROW(controller_->Move(MovementController::Direction::LEFT));
    EXPECT_NO_THROW(controller_->Move(MovementController::Direction::RIGHT));
    EXPECT_NO_THROW(controller_->Move(MovementController::Direction::UP));
    EXPECT_NO_THROW(controller_->Move(MovementController::Direction::DOWN));
    EXPECT_NO_THROW(controller_->Move(MovementController::Direction::TURN_LEFT));
    EXPECT_NO_THROW(controller_->Move(MovementController::Direction::TURN_RIGHT));
}

TEST_F(MovementControllerTest, InitializationVariations) {
    // Test with different world names
    EXPECT_TRUE(controller_->Initialize("world1", "model1"));

    auto controller2 = std::make_unique<MovementController>();
    EXPECT_TRUE(controller2->Initialize("another_world", "another_model"));

    // Test with empty strings (should still succeed)
    auto controller3 = std::make_unique<MovementController>();
    EXPECT_TRUE(controller3->Initialize("", ""));
}

TEST_F(MovementControllerTest, MultipleInstances) {
    // Test that multiple controllers can coexist
    auto controller2 = std::make_unique<MovementController>();
    auto controller3 = std::make_unique<MovementController>();

    EXPECT_TRUE(controller_->Initialize("world1", "model1"));
    EXPECT_TRUE(controller2->Initialize("world2", "model2"));
    EXPECT_TRUE(controller3->Initialize("world3", "model3"));

    // All should work independently
    EXPECT_FALSE(controller_->IsListening());
    EXPECT_FALSE(controller2->IsListening());
    EXPECT_FALSE(controller3->IsListening());

    controller2->StartKeyboardControl();
    EXPECT_FALSE(controller_->IsListening());
    EXPECT_TRUE(controller2->IsListening());
    EXPECT_FALSE(controller3->IsListening());

    controller2->StopKeyboardControl();
}

TEST_F(MovementControllerTest, ThreadSafety) {
    ASSERT_TRUE(controller_->Initialize("test_world", "test_model"));

    // Test that starting/stopping from different threads works
    std::thread t1([this]() {
        controller_->StartKeyboardControl();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        controller_->StopKeyboardControl();
    });

    std::thread t2([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        controller_->StartKeyboardControl();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    t1.join();
    t2.join();

    // Final cleanup
    controller_->StopKeyboardControl();
}

TEST_F(MovementControllerTest, StressTest) {
    ASSERT_TRUE(controller_->Initialize("stress_world", "stress_model"));

    // Rapidly start/stop keyboard control
    for (int i = 0; i < 100; ++i) {
        controller_->StartKeyboardControl();
        controller_->StopKeyboardControl();
    }

    EXPECT_FALSE(controller_->IsListening());
}

TEST_F(MovementControllerTest, ConfigurationEdgeCases) {
    // Test with extreme values
    controller_->SetLinearStep(0.0);  // Zero step
    controller_->SetAngularStep(0.0); // Zero step
    EXPECT_NO_THROW(controller_->Move(MovementController::Direction::FORWARD));

    controller_->SetLinearStep(1000.0);  // Large step
    controller_->SetAngularStep(10.0);   // Large angular step
    EXPECT_NO_THROW(controller_->Move(MovementController::Direction::TURN_LEFT));

    controller_->SetLinearStep(-1.0);  // Negative step
    controller_->SetAngularStep(-1.0); // Negative angular step
    EXPECT_NO_THROW(controller_->Move(MovementController::Direction::UP));
}
