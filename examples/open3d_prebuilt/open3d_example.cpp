// Builds a point cloud and a mesh with Open3D, writes the cloud to a PLY file
// and, with --show, displays both in a window.
//
//   bazelisk run //:open3d_example -- [output.ply] [--show]
#include <open3d/Open3D.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>

namespace {

std::filesystem::path resolve(const std::filesystem::path& path) {
    // `bazel run` starts in the runfiles tree: resolve relative paths against
    // the directory the command was run from.
    const char* cwd = std::getenv("BUILD_WORKING_DIRECTORY");
    return (path.is_relative() && cwd != nullptr) ? std::filesystem::path(cwd) / path : path;
}

}  // namespace

int main(int argc, char** argv) {
    std::filesystem::path output = "points.ply";
    bool show = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--show") {
            show = true;
        } else {
            output = arg;
        }
    }

    // The corners of a unit cube, rotated 45 degrees around z and shifted.
    auto cloud = std::make_shared<open3d::geometry::PointCloud>();
    for (double x : {0.0, 1.0}) {
        for (double y : {0.0, 1.0}) {
            for (double z : {0.0, 1.0}) {
                cloud->points_.emplace_back(x, y, z);
            }
        }
    }
    cloud->PaintUniformColor({1.0, 0.5, 0.0});
    cloud->Rotate(open3d::geometry::Geometry3D::GetRotationMatrixFromXYZ({0.0, 0.0, M_PI / 4}),
                  Eigen::Vector3d::Zero());
    cloud->Translate({1.0, 1.0, 1.0});

    const auto box = cloud->GetAxisAlignedBoundingBox();
    std::cout << "Point cloud: " << cloud->points_.size() << " points, bounding box min "
              << box.min_bound_.transpose() << ", max " << box.max_bound_.transpose() << '\n';

    output = resolve(output);
    if (!open3d::io::WritePointCloud(output.string(), *cloud)) {
        std::cerr << "Failed to write " << output << '\n';
        return 1;
    }
    std::cout << "Wrote " << output << '\n';

    auto mesh = open3d::geometry::TriangleMesh::CreateBox();
    mesh->ComputeVertexNormals();
    std::cout << "Mesh: " << mesh->vertices_.size() << " vertices, " << mesh->triangles_.size()
              << " triangles\n";

    if (show) {
        open3d::visualization::DrawGeometries({cloud, mesh}, "Open3D example");
    }
    return 0;
}
