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
            sensor_->Shutdown();
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
    EXPECT_FALSE(sensor_->IsInitialized());
}

TEST_F(Lidar3DSensorTest, InitializeWithValidTopic) {
    bool result = sensor_->Initialize("/test_lidar", [this](const PointCloud& cloud) {
        TestCallback(cloud);
    });
    EXPECT_TRUE(result);
    EXPECT_TRUE(sensor_->IsInitialized());
}

TEST_F(Lidar3DSensorTest, InitializeWithEmptyTopic) {
    bool result = sensor_->Initialize("", [this](const PointCloud& cloud) {
        TestCallback(cloud);
    });
    EXPECT_FALSE(result);  // Should fail with empty topic
    EXPECT_FALSE(sensor_->IsInitialized());
}

TEST_F(Lidar3DSensorTest, CallbackInitialization) {
    // Initialize with callback
    EXPECT_TRUE(sensor_->Initialize("/test_topic", [this](const PointCloud& cloud) {
        TestCallback(cloud);
    }));
    EXPECT_TRUE(sensor_->IsInitialized());

    SUCCEED(); // No exceptions expected
}

TEST_F(Lidar3DSensorTest, InitializeShutdownLifecycle) {
    // Initialize sensor
    ASSERT_TRUE(sensor_->Initialize("/test_topic", [this](const PointCloud& cloud) {
        TestCallback(cloud);
    }));
    EXPECT_TRUE(sensor_->IsInitialized());

    // Shutdown sensor
    sensor_->Shutdown();
    EXPECT_FALSE(sensor_->IsInitialized());

    // Multiple shutdowns should be safe
    sensor_->Shutdown();
    sensor_->Shutdown();
    EXPECT_FALSE(sensor_->IsInitialized());

    // Can reinitialize
    EXPECT_TRUE(sensor_->Initialize("/test_topic2", [this](const PointCloud& cloud) {
        TestCallback(cloud);
    }));
    EXPECT_TRUE(sensor_->IsInitialized());
}

TEST_F(Lidar3DSensorTest, DoubleInitializationPrevented) {
    // First initialization should succeed
    ASSERT_TRUE(sensor_->Initialize("/test_topic", [this](const PointCloud& cloud) {
        TestCallback(cloud);
    }));
    EXPECT_TRUE(sensor_->IsInitialized());

    // Second initialization should fail
    EXPECT_FALSE(sensor_->Initialize("/another_topic", [this](const PointCloud& cloud) {
        TestCallback(cloud);
    }));
    EXPECT_TRUE(sensor_->IsInitialized()); // Still initialized with first topic
}

TEST_F(Lidar3DSensorTest, MessagesReceivedCounter) {
    ASSERT_TRUE(sensor_->Initialize("/test_topic", [this](const PointCloud& cloud) {
        TestCallback(cloud);
    }));

    // Initially zero
    EXPECT_EQ(sensor_->GetMessagesReceived(), 0);

    // Note: In test environment, we won't receive actual messages,
    // but counter should still be accessible
    EXPECT_GE(sensor_->GetMessagesReceived(), 0);
}

TEST_F(Lidar3DSensorTest, MultipleInstances) {
    auto sensor2 = std::make_unique<Lidar3DSensor>();
    auto sensor3 = std::make_unique<Lidar3DSensor>();

    // All should initialize successfully with different topics
    EXPECT_TRUE(sensor_->Initialize("/lidar1", [this](const PointCloud& cloud) {
        TestCallback(cloud);
    }));
    EXPECT_TRUE(sensor2->Initialize("/lidar2", [](const PointCloud& cloud) {
        // Empty callback
    }));
    EXPECT_TRUE(sensor3->Initialize("/lidar3", [](const PointCloud& cloud) {
        // Empty callback
    }));

    // All should be initialized
    EXPECT_TRUE(sensor_->IsInitialized());
    EXPECT_TRUE(sensor2->IsInitialized());
    EXPECT_TRUE(sensor3->IsInitialized());

    // All should shutdown independently
    sensor_->Shutdown();
    sensor2->Shutdown();
    sensor3->Shutdown();

    EXPECT_FALSE(sensor_->IsInitialized());
    EXPECT_FALSE(sensor2->IsInitialized());
    EXPECT_FALSE(sensor3->IsInitialized());
}

TEST_F(Lidar3DSensorTest, CallbackExceptionHandling) {
    ASSERT_TRUE(sensor_->Initialize("/test_topic", [](const PointCloud& cloud) {
        throw std::runtime_error("Test exception");
    }));

    // Sensor should still be functional after callback exception
    // (though in test environment we won't trigger actual callbacks)
    EXPECT_NO_THROW(sensor_->Shutdown());
}

TEST_F(Lidar3DSensorTest, ThreadSafety) {
    ASSERT_TRUE(sensor_->Initialize("/threaded_test", [this](const PointCloud& cloud) {
        TestCallback(cloud);
    }));

    // Test concurrent initialize/shutdown operations
    std::thread t1([this]() {
        for (int i = 0; i < 10; ++i) {
            if (!sensor_->IsInitialized()) {
                sensor_->Initialize("/threaded_test_" + std::to_string(i), [this](const PointCloud& cloud) {
                    TestCallback(cloud);
                });
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    std::thread t2([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        for (int i = 0; i < 10; ++i) {
            sensor_->Shutdown();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    t1.join();
    t2.join();

    // Final cleanup
    sensor_->Shutdown();
}

TEST_F(Lidar3DSensorTest, LambdaCallbackVariations) {
    // Test different callback types
    int local_counter = 0;

    // Capture by reference
    EXPECT_TRUE(sensor_->Initialize("/callback_test1", [&local_counter](const PointCloud& cloud) {
        local_counter++;
    }));
    sensor_->Shutdown();

    // Capture by value
    EXPECT_TRUE(sensor_->Initialize("/callback_test2", [local_counter](const PointCloud& cloud) mutable {
        local_counter++;
    }));
    sensor_->Shutdown();

    // No capture
    EXPECT_TRUE(sensor_->Initialize("/callback_test3", [](const PointCloud& cloud) {
        // Do nothing
    }));
}

TEST_F(Lidar3DSensorTest, TopicVariations) {
    // Test various topic name formats
    EXPECT_TRUE(sensor_->Initialize("/lidar", [](const PointCloud& cloud) {}));
    sensor_->Shutdown();

    auto sensor2 = std::make_unique<Lidar3DSensor>();
    EXPECT_TRUE(sensor2->Initialize("lidar", [](const PointCloud& cloud) {}));  // Without leading slash

    auto sensor3 = std::make_unique<Lidar3DSensor>();
    EXPECT_TRUE(sensor3->Initialize("/very/deep/topic/structure/lidar", [](const PointCloud& cloud) {}));

    auto sensor4 = std::make_unique<Lidar3DSensor>();
    EXPECT_TRUE(sensor4->Initialize("/lidar_with_underscores_and_123", [](const PointCloud& cloud) {}));
}
