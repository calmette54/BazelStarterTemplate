"""
Comprehensive test suite for eigen_bindings Python module.

Tests the pybind11 bindings that expose Eigen mathematical operations to Python,
with particular focus on NumPy integration and numerical accuracy.
"""

import numpy as np
import pytest
import sys
import os

# Add the parent directory to the path to import the bindings
# This assumes the test is run from the correct context where the bindings are available
try:
    import eigen_bindings as eb
except ImportError as e:
    pytest.skip(f"eigen_bindings module not available: {e}", allow_module_level=True)


class TestEigenMath:
    """Test suite for EigenMath class and module functions."""
    
    def setup_method(self):
        """Set up test fixtures."""
        self.tolerance = 1e-10
        self.np_tolerance = 1e-12
    
    def assert_arrays_close(self, a, b, rtol=1e-12, atol=1e-12):
        """Helper to assert numpy arrays are close."""
        np.testing.assert_allclose(a, b, rtol=rtol, atol=atol)
    
    # =============================================================================
    # Matrix Operations Tests
    # =============================================================================
    
    def test_create_random_matrix(self):
        """Test random matrix creation."""
        # Basic functionality
        matrix = eb.create_random_matrix(3, 4, seed=42)
        assert matrix.shape == (3, 4)
        assert matrix.dtype == np.float64
        
        # Reproducibility with same seed
        matrix2 = eb.create_random_matrix(3, 4, seed=42)
        self.assert_arrays_close(matrix, matrix2)
        
        # Different results with different seed
        matrix3 = eb.create_random_matrix(3, 4, seed=43)
        assert not np.allclose(matrix, matrix3)
        
        # Values should be in expected range [-1, 1]
        assert np.all(matrix >= -1.0)
        assert np.all(matrix <= 1.0)
        
        # Test error handling
        with pytest.raises(ValueError):
            eb.create_random_matrix(0, 4)
        with pytest.raises(ValueError):
            eb.create_random_matrix(3, 0)
        with pytest.raises(ValueError):
            eb.create_random_matrix(-1, 4)
    
    def test_create_identity_matrix(self):
        """Test identity matrix creation."""
        identity = eb.EigenMath.create_identity_matrix(3)
        expected = np.eye(3)
        self.assert_arrays_close(identity, expected)
        
        # Test 1x1 case
        identity1 = eb.EigenMath.create_identity_matrix(1)
        assert identity1[0, 0] == 1.0
        
        # Error handling
        with pytest.raises(ValueError):
            eb.EigenMath.create_identity_matrix(0)
        with pytest.raises(ValueError):
            eb.EigenMath.create_identity_matrix(-1)
    
    def test_multiply_matrices(self):
        """Test matrix multiplication."""
        a = np.array([[1, 2, 3],
                      [4, 5, 6]], dtype=np.float64)
        b = np.array([[7, 8],
                      [9, 10],
                      [11, 12]], dtype=np.float64)
        
        result = eb.multiply_matrices(a, b)
        expected = np.dot(a, b)
        self.assert_arrays_close(result, expected)
        
        # Test with identity
        identity = eb.EigenMath.create_identity_matrix(3)
        c = eb.create_random_matrix(3, 3, seed=42)
        result2 = eb.multiply_matrices(identity, c)
        self.assert_arrays_close(result2, c)
        
        # Test error handling - incompatible dimensions
        d = np.random.rand(2, 3)
        e = np.random.rand(4, 2)  # 3 != 4
        with pytest.raises(ValueError):
            eb.multiply_matrices(d, e)
    
    def test_transpose_matrix(self):
        """Test matrix transposition."""
        matrix = np.array([[1, 2, 3],
                          [4, 5, 6]], dtype=np.float64)
        
        result = eb.EigenMath.transpose_matrix(matrix)
        expected = matrix.T
        self.assert_arrays_close(result, expected)
        
        # Double transpose should equal original
        double_transpose = eb.EigenMath.transpose_matrix(result)
        self.assert_arrays_close(double_transpose, matrix)
    
    def test_invert_matrix(self):
        """Test matrix inversion."""
        # Test with known invertible matrix
        matrix = np.array([[1, 2],
                          [3, 4]], dtype=np.float64)
        
        inverse = eb.invert_matrix(matrix)
        product = eb.multiply_matrices(matrix, inverse)
        identity = np.eye(2)
        self.assert_arrays_close(product, identity, rtol=1e-10, atol=1e-10)
        
        # Compare with numpy inverse
        np_inverse = np.linalg.inv(matrix)
        self.assert_arrays_close(inverse, np_inverse, rtol=1e-12, atol=1e-12)
        
        # Test error handling - non-square matrix
        non_square = np.random.rand(2, 3)
        with pytest.raises(ValueError):
            eb.invert_matrix(non_square)
        
        # Test error handling - singular matrix
        singular = np.array([[1, 2],
                            [2, 4]], dtype=np.float64)  # Row 2 = 2 * Row 1
        with pytest.raises(ValueError):
            eb.invert_matrix(singular)
    
    def test_determinant(self):
        """Test determinant computation."""
        # Test 2x2 determinant
        matrix = np.array([[1, 2],
                          [3, 4]], dtype=np.float64)
        det = eb.EigenMath.determinant(matrix)
        expected = np.linalg.det(matrix)
        assert abs(det - expected) < self.tolerance
        
        # Test identity matrix
        identity = eb.EigenMath.create_identity_matrix(3)
        det_identity = eb.EigenMath.determinant(identity)
        assert abs(det_identity - 1.0) < self.tolerance
        
        # Test error handling
        non_square = np.random.rand(2, 3)
        with pytest.raises(ValueError):
            eb.EigenMath.determinant(non_square)
    
    # =============================================================================
    # Vector Operations Tests
    # =============================================================================
    
    def test_create_random_vector(self):
        """Test random vector creation."""
        vector = eb.EigenMath.create_random_vector(5, seed=42)
        assert vector.shape == (5,)
        assert vector.dtype == np.float64
        
        # Reproducibility
        vector2 = eb.EigenMath.create_random_vector(5, seed=42)
        self.assert_arrays_close(vector, vector2)
        
        # Values in expected range
        assert np.all(vector >= -1.0)
        assert np.all(vector <= 1.0)
        
        # Error handling
        with pytest.raises(ValueError):
            eb.EigenMath.create_random_vector(0)
    
    def test_dot_product(self):
        """Test dot product computation."""
        a = np.array([1, 2, 3], dtype=np.float64)
        b = np.array([4, 5, 6], dtype=np.float64)
        
        result = eb.EigenMath.dot_product(a, b)
        expected = np.dot(a, b)
        assert abs(result - expected) < self.tolerance
        
        # Test with orthogonal vectors
        c = np.array([1, 0], dtype=np.float64)
        d = np.array([0, 1], dtype=np.float64)
        dot_orthogonal = eb.EigenMath.dot_product(c, d)
        assert abs(dot_orthogonal) < self.tolerance
        
        # Error handling - different sizes
        e = np.random.rand(3)
        f = np.random.rand(4)
        with pytest.raises(ValueError):
            eb.EigenMath.dot_product(e, f)
    
    def test_cross_product(self):
        """Test cross product computation."""
        # Unit vectors
        a = np.array([1, 0, 0], dtype=np.float64)  # x-axis
        b = np.array([0, 1, 0], dtype=np.float64)  # y-axis
        
        result = eb.EigenMath.cross_product(a, b)
        expected = np.array([0, 0, 1], dtype=np.float64)  # z-axis
        self.assert_arrays_close(result, expected)
        
        # Compare with numpy
        np_cross = np.cross(a, b)
        self.assert_arrays_close(result, np_cross)
        
        # Anti-commutativity: a × b = -(b × a)
        result2 = eb.EigenMath.cross_product(b, a)
        self.assert_arrays_close(result, -result2)
        
        # Parallel vectors should give zero
        c = np.array([1, 1, 1], dtype=np.float64)
        d = np.array([2, 2, 2], dtype=np.float64)
        result3 = eb.EigenMath.cross_product(c, d)
        assert np.linalg.norm(result3) < self.tolerance
    
    def test_normalize_vector(self):
        """Test vector normalization."""
        vector = np.array([3, 4, 0], dtype=np.float64)  # Length = 5
        
        normalized = eb.EigenMath.normalize_vector(vector)
        assert abs(np.linalg.norm(normalized) - 1.0) < self.tolerance
        
        # Compare with numpy normalization
        expected = vector / np.linalg.norm(vector)
        self.assert_arrays_close(normalized, expected)
        
        # Error handling - zero vector
        zero_vector = np.zeros(3, dtype=np.float64)
        with pytest.raises(ValueError):
            eb.EigenMath.normalize_vector(zero_vector)
    
    def test_vector_norm(self):
        """Test vector norm computation."""
        vector = np.array([3, 4, 0], dtype=np.float64)  # 3-4-5 triangle
        
        norm = eb.EigenMath.vector_norm(vector)
        expected = np.linalg.norm(vector)
        assert abs(norm - expected) < self.tolerance
        assert abs(norm - 5.0) < self.tolerance
    
    # =============================================================================
    # Advanced Linear Algebra Tests
    # =============================================================================
    
    def test_eigen_decomposition(self):
        """Test eigenvalue decomposition."""
        # Create symmetric matrix with known eigenvalues
        matrix = np.array([[4, -2, 0],
                          [-2, 4, 0],
                          [0, 0, 6]], dtype=np.float64)
        
        eigenvalues, eigenvectors = eb.EigenMath.eigen_decomposition(matrix)
        
        # Compare with numpy
        np_eigenvals, np_eigenvecs = np.linalg.eigh(matrix)
        
        # Sort both results for comparison (eigenvalues might be in different order)
        sort_idx = np.argsort(eigenvalues)
        eigenvalues_sorted = eigenvalues[sort_idx]
        eigenvectors_sorted = eigenvectors[:, sort_idx]
        
        np_sort_idx = np.argsort(np_eigenvals)
        np_eigenvals_sorted = np_eigenvals[np_sort_idx]
        np_eigenvecs_sorted = np_eigenvecs[:, np_sort_idx]
        
        self.assert_arrays_close(eigenvalues_sorted, np_eigenvals_sorted, rtol=1e-10)
        
        # Eigenvectors might differ by sign, so check the eigenvalue equation
        for i in range(len(eigenvalues)):
            v = eigenvectors[:, i]
            lam = eigenvalues[i]
            Av = np.dot(matrix, v)
            lam_v = lam * v
            self.assert_arrays_close(Av, lam_v, rtol=1e-10, atol=1e-10)
        
        # Error handling - non-square matrix
        non_square = np.random.rand(2, 3)
        with pytest.raises(ValueError):
            eb.EigenMath.eigen_decomposition(non_square)
        
        # Error handling - non-symmetric matrix
        non_symmetric = np.array([[1, 2], [3, 4]], dtype=np.float64)
        with pytest.raises(ValueError):
            eb.EigenMath.eigen_decomposition(non_symmetric)
    
    def test_svd_decomposition(self):
        """Test SVD decomposition."""
        matrix = np.array([[1, 2],
                          [3, 4],
                          [5, 6]], dtype=np.float64)
        
        U, sigma, Vt = eb.EigenMath.svd_decomposition(matrix)
        
        # Reconstruct original matrix
        sigma_matrix = np.zeros((U.shape[1], Vt.shape[0]))
        np.fill_diagonal(sigma_matrix, sigma)
        reconstructed = np.dot(U, np.dot(sigma_matrix, Vt))
        
        self.assert_arrays_close(matrix, reconstructed, rtol=1e-12, atol=1e-12)
        
        # Compare with numpy SVD
        np_U, np_sigma, np_Vt = np.linalg.svd(matrix, full_matrices=True)
        self.assert_arrays_close(sigma, np_sigma, rtol=1e-12)
    
    def test_solve_linear_system(self):
        """Test linear system solving."""
        # Create well-conditioned system
        A = np.array([[1, 2, 3],
                     [4, 5, 6],
                     [7, 8, 10]], dtype=np.float64)  # Make sure it's invertible
        b = np.array([14, 32, 53], dtype=np.float64)
        
        x = eb.solve_linear_system(A, b)
        
        # Verify solution
        check = np.dot(A, x)
        self.assert_arrays_close(check, b, rtol=1e-10, atol=1e-10)
        
        # Compare with numpy
        np_x = np.linalg.solve(A, b)
        self.assert_arrays_close(x, np_x, rtol=1e-12, atol=1e-12)
        
        # Error handling - incompatible dimensions
        wrong_b = np.random.rand(4)
        with pytest.raises(ValueError):
            eb.solve_linear_system(A, wrong_b)
        
        # Error handling - singular matrix
        singular = np.array([[1, 2], [2, 4]], dtype=np.float64)
        b_small = np.array([1, 2], dtype=np.float64)
        with pytest.raises(ValueError):
            eb.solve_linear_system(singular, b_small)
    
    def test_qr_decomposition(self):
        """Test QR decomposition."""
        matrix = np.array([[1, 2, 3],
                          [4, 5, 6],
                          [7, 8, 9]], dtype=np.float64)
        
        Q, R = eb.EigenMath.qr_decomposition(matrix)
        
        # Check reconstruction
        reconstructed = np.dot(Q, R)
        self.assert_arrays_close(matrix, reconstructed, rtol=1e-12, atol=1e-12)
        
        # Check that Q is orthogonal
        QtQ = np.dot(Q.T, Q)
        identity = np.eye(Q.shape[1])
        self.assert_arrays_close(QtQ, identity, rtol=1e-12, atol=1e-12)
        
        # Check that R is upper triangular
        for i in range(R.shape[0]):
            for j in range(min(i, R.shape[1])):
                assert abs(R[i, j]) < 1e-12, f"R[{i},{j}] = {R[i, j]} should be ~0"
    
    # =============================================================================
    # Geometric Operations Tests
    # =============================================================================
    
    def test_euler_rotation_roundtrip(self):
        """Test Euler angle to rotation matrix conversion and back."""
        roll, pitch, yaw = 0.1, 0.2, 0.3
        
        rotation = eb.EigenMath.euler_to_rotation_matrix(roll, pitch, yaw)
        recovered_euler = eb.EigenMath.rotation_matrix_to_euler(rotation)
        
        assert abs(recovered_euler[0] - roll) < 1e-10
        assert abs(recovered_euler[1] - pitch) < 1e-10
        assert abs(recovered_euler[2] - yaw) < 1e-10
        
        # Check rotation matrix properties
        assert abs(np.linalg.det(rotation) - 1.0) < self.tolerance  # det = 1
        should_be_identity = np.dot(rotation, rotation.T)
        identity = np.eye(3)
        self.assert_arrays_close(should_be_identity, identity, rtol=1e-12, atol=1e-12)
    
    def test_create_transformation_matrix(self):
        """Test transformation matrix creation."""
        translation = np.array([1, 2, 3], dtype=np.float64)
        rotation = eb.EigenMath.euler_to_rotation_matrix(0, 0, np.pi/2)  # 90° yaw
        
        transform = eb.EigenMath.create_transformation_matrix(translation, rotation)
        
        # Check structure
        assert transform.shape == (4, 4)
        self.assert_arrays_close(transform[:3, :3], rotation)
        self.assert_arrays_close(transform[:3, 3], translation)
        self.assert_arrays_close(transform[3, :], np.array([0, 0, 0, 1]))
    
    # =============================================================================
    # Utility Functions Tests
    # =============================================================================
    
    def test_matrices_equal(self):
        """Test matrix equality checking."""
        a = eb.create_random_matrix(3, 3, seed=42)
        b = np.copy(a)
        c = a + np.ones((3, 3)) * 1e-5
        
        assert eb.EigenMath.matrices_equal(a, b, self.tolerance)
        assert not eb.EigenMath.matrices_equal(a, c, self.tolerance)
        assert eb.EigenMath.matrices_equal(a, c, 1e-4)
        
        # Test different dimensions
        d = np.random.rand(2, 2)
        assert not eb.EigenMath.matrices_equal(a, d, self.tolerance)
    
    def test_matrix_info(self):
        """Test matrix info string generation."""
        matrix = eb.EigenMath.create_identity_matrix(3)
        info = eb.EigenMath.matrix_info(matrix)
        
        assert isinstance(info, str)
        assert len(info) > 0
        assert "3 x 3" in info
        assert "Determinant" in info
    
    def test_benchmark_matrix_multiplication(self):
        """Test matrix multiplication benchmarking."""
        time_ms = eb.EigenMath.benchmark_matrix_multiplication(10, 5)
        
        assert isinstance(time_ms, float)
        assert time_ms > 0.0
        assert time_ms < 1000.0  # Should not take more than 1 second for small matrices
        
        # Error handling
        with pytest.raises(ValueError):
            eb.EigenMath.benchmark_matrix_multiplication(0, 10)
        with pytest.raises(ValueError):
            eb.EigenMath.benchmark_matrix_multiplication(10, 0)
    
    # =============================================================================
    # Integration and Performance Tests
    # =============================================================================
    
    def test_numpy_integration(self):
        """Test seamless integration with NumPy arrays."""
        # Test various NumPy dtypes and layouts
        a_float32 = np.random.rand(3, 3).astype(np.float32)
        a_float64 = np.random.rand(3, 3).astype(np.float64)
        
        # Both should work (with appropriate conversion)
        result32 = eb.EigenMath.determinant(a_float32)
        result64 = eb.EigenMath.determinant(a_float64)
        
        assert isinstance(result32, float)
        assert isinstance(result64, float)
        
        # Test row-major vs column-major arrays
        a_c_order = np.asarray(np.random.rand(3, 3), order='C')  # Row-major
        a_f_order = np.asarray(np.random.rand(3, 3), order='F')  # Column-major
        
        det_c = eb.EigenMath.determinant(a_c_order)
        det_f = eb.EigenMath.determinant(a_f_order)
        
        assert isinstance(det_c, float)
        assert isinstance(det_f, float)
    
    def test_large_matrices(self):
        """Test with reasonably large matrices to verify performance."""
        large_matrix = eb.create_random_matrix(100, 100, seed=42)
        assert large_matrix.shape == (100, 100)
        
        # These operations should complete in reasonable time
        det = eb.EigenMath.determinant(large_matrix)
        assert isinstance(det, float)
        
        # Test matrix multiplication
        result = eb.multiply_matrices(large_matrix, large_matrix)
        assert result.shape == (100, 100)
    
    def test_module_attributes(self):
        """Test module-level attributes and constants."""
        assert hasattr(eb, '__version__')
        assert hasattr(eb, '__author__')
        assert hasattr(eb, '__description__')
        assert hasattr(eb, 'PI')
        assert hasattr(eb, 'E')
        
        assert abs(eb.PI - np.pi) < 1e-15
        assert abs(eb.E - np.e) < 1e-15
    
    def test_exception_translation(self):
        """Test that C++ exceptions are properly translated to Python exceptions."""
        # These should raise ValueError (translated from std::invalid_argument)
        with pytest.raises(ValueError):
            eb.EigenMath.determinant(np.random.rand(2, 3))  # Non-square
        
        with pytest.raises(ValueError):
            eb.create_random_matrix(-1, 3)  # Negative dimension
        
        # These should raise RuntimeError (translated from std::runtime_error)  
        # Note: Might need to adjust based on actual implementation
        try:
            singular = np.array([[1, 2], [2, 4]], dtype=np.float64)
            eb.invert_matrix(singular)
            assert False, "Should have raised an exception"
        except (ValueError, RuntimeError):
            pass  # Either is acceptable for singular matrix


if __name__ == "__main__":
    # Allow running tests directly
    pytest.main([__file__, "-v"])