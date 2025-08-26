#include <iostream>
#include <memory>
#include <vector>

// Open3D headers - avoid GUI/visualization headers that require OpenGL
#include <open3d/geometry/PointCloud.h>
#include <open3d/geometry/TriangleMesh.h>
#include <open3d/geometry/BoundingVolume.h>
#include <open3d/io/PointCloudIO.h>
#include <open3d/utility/Logging.h>

int main() {
    std::cout << "Open3D example with rules_nixpkgs system dependencies!" << std::endl;

    try {
        // Create a simple point cloud
        auto point_cloud = std::make_shared<open3d::geometry::PointCloud>();

        // Add some sample points
        std::vector<Eigen::Vector3d> points = {
            {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0},
            {1.0, 1.0, 0.0},
            {1.0, 0.0, 1.0},
            {0.0, 1.0, 1.0},
            {1.0, 1.0, 1.0}
        };

        // Set the points
        point_cloud->points_ = points;

        // Add colors (optional)
        std::vector<Eigen::Vector3d> colors;
        for (size_t i = 0; i < points.size(); ++i) {
            colors.push_back({
                static_cast<double>(i) / points.size(),  // Red component
                0.5,                                     // Green component
                1.0 - static_cast<double>(i) / points.size()  // Blue component
            });
        }
        point_cloud->colors_ = colors;

        std::cout << "✓ Created point cloud with " << point_cloud->points_.size() << " points" << std::endl;

        // Apply a transformation
        Eigen::Matrix4d transformation = Eigen::Matrix4d::Identity();
        transformation.block<3,3>(0,0) = Eigen::Matrix3d(
            Eigen::AngleAxisd(M_PI / 4, Eigen::Vector3d::UnitZ())
        );
        transformation.block<3,1>(0,3) = Eigen::Vector3d(1.0, 1.0, 1.0);

        point_cloud->Transform(transformation);
        std::cout << "✓ Applied transformation to point cloud" << std::endl;

        // Compute some basic properties
        if (point_cloud->HasPoints()) {
            auto bbox = point_cloud->GetAxisAlignedBoundingBox();
            std::cout << "✓ Bounding box computed: "
                      << "min(" << bbox.min_bound_.transpose() << ") "
                      << "max(" << bbox.max_bound_.transpose() << ")" << std::endl;
        }

        // Try to save the point cloud (headless operation)
        std::string output_path = "/tmp/open3d_example_output.ply";
        bool saved = open3d::io::WritePointCloud(output_path, *point_cloud);

        if (saved) {
            std::cout << "✓ Point cloud saved to " << output_path << std::endl;
        } else {
            std::cout << "! Failed to save point cloud to " << output_path << std::endl;
        }

        // Create a simple mesh (cube)
        auto mesh = open3d::geometry::TriangleMesh::CreateBox(1.0, 1.0, 1.0);
        mesh->ComputeVertexNormals();

        std::cout << "✓ Created cube mesh with " << mesh->vertices_.size()
                  << " vertices and " << mesh->triangles_.size() << " triangles" << std::endl;

        std::cout << "✓ Open3D example completed successfully!" << std::endl;
        std::cout << "✓ This demonstrates that Open3D prebuilt binary works with Bazel!" << std::endl;
        std::cout << "✓ System dependencies (if provided by nixpkgs) are working hermetically!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
