#include "eigen_math.h"

#include <Eigen/Eigenvalues>
#include <Eigen/QR>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <random>
#include <sstream>

namespace eigen_math {

// Type aliases for cleaner code
using MatrixXd = EigenMath::MatrixXd;
using VectorXd = EigenMath::VectorXd;
using Vector3d = EigenMath::Vector3d;
using Matrix3d = EigenMath::Matrix3d;
using Matrix4d = EigenMath::Matrix4d;

// =============================================================================
// Matrix Operations
// =============================================================================

MatrixXd EigenMath::createRandomMatrix(int rows, int cols, int seed) {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Matrix dimensions must be positive");
    }
    
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dis(-1.0, 1.0);
    
    MatrixXd matrix(rows, cols);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix(i, j) = dis(gen);
        }
    }
    
    return matrix;
}

MatrixXd EigenMath::createIdentityMatrix(int size) {
    if (size <= 0) {
        throw std::invalid_argument("Matrix size must be positive");
    }
    
    return MatrixXd::Identity(size, size);
}

MatrixXd EigenMath::multiplyMatrices(const MatrixXd& a, const MatrixXd& b) {
    if (a.cols() != b.rows()) {
        throw std::invalid_argument(
            "Matrix dimensions incompatible for multiplication: " +
            std::to_string(a.rows()) + "x" + std::to_string(a.cols()) +
            " * " + std::to_string(b.rows()) + "x" + std::to_string(b.cols())
        );
    }
    
    return a * b;
}

MatrixXd EigenMath::transposeMatrix(const MatrixXd& matrix) {
    return matrix.transpose();
}

MatrixXd EigenMath::invertMatrix(const MatrixXd& matrix) {
    if (matrix.rows() != matrix.cols()) {
        throw std::invalid_argument("Matrix must be square for inversion");
    }
    
    Eigen::FullPivLU<MatrixXd> lu(matrix);
    if (!lu.isInvertible()) {
        throw std::invalid_argument("Matrix is not invertible (determinant is zero)");
    }
    
    return lu.inverse();
}

double EigenMath::determinant(const MatrixXd& matrix) {
    if (matrix.rows() != matrix.cols()) {
        throw std::invalid_argument("Matrix must be square to compute determinant");
    }
    
    return matrix.determinant();
}

// =============================================================================
// Vector Operations
// =============================================================================

VectorXd EigenMath::createRandomVector(int size, int seed) {
    if (size <= 0) {
        throw std::invalid_argument("Vector size must be positive");
    }
    
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dis(-1.0, 1.0);
    
    VectorXd vector(size);
    for (int i = 0; i < size; ++i) {
        vector(i) = dis(gen);
    }
    
    return vector;
}

double EigenMath::dotProduct(const VectorXd& a, const VectorXd& b) {
    if (a.size() != b.size()) {
        throw std::invalid_argument(
            "Vector sizes must match for dot product: " +
            std::to_string(a.size()) + " vs " + std::to_string(b.size())
        );
    }
    
    return a.dot(b);
}

Vector3d EigenMath::crossProduct(const Vector3d& a, const Vector3d& b) {
    return a.cross(b);
}

VectorXd EigenMath::normalizeVector(const VectorXd& vector) {
    double norm = vector.norm();
    if (norm < 1e-12) {
        throw std::invalid_argument("Cannot normalize zero vector");
    }
    
    return vector / norm;
}

double EigenMath::vectorNorm(const VectorXd& vector) {
    return vector.norm();
}

// =============================================================================
// Advanced Linear Algebra
// =============================================================================

std::pair<VectorXd, MatrixXd> EigenMath::eigenDecomposition(const MatrixXd& matrix) {
    if (matrix.rows() != matrix.cols()) {
        throw std::invalid_argument("Matrix must be square for eigen decomposition");
    }
    
    // Check if matrix is approximately symmetric
    MatrixXd diff = matrix - matrix.transpose();
    if (diff.norm() > 1e-10) {
        throw std::invalid_argument("Matrix must be symmetric for real eigen decomposition");
    }
    
    Eigen::SelfAdjointEigenSolver<MatrixXd> solver(matrix);
    if (solver.info() != Eigen::Success) {
        throw std::runtime_error("Eigenvalue decomposition failed");
    }
    
    return {solver.eigenvalues(), solver.eigenvectors()};
}

std::tuple<MatrixXd, VectorXd, MatrixXd> EigenMath::svdDecomposition(const MatrixXd& matrix) {
    Eigen::JacobiSVD<MatrixXd> svd(matrix, Eigen::ComputeFullU | Eigen::ComputeFullV);
    
    return {svd.matrixU(), svd.singularValues(), svd.matrixV().transpose()};
}

VectorXd EigenMath::solveLinearSystem(const MatrixXd& A, const VectorXd& b) {
    if (A.rows() != b.size()) {
        throw std::invalid_argument(
            "Incompatible dimensions for linear system: matrix rows (" +
            std::to_string(A.rows()) + ") != vector size (" + std::to_string(b.size()) + ")"
        );
    }
    
    if (A.rows() != A.cols()) {
        // Over/under-determined system, use least squares
        return A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);
    } else {
        // Square system, use LU decomposition
        Eigen::FullPivLU<MatrixXd> lu(A);
        if (!lu.isInvertible()) {
            throw std::invalid_argument("Matrix is singular, no unique solution exists");
        }
        return lu.solve(b);
    }
}

std::pair<MatrixXd, MatrixXd> EigenMath::qrDecomposition(const MatrixXd& matrix) {
    Eigen::HouseholderQR<MatrixXd> qr(matrix);
    
    MatrixXd Q = qr.householderQ();
    MatrixXd R = qr.matrixQR().triangularView<Eigen::Upper>();
    
    return {Q, R};
}

// =============================================================================
// Geometric Operations
// =============================================================================

Matrix3d EigenMath::eulerToRotationMatrix(double roll, double pitch, double yaw) {
    // Z-Y-X Euler angle convention (yaw-pitch-roll)
    Eigen::AngleAxisd rollAngle(roll, Vector3d::UnitX());
    Eigen::AngleAxisd pitchAngle(pitch, Vector3d::UnitY());
    Eigen::AngleAxisd yawAngle(yaw, Vector3d::UnitZ());
    
    Eigen::Quaterniond q = yawAngle * pitchAngle * rollAngle;
    return q.matrix();
}

Vector3d EigenMath::rotationMatrixToEuler(const Matrix3d& rotation) {
    // Extract Euler angles using Z-Y-X convention
    Vector3d euler;
    
    // Check for gimbal lock
    double sy = std::sqrt(rotation(0,0) * rotation(0,0) + rotation(1,0) * rotation(1,0));
    bool singular = sy < 1e-6;
    
    if (!singular) {
        euler(0) = std::atan2(rotation(2,1), rotation(2,2));  // roll
        euler(1) = std::atan2(-rotation(2,0), sy);             // pitch
        euler(2) = std::atan2(rotation(1,0), rotation(0,0));   // yaw
    } else {
        euler(0) = std::atan2(-rotation(1,2), rotation(1,1)); // roll
        euler(1) = std::atan2(-rotation(2,0), sy);             // pitch
        euler(2) = 0;                                          // yaw
    }
    
    return euler;
}

Matrix4d EigenMath::createTransformationMatrix(const Vector3d& translation, const Matrix3d& rotation) {
    Matrix4d transform = Matrix4d::Identity();
    transform.block<3,3>(0,0) = rotation;
    transform.block<3,1>(0,3) = translation;
    return transform;
}

// =============================================================================
// Utility Functions
// =============================================================================

bool EigenMath::matricesEqual(const MatrixXd& a, const MatrixXd& b, double tolerance) {
    if (a.rows() != b.rows() || a.cols() != b.cols()) {
        return false;
    }
    
    return (a - b).norm() < tolerance;
}

std::string EigenMath::matrixInfo(const MatrixXd& matrix) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(6);
    
    ss << "Matrix Info:\n";
    ss << "  Dimensions: " << matrix.rows() << " x " << matrix.cols() << "\n";
    ss << "  Norm: " << matrix.norm() << "\n";
    
    if (matrix.rows() == matrix.cols()) {
        ss << "  Determinant: " << matrix.determinant() << "\n";
        ss << "  Trace: " << matrix.trace() << "\n";
        
        // Check if symmetric
        MatrixXd diff = matrix - matrix.transpose();
        if (diff.norm() < 1e-10) {
            ss << "  Type: Symmetric\n";
        } else {
            ss << "  Type: General square\n";
        }
    }
    
    ss << "  Min value: " << matrix.minCoeff() << "\n";
    ss << "  Max value: " << matrix.maxCoeff() << "\n";
    
    return ss.str();
}

double EigenMath::benchmarkMatrixMultiplication(int size, int iterations) {
    if (size <= 0 || iterations <= 0) {
        throw std::invalid_argument("Size and iterations must be positive");
    }
    
    // Create random matrices for benchmarking
    MatrixXd a = createRandomMatrix(size, size, 42);
    MatrixXd b = createRandomMatrix(size, size, 43);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        MatrixXd c = a * b;
        // Prevent optimization from eliminating the computation
        volatile double dummy = c(0, 0);
        (void)dummy;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    return static_cast<double>(duration.count()) / iterations;
}

// =============================================================================
// Helper Functions
// =============================================================================

void EigenMath::validateMatrixDimensions(const MatrixXd& matrix, int expected_rows, int expected_cols) {
    if (expected_rows >= 0 && matrix.rows() != expected_rows) {
        throw std::invalid_argument(
            "Expected " + std::to_string(expected_rows) + " rows, got " + std::to_string(matrix.rows())
        );
    }
    
    if (expected_cols >= 0 && matrix.cols() != expected_cols) {
        throw std::invalid_argument(
            "Expected " + std::to_string(expected_cols) + " columns, got " + std::to_string(matrix.cols())
        );
    }
}

void EigenMath::validateVectorDimensions(const VectorXd& vector, int expected_size) {
    if (expected_size >= 0 && vector.size() != expected_size) {
        throw std::invalid_argument(
            "Expected vector size " + std::to_string(expected_size) + ", got " + std::to_string(vector.size())
        );
    }
}

} // namespace eigen_math