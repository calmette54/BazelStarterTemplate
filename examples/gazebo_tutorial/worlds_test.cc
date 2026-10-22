#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <sdf/Root.hh>
#include <string>
#include <vector>

namespace {

std::vector<std::filesystem::path> WorldFiles() {
    const std::filesystem::path dir = std::filesystem::path(std::getenv("TEST_SRCDIR")) / "_main" / "worlds";
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() == ".sdf") {
            files.push_back(entry.path());
        }
    }
    return files;
}

}  // namespace

TEST(WorldsTest, EveryWorldParses) {
    const auto files = WorldFiles();
    ASSERT_FALSE(files.empty());
    for (const auto& file : files) {
        sdf::Root root;
        const sdf::Errors errors = root.Load(file.string());
        std::string messages;
        for (const auto& error : errors) {
            messages += "\n  " + error.Message();
        }
        EXPECT_TRUE(errors.empty()) << file.filename() << ":" << messages;
        EXPECT_GE(root.WorldCount(), 1U) << file.filename();
    }
}
