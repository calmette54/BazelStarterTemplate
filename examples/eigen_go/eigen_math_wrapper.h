#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Error Codes
// ============================================================================

typedef enum {
    EIGEN_SUCCESS = 0,
    EIGEN_ERROR_INVALID_ARGUMENT = -1,
    EIGEN_ERROR_INCOMPATIBLE_DIMENSIONS = -2,
    EIGEN_ERROR_NOT_INVERTIBLE = -3,
    EIGEN_ERROR_ZERO_VECTOR = -4,
    EIGEN_ERROR_MEMORY_ALLOCATION = -5,
    EIGEN_ERROR_UNKNOWN = -99
} EigenResult;

// ============================================================================
// Opaque Handle Types
// ============================================================================

// Forward declarations for opaque handles
struct EigenMatrixHandleImpl;
struct EigenVectorHandleImpl;

// Opaque handles for C++ objects - we don't expose the actual types to C
typedef struct EigenMatrixHandleImpl* EigenMatrixHandle;
typedef struct EigenVectorHandleImpl* EigenVectorHandle;

// ============================================================================
// Matrix Operations
// ============================================================================

/**
 * @brief Create a random matrix with specified dimensions
 * @param rows Number of rows
 * @param cols Number of columns
 * @param seed Random seed for reproducibility
 * @param handle Pointer to store the created matrix handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_create_random_matrix(int rows, int cols, int seed, EigenMatrixHandle* handle);

/**
 * @brief Create an identity matrix
 * @param size Matrix dimension (square matrix)
 * @param handle Pointer to store the created matrix handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_create_identity_matrix(int size, EigenMatrixHandle* handle);

/**
 * @brief Create a matrix from raw data
 * @param data Raw matrix data in row-major order
 * @param rows Number of rows
 * @param cols Number of columns
 * @param handle Pointer to store the created matrix handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_create_matrix_from_data(const double* data, int rows, int cols, EigenMatrixHandle* handle);

/**
 * @brief Get matrix data as raw array
 * @param handle Matrix handle
 * @param data Pointer to store matrix data (caller must allocate rows*cols doubles)
 * @param rows Pointer to store number of rows
 * @param cols Pointer to store number of columns
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_get_matrix_data(EigenMatrixHandle handle, double* data, int* rows, int* cols);

/**
 * @brief Get matrix dimensions
 * @param handle Matrix handle
 * @param rows Pointer to store number of rows
 * @param cols Pointer to store number of columns
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_get_matrix_dimensions(EigenMatrixHandle handle, int* rows, int* cols);

/**
 * @brief Multiply two matrices
 * @param a First matrix handle
 * @param b Second matrix handle
 * @param result Pointer to store result matrix handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_multiply_matrices(EigenMatrixHandle a, EigenMatrixHandle b, EigenMatrixHandle* result);

/**
 * @brief Transpose a matrix
 * @param matrix Input matrix handle
 * @param result Pointer to store result matrix handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_transpose_matrix(EigenMatrixHandle matrix, EigenMatrixHandle* result);

/**
 * @brief Compute matrix inverse
 * @param matrix Input matrix handle (must be square)
 * @param result Pointer to store result matrix handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_invert_matrix(EigenMatrixHandle matrix, EigenMatrixHandle* result);

/**
 * @brief Compute matrix determinant
 * @param matrix Input matrix handle (must be square)
 * @param determinant Pointer to store determinant value
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_determinant(EigenMatrixHandle matrix, double* determinant);

/**
 * @brief Free a matrix handle and its associated memory
 * @param handle Matrix handle to free
 */
void eigen_free_matrix(EigenMatrixHandle handle);

// ============================================================================
// Vector Operations
// ============================================================================

/**
 * @brief Create a random vector with specified dimension
 * @param size Vector dimension
 * @param seed Random seed for reproducibility
 * @param handle Pointer to store the created vector handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_create_random_vector(int size, int seed, EigenVectorHandle* handle);

/**
 * @brief Create a vector from raw data
 * @param data Raw vector data
 * @param size Vector size
 * @param handle Pointer to store the created vector handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_create_vector_from_data(const double* data, int size, EigenVectorHandle* handle);

/**
 * @brief Get vector data as raw array
 * @param handle Vector handle
 * @param data Pointer to store vector data (caller must allocate size doubles)
 * @param size Pointer to store vector size
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_get_vector_data(EigenVectorHandle handle, double* data, int* size);

/**
 * @brief Get vector size
 * @param handle Vector handle
 * @param size Pointer to store vector size
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_get_vector_size(EigenVectorHandle handle, int* size);

/**
 * @brief Compute dot product of two vectors
 * @param a First vector handle
 * @param b Second vector handle
 * @param result Pointer to store dot product
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_dot_product(EigenVectorHandle a, EigenVectorHandle b, double* result);

/**
 * @brief Compute cross product of two 3D vectors
 * @param a First 3D vector handle
 * @param b Second 3D vector handle
 * @param result Pointer to store result vector handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_cross_product(EigenVectorHandle a, EigenVectorHandle b, EigenVectorHandle* result);

/**
 * @brief Normalize a vector (make unit length)
 * @param vector Input vector handle
 * @param result Pointer to store result vector handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_normalize_vector(EigenVectorHandle vector, EigenVectorHandle* result);

/**
 * @brief Compute Euclidean norm of a vector
 * @param vector Input vector handle
 * @param norm Pointer to store norm value
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_vector_norm(EigenVectorHandle vector, double* norm);

/**
 * @brief Free a vector handle and its associated memory
 * @param handle Vector handle to free
 */
void eigen_free_vector(EigenVectorHandle handle);

// ============================================================================
// Advanced Linear Algebra
// ============================================================================

/**
 * @brief Compute eigenvalues and eigenvectors of a symmetric matrix
 * @param matrix Input symmetric matrix handle
 * @param eigenvalues Pointer to store eigenvalues vector handle
 * @param eigenvectors Pointer to store eigenvectors matrix handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_eigen_decomposition(EigenMatrixHandle matrix, EigenVectorHandle* eigenvalues, EigenMatrixHandle* eigenvectors);

/**
 * @brief Compute Singular Value Decomposition (SVD)
 * @param matrix Input matrix handle
 * @param U Pointer to store U matrix handle
 * @param singular_values Pointer to store singular values vector handle
 * @param Vt Pointer to store V transpose matrix handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_svd_decomposition(EigenMatrixHandle matrix, EigenMatrixHandle* U, EigenVectorHandle* singular_values, EigenMatrixHandle* Vt);

/**
 * @brief Solve linear system Ax = b
 * @param A Coefficient matrix handle
 * @param b Right-hand side vector handle
 * @param x Pointer to store solution vector handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_solve_linear_system(EigenMatrixHandle A, EigenVectorHandle b, EigenVectorHandle* x);

/**
 * @brief Compute QR decomposition
 * @param matrix Input matrix handle
 * @param Q Pointer to store Q matrix handle
 * @param R Pointer to store R matrix handle
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_qr_decomposition(EigenMatrixHandle matrix, EigenMatrixHandle* Q, EigenMatrixHandle* R);

// ============================================================================
// Geometric Operations
// ============================================================================

/**
 * @brief Create a rotation matrix from Euler angles (Z-Y-X convention)
 * @param roll Rotation around X-axis (radians)
 * @param pitch Rotation around Y-axis (radians)
 * @param yaw Rotation around Z-axis (radians)
 * @param result Pointer to store result matrix handle (3x3)
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_euler_to_rotation_matrix(double roll, double pitch, double yaw, EigenMatrixHandle* result);

/**
 * @brief Extract Euler angles from rotation matrix
 * @param rotation 3x3 rotation matrix handle
 * @param roll Pointer to store roll angle (radians)
 * @param pitch Pointer to store pitch angle (radians)
 * @param yaw Pointer to store yaw angle (radians)
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_rotation_matrix_to_euler(EigenMatrixHandle rotation, double* roll, double* pitch, double* yaw);

/**
 * @brief Create a transformation matrix from translation and rotation
 * @param tx Translation X component
 * @param ty Translation Y component
 * @param tz Translation Z component
 * @param rotation 3x3 rotation matrix handle
 * @param result Pointer to store result matrix handle (4x4)
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_create_transformation_matrix(double tx, double ty, double tz, EigenMatrixHandle rotation, EigenMatrixHandle* result);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Check if two matrices are approximately equal
 * @param a First matrix handle
 * @param b Second matrix handle
 * @param tolerance Numerical tolerance
 * @param result Pointer to store comparison result (1 if equal, 0 if not)
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_matrices_equal(EigenMatrixHandle a, EigenMatrixHandle b, double tolerance, int* result);

/**
 * @brief Get matrix properties as string
 * @param matrix Input matrix handle
 * @param info Buffer to store matrix info string (caller must allocate)
 * @param buffer_size Size of the info buffer
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_matrix_info(EigenMatrixHandle matrix, char* info, int buffer_size);

/**
 * @brief Benchmark matrix multiplication performance
 * @param size Matrix size for square matrices
 * @param iterations Number of iterations
 * @param time_ms Pointer to store time in milliseconds per operation
 * @return EIGEN_SUCCESS on success, error code on failure
 */
EigenResult eigen_benchmark_matrix_multiplication(int size, int iterations, double* time_ms);

// ============================================================================
// Error Handling
// ============================================================================

/**
 * @brief Get error message for a given error code
 * @param error_code Error code from EigenResult
 * @return Human-readable error message string
 */
const char* eigen_get_error_message(EigenResult error_code);

#ifdef __cplusplus
}
#endif