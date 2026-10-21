#include "eigen_math_wrapper.h"
#include "eigen_math.h"
#include <memory>
#include <cstring>
#include <sstream>

using namespace eigen_math;

// ============================================================================
// Internal Handle Structures
// ============================================================================

struct EigenMatrixHandleImpl {
    std::unique_ptr<EigenMath::MatrixXd> matrix;
    
    EigenMatrixHandleImpl(std::unique_ptr<EigenMath::MatrixXd> m) : matrix(std::move(m)) {}
};

struct EigenVectorHandleImpl {
    std::unique_ptr<EigenMath::VectorXd> vector;
    
    EigenVectorHandleImpl(std::unique_ptr<EigenMath::VectorXd> v) : vector(std::move(v)) {}
};

// ============================================================================
// Error Handling Utilities
// ============================================================================

const char* eigen_get_error_message(EigenResult error_code) {
    switch (error_code) {
        case EIGEN_SUCCESS:
            return "Success";
        case EIGEN_ERROR_INVALID_ARGUMENT:
            return "Invalid argument";
        case EIGEN_ERROR_INCOMPATIBLE_DIMENSIONS:
            return "Incompatible matrix/vector dimensions";
        case EIGEN_ERROR_NOT_INVERTIBLE:
            return "Matrix is not invertible";
        case EIGEN_ERROR_ZERO_VECTOR:
            return "Cannot normalize zero vector";
        case EIGEN_ERROR_MEMORY_ALLOCATION:
            return "Memory allocation failed";
        case EIGEN_ERROR_UNKNOWN:
        default:
            return "Unknown error";
    }
}

// Helper function to safely execute C++ code and convert exceptions to error codes
template<typename Func>
EigenResult safe_execute(Func&& func) {
    try {
        func();
        return EIGEN_SUCCESS;
    } catch (const std::invalid_argument& e) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    } catch (const std::bad_alloc& e) {
        return EIGEN_ERROR_MEMORY_ALLOCATION;
    } catch (...) {
        return EIGEN_ERROR_UNKNOWN;
    }
}

// ============================================================================
// Matrix Operations
// ============================================================================

EigenResult eigen_create_random_matrix(int rows, int cols, int seed, EigenMatrixHandle* handle) {
    if (!handle || rows <= 0 || cols <= 0) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto matrix = std::make_unique<EigenMath::MatrixXd>(
            EigenMath::createRandomMatrix(rows, cols, seed)
        );
        *handle = new EigenMatrixHandleImpl(std::move(matrix));
    });
}

EigenResult eigen_create_identity_matrix(int size, EigenMatrixHandle* handle) {
    if (!handle || size <= 0) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto matrix = std::make_unique<EigenMath::MatrixXd>(
            EigenMath::createIdentityMatrix(size)
        );
        *handle = new EigenMatrixHandleImpl(std::move(matrix));
    });
}

EigenResult eigen_create_matrix_from_data(const double* data, int rows, int cols, EigenMatrixHandle* handle) {
    if (!data || !handle || rows <= 0 || cols <= 0) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto matrix = std::make_unique<EigenMath::MatrixXd>(rows, cols);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                (*matrix)(i, j) = data[i * cols + j];
            }
        }
        *handle = new EigenMatrixHandleImpl(std::move(matrix));
    });
}

EigenResult eigen_get_matrix_data(EigenMatrixHandle handle, double* data, int* rows, int* cols) {
    if (!handle || !data || !rows || !cols) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        const auto& matrix = *handle->matrix;
        *rows = static_cast<int>(matrix.rows());
        *cols = static_cast<int>(matrix.cols());
        
        for (int i = 0; i < *rows; ++i) {
            for (int j = 0; j < *cols; ++j) {
                data[i * (*cols) + j] = matrix(i, j);
            }
        }
    });
}

EigenResult eigen_get_matrix_dimensions(EigenMatrixHandle handle, int* rows, int* cols) {
    if (!handle || !rows || !cols) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        const auto& matrix = *handle->matrix;
        *rows = static_cast<int>(matrix.rows());
        *cols = static_cast<int>(matrix.cols());
    });
}

EigenResult eigen_multiply_matrices(EigenMatrixHandle a, EigenMatrixHandle b, EigenMatrixHandle* result) {
    if (!a || !b || !result) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        const auto& mat_a = *a->matrix;
        const auto& mat_b = *b->matrix;
        
        if (mat_a.cols() != mat_b.rows()) {
            throw std::invalid_argument("Matrix dimensions incompatible for multiplication");
        }
        
        auto result_matrix = std::make_unique<EigenMath::MatrixXd>(
            EigenMath::multiplyMatrices(mat_a, mat_b)
        );
        *result = new EigenMatrixHandleImpl(std::move(result_matrix));
    });
}

EigenResult eigen_transpose_matrix(EigenMatrixHandle matrix, EigenMatrixHandle* result) {
    if (!matrix || !result) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto result_matrix = std::make_unique<EigenMath::MatrixXd>(
            EigenMath::transposeMatrix(*matrix->matrix)
        );
        *result = new EigenMatrixHandleImpl(std::move(result_matrix));
    });
}

EigenResult eigen_invert_matrix(EigenMatrixHandle matrix, EigenMatrixHandle* result) {
    if (!matrix || !result) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto result_matrix = std::make_unique<EigenMath::MatrixXd>(
            EigenMath::invertMatrix(*matrix->matrix)
        );
        *result = new EigenMatrixHandleImpl(std::move(result_matrix));
    });
}

EigenResult eigen_determinant(EigenMatrixHandle matrix, double* determinant) {
    if (!matrix || !determinant) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        *determinant = EigenMath::determinant(*matrix->matrix);
    });
}

void eigen_free_matrix(EigenMatrixHandle handle) {
    delete handle;
}

// ============================================================================
// Vector Operations
// ============================================================================

EigenResult eigen_create_random_vector(int size, int seed, EigenVectorHandle* handle) {
    if (!handle || size <= 0) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto vector = std::make_unique<EigenMath::VectorXd>(
            EigenMath::createRandomVector(size, seed)
        );
        *handle = new EigenVectorHandleImpl(std::move(vector));
    });
}

EigenResult eigen_create_vector_from_data(const double* data, int size, EigenVectorHandle* handle) {
    if (!data || !handle || size <= 0) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto vector = std::make_unique<EigenMath::VectorXd>(size);
        for (int i = 0; i < size; ++i) {
            (*vector)(i) = data[i];
        }
        *handle = new EigenVectorHandleImpl(std::move(vector));
    });
}

EigenResult eigen_get_vector_data(EigenVectorHandle handle, double* data, int* size) {
    if (!handle || !data || !size) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        const auto& vector = *handle->vector;
        *size = static_cast<int>(vector.size());
        
        for (int i = 0; i < *size; ++i) {
            data[i] = vector(i);
        }
    });
}

EigenResult eigen_get_vector_size(EigenVectorHandle handle, int* size) {
    if (!handle || !size) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        const auto& vector = *handle->vector;
        *size = static_cast<int>(vector.size());
    });
}

EigenResult eigen_dot_product(EigenVectorHandle a, EigenVectorHandle b, double* result) {
    if (!a || !b || !result) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        *result = EigenMath::dotProduct(*a->vector, *b->vector);
    });
}

EigenResult eigen_cross_product(EigenVectorHandle a, EigenVectorHandle b, EigenVectorHandle* result) {
    if (!a || !b || !result) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        const auto& vec_a = *a->vector;
        const auto& vec_b = *b->vector;
        
        if (vec_a.size() != 3 || vec_b.size() != 3) {
            throw std::invalid_argument("Cross product requires 3D vectors");
        }
        
        EigenMath::Vector3d a3d(vec_a(0), vec_a(1), vec_a(2));
        EigenMath::Vector3d b3d(vec_b(0), vec_b(1), vec_b(2));
        
        EigenMath::Vector3d cross_result = EigenMath::crossProduct(a3d, b3d);
        
        auto result_vector = std::make_unique<EigenMath::VectorXd>(3);
        (*result_vector)(0) = cross_result(0);
        (*result_vector)(1) = cross_result(1);
        (*result_vector)(2) = cross_result(2);
        
        *result = new EigenVectorHandleImpl(std::move(result_vector));
    });
}

EigenResult eigen_normalize_vector(EigenVectorHandle vector, EigenVectorHandle* result) {
    if (!vector || !result) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto result_vector = std::make_unique<EigenMath::VectorXd>(
            EigenMath::normalizeVector(*vector->vector)
        );
        *result = new EigenVectorHandleImpl(std::move(result_vector));
    });
}

EigenResult eigen_vector_norm(EigenVectorHandle vector, double* norm) {
    if (!vector || !norm) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        *norm = EigenMath::vectorNorm(*vector->vector);
    });
}

void eigen_free_vector(EigenVectorHandle handle) {
    delete handle;
}

// ============================================================================
// Advanced Linear Algebra
// ============================================================================

EigenResult eigen_eigen_decomposition(EigenMatrixHandle matrix, EigenVectorHandle* eigenvalues, EigenMatrixHandle* eigenvectors) {
    if (!matrix || !eigenvalues || !eigenvectors) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto decomp_result = EigenMath::eigenDecomposition(*matrix->matrix);
        
        auto vals = std::make_unique<EigenMath::VectorXd>(decomp_result.first);
        auto vecs = std::make_unique<EigenMath::MatrixXd>(decomp_result.second);
        
        *eigenvalues = new EigenVectorHandleImpl(std::move(vals));
        *eigenvectors = new EigenMatrixHandleImpl(std::move(vecs));
    });
}

EigenResult eigen_svd_decomposition(EigenMatrixHandle matrix, EigenMatrixHandle* U, EigenVectorHandle* singular_values, EigenMatrixHandle* Vt) {
    if (!matrix || !U || !singular_values || !Vt) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto svd_result = EigenMath::svdDecomposition(*matrix->matrix);
        
        auto u_matrix = std::make_unique<EigenMath::MatrixXd>(std::get<0>(svd_result));
        auto s_vector = std::make_unique<EigenMath::VectorXd>(std::get<1>(svd_result));
        auto vt_matrix = std::make_unique<EigenMath::MatrixXd>(std::get<2>(svd_result));
        
        *U = new EigenMatrixHandleImpl(std::move(u_matrix));
        *singular_values = new EigenVectorHandleImpl(std::move(s_vector));
        *Vt = new EigenMatrixHandleImpl(std::move(vt_matrix));
    });
}

EigenResult eigen_solve_linear_system(EigenMatrixHandle A, EigenVectorHandle b, EigenVectorHandle* x) {
    if (!A || !b || !x) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto solution = std::make_unique<EigenMath::VectorXd>(
            EigenMath::solveLinearSystem(*A->matrix, *b->vector)
        );
        *x = new EigenVectorHandleImpl(std::move(solution));
    });
}

EigenResult eigen_qr_decomposition(EigenMatrixHandle matrix, EigenMatrixHandle* Q, EigenMatrixHandle* R) {
    if (!matrix || !Q || !R) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto qr_result = EigenMath::qrDecomposition(*matrix->matrix);
        
        auto q_matrix = std::make_unique<EigenMath::MatrixXd>(qr_result.first);
        auto r_matrix = std::make_unique<EigenMath::MatrixXd>(qr_result.second);
        
        *Q = new EigenMatrixHandleImpl(std::move(q_matrix));
        *R = new EigenMatrixHandleImpl(std::move(r_matrix));
    });
}

// ============================================================================
// Geometric Operations  
// ============================================================================

EigenResult eigen_euler_to_rotation_matrix(double roll, double pitch, double yaw, EigenMatrixHandle* result) {
    if (!result) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        EigenMath::Matrix3d rot3d = EigenMath::eulerToRotationMatrix(roll, pitch, yaw);
        
        // Convert Matrix3d to MatrixXd
        auto result_matrix = std::make_unique<EigenMath::MatrixXd>(3, 3);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                (*result_matrix)(i, j) = rot3d(i, j);
            }
        }
        
        *result = new EigenMatrixHandleImpl(std::move(result_matrix));
    });
}

EigenResult eigen_rotation_matrix_to_euler(EigenMatrixHandle rotation, double* roll, double* pitch, double* yaw) {
    if (!rotation || !roll || !pitch || !yaw) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        const auto& mat = *rotation->matrix;
        if (mat.rows() != 3 || mat.cols() != 3) {
            throw std::invalid_argument("Rotation matrix must be 3x3");
        }
        
        // Convert MatrixXd to Matrix3d
        EigenMath::Matrix3d rot3d;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                rot3d(i, j) = mat(i, j);
            }
        }
        
        EigenMath::Vector3d euler = EigenMath::rotationMatrixToEuler(rot3d);
        *roll = euler(0);
        *pitch = euler(1);
        *yaw = euler(2);
    });
}

EigenResult eigen_create_transformation_matrix(double tx, double ty, double tz, EigenMatrixHandle rotation, EigenMatrixHandle* result) {
    if (!rotation || !result) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        const auto& rot_mat = *rotation->matrix;
        if (rot_mat.rows() != 3 || rot_mat.cols() != 3) {
            throw std::invalid_argument("Rotation matrix must be 3x3");
        }
        
        // Convert MatrixXd to Matrix3d and Vector3d
        EigenMath::Matrix3d rot3d;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                rot3d(i, j) = rot_mat(i, j);
            }
        }
        
        EigenMath::Vector3d translation(tx, ty, tz);
        EigenMath::Matrix4d transform4d = EigenMath::createTransformationMatrix(translation, rot3d);
        
        // Convert Matrix4d to MatrixXd
        auto result_matrix = std::make_unique<EigenMath::MatrixXd>(4, 4);
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                (*result_matrix)(i, j) = transform4d(i, j);
            }
        }
        
        *result = new EigenMatrixHandleImpl(std::move(result_matrix));
    });
}

// ============================================================================
// Utility Functions
// ============================================================================

EigenResult eigen_matrices_equal(EigenMatrixHandle a, EigenMatrixHandle b, double tolerance, int* result) {
    if (!a || !b || !result) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        bool are_equal = EigenMath::matricesEqual(*a->matrix, *b->matrix, tolerance);
        *result = are_equal ? 1 : 0;
    });
}

EigenResult eigen_matrix_info(EigenMatrixHandle matrix, char* info, int buffer_size) {
    if (!matrix || !info || buffer_size <= 0) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        std::string info_str = EigenMath::matrixInfo(*matrix->matrix);
        
        // Safely copy string to buffer
        size_t copy_length = std::min(static_cast<size_t>(buffer_size - 1), info_str.length());
        std::strncpy(info, info_str.c_str(), copy_length);
        info[copy_length] = '\0';
    });
}

EigenResult eigen_benchmark_matrix_multiplication(int size, int iterations, double* time_ms) {
    if (size <= 0 || iterations <= 0 || !time_ms) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        *time_ms = EigenMath::benchmarkMatrixMultiplication(size, iterations);
    });
}