#include <gtest/gtest.h>

#include <Eigen/Core>
#include <cmath>

#include "eigen_math.h"

using namespace eigen_math;
using namespace Eigen;

class EigenMathTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Set up common test data
    tolerance = 1e-10;
  }

  double tolerance;

  // Helper function to check matrix equality with tolerance
  bool matricesEqual(const MatrixXd& a, const MatrixXd& b, double tol = 1e-10) {
    return EigenMath::matricesEqual(a, b, tol);
  }
};

// =============================================================================
// Matrix Operations Tests
// =============================================================================

TEST_F(EigenMathTest, CreateRandomMatrix) {
  // Test basic functionality
  MatrixXd matrix = EigenMath::createRandomMatrix(3, 4, 42);
  EXPECT_EQ(matrix.rows(), 3);
  EXPECT_EQ(matrix.cols(), 4);

  // Test reproducibility with same seed
  MatrixXd matrix2 = EigenMath::createRandomMatrix(3, 4, 42);
  EXPECT_TRUE(matricesEqual(matrix, matrix2, tolerance));

  // Test different results with different seed
  MatrixXd matrix3 = EigenMath::createRandomMatrix(3, 4, 43);
  EXPECT_FALSE(matricesEqual(matrix, matrix3, tolerance));

  // Test error handling
  EXPECT_THROW(EigenMath::createRandomMatrix(0, 4), std::invalid_argument);
  EXPECT_THROW(EigenMath::createRandomMatrix(3, 0), std::invalid_argument);
  EXPECT_THROW(EigenMath::createRandomMatrix(-1, 4), std::invalid_argument);
}

TEST_F(EigenMathTest, CreateIdentityMatrix) {
  // Test 3x3 identity
  MatrixXd identity = EigenMath::createIdentityMatrix(3);
  MatrixXd expected = MatrixXd::Identity(3, 3);
  EXPECT_TRUE(matricesEqual(identity, expected, tolerance));

  // Test 1x1 identity
  MatrixXd identity1 = EigenMath::createIdentityMatrix(1);
  EXPECT_EQ(identity1(0, 0), 1.0);

  // Test error handling
  EXPECT_THROW(EigenMath::createIdentityMatrix(0), std::invalid_argument);
  EXPECT_THROW(EigenMath::createIdentityMatrix(-1), std::invalid_argument);
}

TEST_F(EigenMathTest, MultiplyMatrices) {
  // Test compatible matrices
  MatrixXd a(2, 3);
  a << 1, 2, 3, 4, 5, 6;

  MatrixXd b(3, 2);
  b << 7, 8, 9, 10, 11, 12;

  MatrixXd result = EigenMath::multiplyMatrices(a, b);
  MatrixXd expected = a * b;
  EXPECT_TRUE(matricesEqual(result, expected, tolerance));

  // Test with identity
  MatrixXd identity = EigenMath::createIdentityMatrix(3);
  MatrixXd c = EigenMath::createRandomMatrix(3, 3, 42);
  MatrixXd result2 = EigenMath::multiplyMatrices(identity, c);
  EXPECT_TRUE(matricesEqual(result2, c, tolerance));

  // Test error handling - incompatible dimensions
  MatrixXd d(2, 3);
  MatrixXd e(4, 2);  // 3 != 4
  EXPECT_THROW(EigenMath::multiplyMatrices(d, e), std::invalid_argument);
}

TEST_F(EigenMathTest, TransposeMatrix) {
  MatrixXd matrix(2, 3);
  matrix << 1, 2, 3, 4, 5, 6;

  MatrixXd result = EigenMath::transposeMatrix(matrix);
  MatrixXd expected = matrix.transpose();
  EXPECT_TRUE(matricesEqual(result, expected, tolerance));

  // Test that transpose of transpose equals original
  MatrixXd double_transpose = EigenMath::transposeMatrix(result);
  EXPECT_TRUE(matricesEqual(double_transpose, matrix, tolerance));
}

TEST_F(EigenMathTest, InvertMatrix) {
  // Test with known invertible matrix
  MatrixXd matrix(2, 2);
  matrix << 1, 2, 3, 4;

  MatrixXd inverse = EigenMath::invertMatrix(matrix);
  MatrixXd product = EigenMath::multiplyMatrices(matrix, inverse);
  MatrixXd identity = EigenMath::createIdentityMatrix(2);
  EXPECT_TRUE(matricesEqual(product, identity, 1e-10));

  // Test error handling - non-square matrix
  MatrixXd non_square(2, 3);
  EXPECT_THROW(EigenMath::invertMatrix(non_square), std::invalid_argument);

  // Test error handling - singular matrix
  MatrixXd singular(2, 2);
  singular << 1, 2, 2, 4;  // Row 2 = 2 * Row 1, so singular
  EXPECT_THROW(EigenMath::invertMatrix(singular), std::invalid_argument);
}

TEST_F(EigenMathTest, Determinant) {
  // Test 2x2 determinant
  MatrixXd matrix(2, 2);
  matrix << 1, 2, 3, 4;
  double det = EigenMath::determinant(matrix);
  double expected = 1 * 4 - 2 * 3;  // -2
  EXPECT_NEAR(det, expected, tolerance);

  // Test identity matrix determinant
  MatrixXd identity = EigenMath::createIdentityMatrix(3);
  double det_identity = EigenMath::determinant(identity);
  EXPECT_NEAR(det_identity, 1.0, tolerance);

  // Test error handling - non-square matrix
  MatrixXd non_square(2, 3);
  EXPECT_THROW(EigenMath::determinant(non_square), std::invalid_argument);
}

// =============================================================================
// Vector Operations Tests
// =============================================================================

TEST_F(EigenMathTest, CreateRandomVector) {
  // Test basic functionality
  VectorXd vector = EigenMath::createRandomVector(5, 42);
  EXPECT_EQ(vector.size(), 5);

  // Test reproducibility
  VectorXd vector2 = EigenMath::createRandomVector(5, 42);
  EXPECT_TRUE((vector - vector2).norm() < tolerance);

  // Test error handling
  EXPECT_THROW(EigenMath::createRandomVector(0), std::invalid_argument);
  EXPECT_THROW(EigenMath::createRandomVector(-1), std::invalid_argument);
}

TEST_F(EigenMathTest, DotProduct) {
  VectorXd a(3);
  a << 1, 2, 3;
  VectorXd b(3);
  b << 4, 5, 6;

  double result = EigenMath::dotProduct(a, b);
  double expected = 1 * 4 + 2 * 5 + 3 * 6;  // 32
  EXPECT_NEAR(result, expected, tolerance);

  // Test with orthogonal vectors (dot product should be 0)
  VectorXd c(2);
  c << 1, 0;
  VectorXd d(2);
  d << 0, 1;
  double dot_orthogonal = EigenMath::dotProduct(c, d);
  EXPECT_NEAR(dot_orthogonal, 0.0, tolerance);

  // Test error handling - different sizes
  VectorXd e(3);
  VectorXd f(4);
  EXPECT_THROW(EigenMath::dotProduct(e, f), std::invalid_argument);
}

TEST_F(EigenMathTest, CrossProduct) {
  Vector3d a(1, 0, 0);  // Unit vector in x direction
  Vector3d b(0, 1, 0);  // Unit vector in y direction

  Vector3d result = EigenMath::crossProduct(a, b);
  Vector3d expected(0, 0, 1);  // Should be unit vector in z direction

  EXPECT_NEAR((result - expected).norm(), 0.0, tolerance);

  // Test that cross product is anti-commutative
  Vector3d result2 = EigenMath::crossProduct(b, a);
  EXPECT_NEAR((result + result2).norm(), 0.0, tolerance);

  // Test cross product with parallel vectors (should be zero)
  Vector3d c(1, 1, 1);
  Vector3d d(2, 2, 2);  // Parallel to c
  Vector3d result3 = EigenMath::crossProduct(c, d);
  EXPECT_NEAR(result3.norm(), 0.0, tolerance);
}

TEST_F(EigenMathTest, NormalizeVector) {
  VectorXd vector(3);
  vector << 3, 4, 0;  // Length = 5

  VectorXd normalized = EigenMath::normalizeVector(vector);
  EXPECT_NEAR(normalized.norm(), 1.0, tolerance);

  // Check that direction is preserved
  VectorXd expected = vector / vector.norm();
  EXPECT_TRUE((normalized - expected).norm() < tolerance);

  // Test error handling - zero vector
  VectorXd zero_vector = VectorXd::Zero(3);
  EXPECT_THROW(EigenMath::normalizeVector(zero_vector), std::invalid_argument);
}

TEST_F(EigenMathTest, VectorNorm) {
  VectorXd vector(3);
  vector << 3, 4, 0;  // 3-4-5 triangle

  double norm = EigenMath::vectorNorm(vector);
  EXPECT_NEAR(norm, 5.0, tolerance);

  // Test unit vector
  VectorXd unit = EigenMath::normalizeVector(vector);
  double unit_norm = EigenMath::vectorNorm(unit);
  EXPECT_NEAR(unit_norm, 1.0, tolerance);
}

// =============================================================================
// Advanced Linear Algebra Tests
// =============================================================================

TEST_F(EigenMathTest, EigenDecomposition) {
  // Create a symmetric matrix with known eigenvalues
  MatrixXd matrix(3, 3);
  matrix << 4, -2, 0, -2, 4, 0, 0, 0, 6;

  auto [eigenvalues, eigenvectors] = EigenMath::eigenDecomposition(matrix);

  // Check that we have the right number of eigenvalues/vectors
  EXPECT_EQ(eigenvalues.size(), 3);
  EXPECT_EQ(eigenvectors.rows(), 3);
  EXPECT_EQ(eigenvectors.cols(), 3);

  // Verify eigenvalue equation: A * v = λ * v for each eigenvector
  for (int i = 0; i < 3; ++i) {
    VectorXd v = eigenvectors.col(i);
    double lambda = eigenvalues(i);
    VectorXd Av = matrix * v;
    VectorXd lambda_v = lambda * v;
    EXPECT_TRUE((Av - lambda_v).norm() < 1e-10);
  }

  // Test error handling - non-square matrix
  MatrixXd non_square(2, 3);
  EXPECT_THROW(EigenMath::eigenDecomposition(non_square), std::invalid_argument);

  // Test error handling - non-symmetric matrix
  MatrixXd non_symmetric(2, 2);
  non_symmetric << 1, 2, 3, 4;  // 2 != 3
  EXPECT_THROW(EigenMath::eigenDecomposition(non_symmetric), std::invalid_argument);
}

TEST_F(EigenMathTest, SVDDecomposition) {
  MatrixXd matrix(3, 2);
  matrix << 1, 2, 3, 4, 5, 6;

  auto [U, sigma, Vt] = EigenMath::svdDecomposition(matrix);

  // Reconstruct original matrix: A = U * Σ * V^T
  MatrixXd sigma_matrix = MatrixXd::Zero(U.cols(), Vt.rows());
  for (int i = 0; i < sigma.size(); ++i) {
    sigma_matrix(i, i) = sigma(i);
  }
  MatrixXd reconstructed = U * sigma_matrix * Vt;

  EXPECT_TRUE(matricesEqual(matrix, reconstructed, 1e-10));
}

TEST_F(EigenMathTest, SolveLinearSystem) {
  // Test square system with unique solution
  MatrixXd A(3, 3);
  A << 1, 2, 3, 4, 5, 6, 7, 8, 10;  // Make sure it's invertible

  VectorXd b(3);
  b << 14, 32, 53;

  VectorXd x = EigenMath::solveLinearSystem(A, b);
  VectorXd check = A * x;
  EXPECT_TRUE((check - b).norm() < 1e-10);

  // Test error handling - incompatible dimensions
  VectorXd wrong_b(4);
  EXPECT_THROW(EigenMath::solveLinearSystem(A, wrong_b), std::invalid_argument);

  // Test error handling - singular matrix
  MatrixXd singular(2, 2);
  singular << 1, 2, 2, 4;
  VectorXd b_small(2);
  b_small << 1, 2;
  EXPECT_THROW(EigenMath::solveLinearSystem(singular, b_small), std::invalid_argument);
}

TEST_F(EigenMathTest, QRDecomposition) {
  MatrixXd matrix(3, 3);
  matrix << 1, 2, 3, 4, 5, 6, 7, 8, 9;

  auto [Q, R] = EigenMath::qrDecomposition(matrix);

  // Check that Q * R = original matrix
  MatrixXd reconstructed = Q * R;
  EXPECT_TRUE(matricesEqual(matrix, reconstructed, 1e-10));

  // Check that Q is orthogonal (Q^T * Q = I)
  MatrixXd QtQ = Q.transpose() * Q;
  MatrixXd identity = MatrixXd::Identity(Q.cols(), Q.cols());
  EXPECT_TRUE(matricesEqual(QtQ, identity, 1e-10));

  // Check that R is upper triangular
  for (int i = 0; i < R.rows(); ++i) {
    for (int j = 0; j < i && j < R.cols(); ++j) {
      EXPECT_NEAR(R(i, j), 0.0, 1e-10);
    }
  }
}

// =============================================================================
// Geometric Operations Tests
// =============================================================================

TEST_F(EigenMathTest, EulerRotationRoundTrip) {
  // Test round-trip: Euler -> Matrix -> Euler
  double roll = 0.1, pitch = 0.2, yaw = 0.3;

  Matrix3d rotation = EigenMath::eulerToRotationMatrix(roll, pitch, yaw);
  Vector3d recovered_euler = EigenMath::rotationMatrixToEuler(rotation);

  EXPECT_NEAR(recovered_euler(0), roll, 1e-10);
  EXPECT_NEAR(recovered_euler(1), pitch, 1e-10);
  EXPECT_NEAR(recovered_euler(2), yaw, 1e-10);

  // Verify that rotation matrix properties hold
  EXPECT_NEAR(rotation.determinant(), 1.0, tolerance);  // det = 1
  MatrixXd should_be_identity = rotation * rotation.transpose();
  MatrixXd identity = MatrixXd::Identity(3, 3);
  EXPECT_TRUE(matricesEqual(should_be_identity, identity, tolerance));
}

TEST_F(EigenMathTest, CreateTransformationMatrix) {
  Vector3d translation(1, 2, 3);
  Matrix3d rotation = EigenMath::eulerToRotationMatrix(0, 0, M_PI / 2);  // 90° yaw

  Matrix4d transform = EigenMath::createTransformationMatrix(translation, rotation);

  // Check structure of transformation matrix
  EXPECT_TRUE(matricesEqual(transform.block<3, 3>(0, 0), rotation, tolerance));
  EXPECT_TRUE((transform.block<3, 1>(0, 3) - translation).norm() < tolerance);
  EXPECT_NEAR(transform(3, 0), 0.0, tolerance);
  EXPECT_NEAR(transform(3, 1), 0.0, tolerance);
  EXPECT_NEAR(transform(3, 2), 0.0, tolerance);
  EXPECT_NEAR(transform(3, 3), 1.0, tolerance);
}

// =============================================================================
// Utility Function Tests
// =============================================================================

TEST_F(EigenMathTest, MatricesEqual) {
  MatrixXd a = EigenMath::createRandomMatrix(3, 3, 42);
  MatrixXd b = a;
  MatrixXd c = a + MatrixXd::Ones(3, 3) * 1e-5;

  EXPECT_TRUE(EigenMath::matricesEqual(a, b, tolerance));
  EXPECT_FALSE(EigenMath::matricesEqual(a, c, tolerance));
  EXPECT_TRUE(EigenMath::matricesEqual(a, c, 1e-4));

  // Test different dimensions
  MatrixXd d(2, 2);
  EXPECT_FALSE(EigenMath::matricesEqual(a, d, tolerance));
}

TEST_F(EigenMathTest, MatrixInfo) {
  MatrixXd matrix = EigenMath::createIdentityMatrix(3);
  std::string info = EigenMath::matrixInfo(matrix);

  // Just check that we get a non-empty string with expected content
  EXPECT_FALSE(info.empty());
  EXPECT_TRUE(info.find("3 x 3") != std::string::npos);
  EXPECT_TRUE(info.find("Determinant") != std::string::npos);
}

TEST_F(EigenMathTest, BenchmarkMatrixMultiplication) {
  double time_ms = EigenMath::benchmarkMatrixMultiplication(10, 5);
  EXPECT_GT(time_ms, 0.0);     // Should take some measurable time
  EXPECT_LT(time_ms, 1000.0);  // Should not take more than 1 second for small matrices

  // Test error handling
  EXPECT_THROW(EigenMath::benchmarkMatrixMultiplication(0, 10), std::invalid_argument);
  EXPECT_THROW(EigenMath::benchmarkMatrixMultiplication(10, 0), std::invalid_argument);
}