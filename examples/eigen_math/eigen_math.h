#pragma once

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/SVD>
#include <stdexcept>
#include <vector>

namespace eigen_math {

  /**
   * @brief A collection of mathematical operations using Eigen matrices and vectors
   *
   * This class demonstrates various Eigen operations that will be exposed to Python
   * through pybind11 bindings. It showcases both performance-critical computations
   * and common linear algebra operations.
   */
  class EigenMath {
  public:
    // Type aliases for common matrix and vector types
    using MatrixXd = Eigen::MatrixXd;
    using VectorXd = Eigen::VectorXd;
    using Vector3d = Eigen::Vector3d;
    using Matrix3d = Eigen::Matrix3d;
    using Matrix4d = Eigen::Matrix4d;

    /**
     * @brief Default constructor
     */
    EigenMath() = default;

    // =============================================================================
    // Matrix Operations
    // =============================================================================

    /**
     * @brief Create a random matrix with specified dimensions
     * @param rows Number of rows
     * @param cols Number of columns
     * @param seed Random seed for reproducibility
     * @return Random matrix
     */
    static MatrixXd createRandomMatrix(int rows, int cols, int seed = 42);

    /**
     * @brief Create an identity matrix
     * @param size Matrix dimension (square matrix)
     * @return Identity matrix
     */
    static MatrixXd createIdentityMatrix(int size);

    /**
     * @brief Multiply two matrices
     * @param a First matrix
     * @param b Second matrix
     * @return Product matrix a * b
     * @throws std::invalid_argument if matrices are incompatible for multiplication
     */
    static MatrixXd multiplyMatrices(const MatrixXd& a, const MatrixXd& b);

    /**
     * @brief Transpose a matrix
     * @param matrix Input matrix
     * @return Transposed matrix
     */
    static MatrixXd transposeMatrix(const MatrixXd& matrix);

    /**
     * @brief Compute matrix inverse
     * @param matrix Input matrix (must be square)
     * @return Inverse matrix
     * @throws std::invalid_argument if matrix is not square or not invertible
     */
    static MatrixXd invertMatrix(const MatrixXd& matrix);

    /**
     * @brief Compute matrix determinant
     * @param matrix Input matrix (must be square)
     * @return Determinant value
     * @throws std::invalid_argument if matrix is not square
     */
    static double determinant(const MatrixXd& matrix);

    // =============================================================================
    // Vector Operations
    // =============================================================================

    /**
     * @brief Create a random vector with specified dimension
     * @param size Vector dimension
     * @param seed Random seed for reproducibility
     * @return Random vector
     */
    static VectorXd createRandomVector(int size, int seed = 42);

    /**
     * @brief Compute dot product of two vectors
     * @param a First vector
     * @param b Second vector
     * @return Dot product
     * @throws std::invalid_argument if vectors have different sizes
     */
    static double dotProduct(const VectorXd& a, const VectorXd& b);

    /**
     * @brief Compute cross product of two 3D vectors
     * @param a First 3D vector
     * @param b Second 3D vector
     * @return Cross product vector
     */
    static Vector3d crossProduct(const Vector3d& a, const Vector3d& b);

    /**
     * @brief Normalize a vector (make unit length)
     * @param vector Input vector
     * @return Normalized vector
     * @throws std::invalid_argument if vector has zero norm
     */
    static VectorXd normalizeVector(const VectorXd& vector);

    /**
     * @brief Compute Euclidean norm of a vector
     * @param vector Input vector
     * @return Vector norm
     */
    static double vectorNorm(const VectorXd& vector);

    // =============================================================================
    // Advanced Linear Algebra
    // =============================================================================

    /**
     * @brief Compute eigenvalues and eigenvectors of a symmetric matrix
     * @param matrix Input symmetric matrix
     * @return Pair of (eigenvalues, eigenvectors matrix)
     */
    static std::pair<VectorXd, MatrixXd> eigenDecomposition(const MatrixXd& matrix);

    /**
     * @brief Compute Singular Value Decomposition (SVD)
     * @param matrix Input matrix
     * @return Tuple of (U, singular_values, V_transpose)
     */
    static std::tuple<MatrixXd, VectorXd, MatrixXd> svdDecomposition(const MatrixXd& matrix);

    /**
     * @brief Solve linear system Ax = b
     * @param A Coefficient matrix
     * @param b Right-hand side vector
     * @return Solution vector x
     * @throws std::invalid_argument if system is incompatible
     */
    static VectorXd solveLinearSystem(const MatrixXd& A, const VectorXd& b);

    /**
     * @brief Compute QR decomposition
     * @param matrix Input matrix
     * @return Pair of (Q, R) matrices
     */
    static std::pair<MatrixXd, MatrixXd> qrDecomposition(const MatrixXd& matrix);

    // =============================================================================
    // Geometric Operations
    // =============================================================================

    /**
     * @brief Create a rotation matrix from Euler angles (Z-Y-X convention)
     * @param roll Rotation around X-axis (radians)
     * @param pitch Rotation around Y-axis (radians)
     * @param yaw Rotation around Z-axis (radians)
     * @return 3x3 rotation matrix
     */
    static Matrix3d eulerToRotationMatrix(double roll, double pitch, double yaw);

    /**
     * @brief Extract Euler angles from rotation matrix
     * @param rotation 3x3 rotation matrix
     * @return Vector3d with (roll, pitch, yaw) in radians
     */
    static Vector3d rotationMatrixToEuler(const Matrix3d& rotation);

    /**
     * @brief Create a transformation matrix from translation and rotation
     * @param translation 3D translation vector
     * @param rotation 3x3 rotation matrix
     * @return 4x4 homogeneous transformation matrix
     */
    static Matrix4d createTransformationMatrix(const Vector3d& translation,
                                               const Matrix3d& rotation);

    // =============================================================================
    // Utility Functions
    // =============================================================================

    /**
     * @brief Check if two matrices are approximately equal
     * @param a First matrix
     * @param b Second matrix
     * @param tolerance Numerical tolerance
     * @return True if matrices are approximately equal
     */
    static bool matricesEqual(const MatrixXd& a, const MatrixXd& b, double tolerance = 1e-10);

    /**
     * @brief Get matrix properties as string
     * @param matrix Input matrix
     * @return String with dimensions and other properties
     */
    static std::string matrixInfo(const MatrixXd& matrix);

    /**
     * @brief Benchmark matrix multiplication performance
     * @param size Matrix size for square matrices
     * @param iterations Number of iterations
     * @return Time in milliseconds per operation
     */
    static double benchmarkMatrixMultiplication(int size, int iterations = 100);

  private:
    // Helper functions
    static void validateMatrixDimensions(const MatrixXd& matrix, int expected_rows = -1,
                                         int expected_cols = -1);
    static void validateVectorDimensions(const VectorXd& vector, int expected_size = -1);
  };

}  // namespace eigen_math