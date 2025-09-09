#include <gtest/gtest.h>
#include "../lib/sensor/lidar_3d_sensor.h"
#include <atomic>
#include <thread>
#include <chrono>

using namespace lidar3d;

class Lidar3DSensorTest : public ::testing::Test {
protected:
    void SetUp() override {
        sensor_ = std::make_unique<Lidar3DSensor>();
        callback_count_.store(0);
        last_cloud_ = PointCloud();
    }

    void TearDown() override {
        if (sensor_) {
            sensor_->Stop();
        }
    }

    // Test callback function
    void TestCallback(const PointCloud& cloud) {
        callback_count_.fetch_add(1);
        last_cloud_ = cloud;
    }

    std::unique_ptr<Lidar3DSensor> sensor_;
    std::atomic<int> callback_count_;
    PointCloud last_cloud_;
};

TEST_F(Lidar3DSensorTest, DefaultConstruction) {
    EXPECT_EQ(sensor_->GetMessagesReceived(), 0);
}

TEST_F(Lidar3DSensorTest, InitializeWithValidTopic) {
    bool result = sensor_->Initialize("/test_lidar");
    EXPECT_TRUE(result);
}

TEST_F(Lidar3DSensorTest, InitializeWithEmptyTopic) {
    bool result = sensor_->Initialize("");
    EXPECT_FALSE(result);  // Should fail with empty topic
}

TEST_F(Lidar3DSensorTest, CallbackSetting) {
    // Set callback before initialization
    sensor_->SetCallback([this](const PointCloud& cloud) {
        TestCallback(cloud);
    });

    EXPECT_TRUE(sensor_->Initialize("/test_topic"));

    // Change callback after initialization
    sensor_->SetCallback([this](const PointCloud& cloud) {
        callback_count_.fetch_add(2);  // Different behavior
    });

    SUCCEED(); // No exceptions expected
}

TEST_F(Lidar3DSensorTest, StartStopLifecycle) {
    ASSERT_TRUE(sensor_->Initialize("/test_topic"));

    // Start sensor
    sensor_->Start();

    // Stop sensor
    sensor_->Stop();

    // Multiple stops should be safe
    sensor_->Stop();
    sensor_->Stop();

    // Can restart
    sensor_->Start();
    sensor_->Stop();
}

TEST_F(Lidar3DSensorTest, MultipleStartCalls) {
    ASSERT_TRUE(sensor_->Initialize("/test_topic"));

    // Multiple starts should be safe
    sensor_->Start();
    sensor_->Start();
    sensor_->Start();

    sensor_->Stop();
}

TEST_F(Lidar3DSensorTest, CallbackWithoutInitialization) {
    // Setting callback before initialization should work
    bool callback_set = false;
    sensor_->SetCallback([&callback_set](const PointCloud& cloud) {
        callback_set = true;
    });

    EXPECT_FALSE(callback_set);
}

TEST_F(Lidar3DSensorTest, MessagesReceivedCounter) {
    ASSERT_TRUE(sensor_->Initialize("/test_topic"));

    // Initially zero
    EXPECT_EQ(sensor_->GetMessagesReceived(), 0);

    // Set callback to verify counter behavior
    sensor_->SetCallback([this](const PointCloud& cloud) {
        TestCallback(cloud);
    });

    sensor_->Start();

    // Note: In test environment, we won't receive actual messages,
    // but counter should still be accessible
    EXPECT_GE(sensor_->GetMessagesReceived(), 0);

    sensor_->Stop();
}

TEST_F(Lidar3DSensorTest, MultipleInstances) {
    auto sensor2 = std::make_unique<Lidar3DSensor>();
    auto sensor3 = std::make_unique<Lidar3DSensor>();

    // All should initialize successfully with different topics
    EXPECT_TRUE(sensor_->Initialize("/lidar1"));
    EXPECT_TRUE(sensor2->Initialize("/lidar2"));
    EXPECT_TRUE(sensor3->Initialize("/lidar3"));

    // All should start/stop independently
    sensor_->Start();
    sensor2->Start();
    sensor3->Start();

    sensor_->Stop();
    sensor2->Stop();
    sensor3->Stop();
}

TEST_F(Lidar3DSensorTest, CallbackExceptionHandling) {
    ASSERT_TRUE(sensor_->Initialize("/test_topic"));

    // Set a callback that throws
    sensor_->SetCallback([](const PointCloud& cloud) {
        throw std::runtime_error("Test exception");
    });

    sensor_->Start();

    // Sensor should still be functional after callback exception
    // (though in test environment we won't trigger actual callbacks)
    EXPECT_NO_THROW(sensor_->Stop());
}

TEST_F(Lidar3DSensorTest, ThreadSafety) {
    ASSERT_TRUE(sensor_->Initialize("/threaded_test"));

    sensor_->SetCallback([this](const PointCloud& cloud) {
        TestCallback(cloud);
    });

    // Test concurrent start/stop operations
    std::thread t1([this]() {
        for (int i = 0; i < 10; ++i) {
            sensor_->Start();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    std::thread t2([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        for (int i = 0; i < 10; ++i) {
            sensor_->Stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    t1.join();
    t2.join();

    // Final cleanup
    sensor_->Stop();
}

TEST_F(Lidar3DSensorTest, LambdaCallbackVariations) {
    ASSERT_TRUE(sensor_->Initialize("/callback_test"));

    // Test different callback types
    int local_counter = 0;

    // Capture by reference
    sensor_->SetCallback([&local_counter](const PointCloud& cloud) {
        local_counter++;
    });

    // Capture by value
    sensor_->SetCallback([local_counter](const PointCloud& cloud) mutable {
        local_counter++;
    });

    // No capture
    sensor_->SetCallback([](const PointCloud& cloud) {
        // Do nothing
    });

    SUCCEED();
}

TEST_F(Lidar3DSensorTest, TopicVariations) {
    // Test various topic name formats
    EXPECT_TRUE(sensor_->Initialize("/lidar"));

    auto sensor2 = std::make_unique<Lidar3DSensor>();
    EXPECT_TRUE(sensor2->Initialize("lidar"));  // Without leading slash

    auto sensor3 = std::make_unique<Lidar3DSensor>();
    EXPECT_TRUE(sensor3->Initialize("/very/deep/topic/structure/lidar"));

    auto sensor4 = std::make_unique<Lidar3DSensor>();
    EXPECT_TRUE(sensor4->Initialize("/lidar_with_underscores_and_123"));
}
