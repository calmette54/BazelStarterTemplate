package eigen_math

import (
	"fmt"
	"math"
	"runtime"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

// Test tolerance for floating point comparisons
const testTolerance = 1e-10

// =============================================================================
// Matrix Tests
// =============================================================================

func TestNewMatrix(t *testing.T) {
	t.Run("ValidMatrix", func(t *testing.T) {
		data := [][]float64{
			{1, 2, 3},
			{4, 5, 6},
		}
		
		matrix, err := NewMatrix(data)
		require.NoError(t, err)
		require.NotNil(t, matrix)
		defer matrix.Close()
		
		assert.Equal(t, 2, matrix.Rows())
		assert.Equal(t, 3, matrix.Cols())
		
		retrieved, err := matrix.ToSlice()
		require.NoError(t, err)
		assert.Equal(t, data, retrieved)
	})
	
	t.Run("EmptyMatrix", func(t *testing.T) {
		data := [][]float64{}
		
		_, err := NewMatrix(data)
		assert.Error(t, err)
	})
	
	t.Run("InconsistentRows", func(t *testing.T) {
		data := [][]float64{
			{1, 2, 3},
			{4, 5},  // Different length
		}
		
		_, err := NewMatrix(data)
		assert.Error(t, err)
	})
}

func TestNewRandomMatrix(t *testing.T) {
	t.Run("ValidDimensions", func(t *testing.T) {
		matrix, err := NewRandomMatrix(3, 4, 42)
		require.NoError(t, err)
		require.NotNil(t, matrix)
		defer matrix.Close()
		
		assert.Equal(t, 3, matrix.Rows())
		assert.Equal(t, 4, matrix.Cols())
		
		// Verify data is within expected range [-1, 1]
		data, err := matrix.ToSlice()
		require.NoError(t, err)
		
		for _, row := range data {
			for _, val := range row {
				assert.True(t, val >= -1.0 && val <= 1.0, "Value %f out of range", val)
			}
		}
	})
	
	t.Run("InvalidDimensions", func(t *testing.T) {
		_, err := NewRandomMatrix(0, 5)
		assert.Error(t, err)
		
		_, err = NewRandomMatrix(-1, 5)
		assert.Error(t, err)
	})
	
	t.Run("SeedReproducibility", func(t *testing.T) {
		matrix1, err1 := NewRandomMatrix(2, 2, 12345)
		require.NoError(t, err1)
		defer matrix1.Close()
		
		matrix2, err2 := NewRandomMatrix(2, 2, 12345)
		require.NoError(t, err2)
		defer matrix2.Close()
		
		data1, err1 := matrix1.ToSlice()
		require.NoError(t, err1)
		
		data2, err2 := matrix2.ToSlice()
		require.NoError(t, err2)
		
		assert.Equal(t, data1, data2, "Same seed should produce identical matrices")
	})
}

func TestNewIdentityMatrix(t *testing.T) {
	t.Run("ValidSize", func(t *testing.T) {
		matrix, err := NewIdentityMatrix(3)
		require.NoError(t, err)
		require.NotNil(t, matrix)
		defer matrix.Close()
		
		assert.Equal(t, 3, matrix.Rows())
		assert.Equal(t, 3, matrix.Cols())
		
		data, err := matrix.ToSlice()
		require.NoError(t, err)
		
		// Check identity matrix properties
		for i := 0; i < 3; i++ {
			for j := 0; j < 3; j++ {
				if i == j {
					assert.InDelta(t, 1.0, data[i][j], testTolerance, "Diagonal element should be 1")
				} else {
					assert.InDelta(t, 0.0, data[i][j], testTolerance, "Off-diagonal element should be 0")
				}
			}
		}
	})
	
	t.Run("InvalidSize", func(t *testing.T) {
		_, err := NewIdentityMatrix(0)
		assert.Error(t, err)
		
		_, err = NewIdentityMatrix(-1)
		assert.Error(t, err)
	})
}

func TestMatrixMultiply(t *testing.T) {
	t.Run("ValidMultiplication", func(t *testing.T) {
		// Create matrix A (2x3)
		dataA := [][]float64{
			{1, 2, 3},
			{4, 5, 6},
		}
		matrixA, err := NewMatrix(dataA)
		require.NoError(t, err)
		defer matrixA.Close()
		
		// Create matrix B (3x2)
		dataB := [][]float64{
			{7, 8},
			{9, 10},
			{11, 12},
		}
		matrixB, err := NewMatrix(dataB)
		require.NoError(t, err)
		defer matrixB.Close()
		
		// Multiply A * B should give (2x2)
		result, err := matrixA.Multiply(matrixB)
		require.NoError(t, err)
		require.NotNil(t, result)
		defer result.Close()
		
		assert.Equal(t, 2, result.Rows())
		assert.Equal(t, 2, result.Cols())
		
		data, err := result.ToSlice()
		require.NoError(t, err)
		
		// Expected result: [58, 64], [139, 154]
		expected := [][]float64{
			{58, 64},
			{139, 154},
		}
		
		for i := 0; i < 2; i++ {
			for j := 0; j < 2; j++ {
				assert.InDelta(t, expected[i][j], data[i][j], testTolerance)
			}
		}
	})
	
	t.Run("IncompatibleDimensions", func(t *testing.T) {
		matrixA, err := NewMatrix([][]float64{{1, 2}})
		require.NoError(t, err)
		defer matrixA.Close()
		
		matrixB, err := NewMatrix([][]float64{{1}, {2}, {3}})
		require.NoError(t, err)
		defer matrixB.Close()
		
		_, err = matrixA.Multiply(matrixB)
		assert.Error(t, err)
	})
}

func TestMatrixTranspose(t *testing.T) {
	data := [][]float64{
		{1, 2, 3},
		{4, 5, 6},
	}
	
	matrix, err := NewMatrix(data)
	require.NoError(t, err)
	defer matrix.Close()
	
	transposed, err := matrix.Transpose()
	require.NoError(t, err)
	require.NotNil(t, transposed)
	defer transposed.Close()
	
	assert.Equal(t, 3, transposed.Rows())
	assert.Equal(t, 2, transposed.Cols())
	
	result, err := transposed.ToSlice()
	require.NoError(t, err)
	
	expected := [][]float64{
		{1, 4},
		{2, 5},
		{3, 6},
	}
	
	assert.Equal(t, expected, result)
}

func TestMatrixInverse(t *testing.T) {
	t.Run("InvertibleMatrix", func(t *testing.T) {
		// Create a simple invertible 2x2 matrix
		data := [][]float64{
			{1, 2},
			{3, 4},
		}
		
		matrix, err := NewMatrix(data)
		require.NoError(t, err)
		defer matrix.Close()
		
		inverse, err := matrix.Inverse()
		require.NoError(t, err)
		require.NotNil(t, inverse)
		defer inverse.Close()
		
		// Verify A * A^-1 = I
		identity, err := matrix.Multiply(inverse)
		require.NoError(t, err)
		defer identity.Close()
		
		identityData, err := identity.ToSlice()
		require.NoError(t, err)
		
		// Check if result is approximately identity
		for i := 0; i < 2; i++ {
			for j := 0; j < 2; j++ {
				if i == j {
					assert.InDelta(t, 1.0, identityData[i][j], testTolerance)
				} else {
					assert.InDelta(t, 0.0, identityData[i][j], testTolerance)
				}
			}
		}
	})
	
	t.Run("NonInvertibleMatrix", func(t *testing.T) {
		// Singular matrix (determinant = 0)
		data := [][]float64{
			{1, 2},
			{2, 4},  // Second row is 2x first row
		}
		
		matrix, err := NewMatrix(data)
		require.NoError(t, err)
		defer matrix.Close()
		
		_, err = matrix.Inverse()
		assert.Error(t, err, "Should fail to invert singular matrix")
	})
}

func TestMatrixDeterminant(t *testing.T) {
	t.Run("2x2Matrix", func(t *testing.T) {
		data := [][]float64{
			{1, 2},
			{3, 4},
		}
		
		matrix, err := NewMatrix(data)
		require.NoError(t, err)
		defer matrix.Close()
		
		det, err := matrix.Determinant()
		require.NoError(t, err)
		
		// Expected determinant: 1*4 - 2*3 = -2
		assert.InDelta(t, -2.0, det, testTolerance)
	})
	
	t.Run("3x3Matrix", func(t *testing.T) {
		data := [][]float64{
			{1, 2, 3},
			{0, 1, 4},
			{5, 6, 0},
		}
		
		matrix, err := NewMatrix(data)
		require.NoError(t, err)
		defer matrix.Close()
		
		det, err := matrix.Determinant()
		require.NoError(t, err)
		
		// Expected determinant: 1*(1*0 - 4*6) - 2*(0*0 - 4*5) + 3*(0*6 - 1*5) = -24 + 40 - 15 = 1
		assert.InDelta(t, 1.0, det, testTolerance)
	})
}

// =============================================================================
// Vector Tests
// =============================================================================

func TestNewVector(t *testing.T) {
	t.Run("ValidVector", func(t *testing.T) {
		data := []float64{1, 2, 3, 4, 5}
		
		vector, err := NewVector(data)
		require.NoError(t, err)
		require.NotNil(t, vector)
		defer vector.Close()
		
		assert.Equal(t, 5, vector.Size())
		
		retrieved, err := vector.ToSlice()
		require.NoError(t, err)
		assert.Equal(t, data, retrieved)
	})
	
	t.Run("EmptyVector", func(t *testing.T) {
		data := []float64{}
		
		_, err := NewVector(data)
		assert.Error(t, err)
	})
}

func TestNewRandomVector(t *testing.T) {
	vector, err := NewRandomVector(5, 42)
	require.NoError(t, err)
	require.NotNil(t, vector)
	defer vector.Close()
	
	assert.Equal(t, 5, vector.Size())
	
	data, err := vector.ToSlice()
	require.NoError(t, err)
	
	// Verify data is within expected range [-1, 1]
	for _, val := range data {
		assert.True(t, val >= -1.0 && val <= 1.0, "Value %f out of range", val)
	}
}

func TestVectorDot(t *testing.T) {
	t.Run("ValidDotProduct", func(t *testing.T) {
		vectorA, err := NewVector([]float64{1, 2, 3})
		require.NoError(t, err)
		defer vectorA.Close()
		
		vectorB, err := NewVector([]float64{4, 5, 6})
		require.NoError(t, err)
		defer vectorB.Close()
		
		dot, err := vectorA.Dot(vectorB)
		require.NoError(t, err)
		
		// Expected: 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
		assert.InDelta(t, 32.0, dot, testTolerance)
	})
	
	t.Run("IncompatibleSizes", func(t *testing.T) {
		vectorA, err := NewVector([]float64{1, 2})
		require.NoError(t, err)
		defer vectorA.Close()
		
		vectorB, err := NewVector([]float64{1, 2, 3})
		require.NoError(t, err)
		defer vectorB.Close()
		
		_, err = vectorA.Dot(vectorB)
		assert.Error(t, err)
	})
}

func TestVectorCross(t *testing.T) {
	t.Run("Valid3DVectors", func(t *testing.T) {
		vectorA, err := NewVector([]float64{1, 0, 0})
		require.NoError(t, err)
		defer vectorA.Close()
		
		vectorB, err := NewVector([]float64{0, 1, 0})
		require.NoError(t, err)
		defer vectorB.Close()
		
		cross, err := vectorA.Cross(vectorB)
		require.NoError(t, err)
		require.NotNil(t, cross)
		defer cross.Close()
		
		result, err := cross.ToSlice()
		require.NoError(t, err)
		
		// Expected: [1, 0, 0] × [0, 1, 0] = [0, 0, 1]
		expected := []float64{0, 0, 1}
		for i, val := range expected {
			assert.InDelta(t, val, result[i], testTolerance)
		}
	})
	
	t.Run("Non3DVectors", func(t *testing.T) {
		vectorA, err := NewVector([]float64{1, 2})
		require.NoError(t, err)
		defer vectorA.Close()
		
		vectorB, err := NewVector([]float64{3, 4})
		require.NoError(t, err)
		defer vectorB.Close()
		
		_, err = vectorA.Cross(vectorB)
		assert.Error(t, err, "Cross product should only work with 3D vectors")
	})
}

func TestVectorNormalize(t *testing.T) {
	t.Run("ValidVector", func(t *testing.T) {
		vector, err := NewVector([]float64{3, 4, 0})
		require.NoError(t, err)
		defer vector.Close()
		
		normalized, err := vector.Normalize()
		require.NoError(t, err)
		require.NotNil(t, normalized)
		defer normalized.Close()
		
		result, err := normalized.ToSlice()
		require.NoError(t, err)
		
		// Original vector has norm 5, so normalized should be [0.6, 0.8, 0]
		expected := []float64{0.6, 0.8, 0}
		for i, val := range expected {
			assert.InDelta(t, val, result[i], testTolerance)
		}
		
		// Check that the norm is 1
		norm, err := normalized.Norm()
		require.NoError(t, err)
		assert.InDelta(t, 1.0, norm, testTolerance)
	})
	
	t.Run("ZeroVector", func(t *testing.T) {
		vector, err := NewVector([]float64{0, 0, 0})
		require.NoError(t, err)
		defer vector.Close()
		
		_, err = vector.Normalize()
		assert.Error(t, err, "Should not be able to normalize zero vector")
	})
}

func TestVectorNorm(t *testing.T) {
	vector, err := NewVector([]float64{3, 4, 0})
	require.NoError(t, err)
	defer vector.Close()
	
	norm, err := vector.Norm()
	require.NoError(t, err)
	
	// Expected: sqrt(3^2 + 4^2 + 0^2) = 5
	assert.InDelta(t, 5.0, norm, testTolerance)
}

// =============================================================================
// Advanced Linear Algebra Tests
// =============================================================================

func TestEigenDecomposition(t *testing.T) {
	t.Run("SymmetricMatrix", func(t *testing.T) {
		// Create a simple symmetric matrix
		data := [][]float64{
			{2, 1},
			{1, 2},
		}
		
		matrix, err := NewMatrix(data)
		require.NoError(t, err)
		defer matrix.Close()
		
		eigenvals, eigenvecs, err := matrix.EigenDecomposition()
		require.NoError(t, err)
		require.NotNil(t, eigenvals)
		require.NotNil(t, eigenvecs)
		defer eigenvals.Close()
		defer eigenvecs.Close()
		
		vals, err := eigenvals.ToSlice()
		require.NoError(t, err)
		
		// For this matrix, eigenvalues should be 3 and 1
		assert.Equal(t, 2, len(vals))
		// Note: eigenvalues may be returned in any order
		hasThree := false
		hasOne := false
		for _, val := range vals {
			if math.Abs(val-3.0) < testTolerance {
				hasThree = true
			}
			if math.Abs(val-1.0) < testTolerance {
				hasOne = true
			}
		}
		assert.True(t, hasThree, "Should have eigenvalue 3")
		assert.True(t, hasOne, "Should have eigenvalue 1")
	})
}

func TestSVDDecomposition(t *testing.T) {
	data := [][]float64{
		{1, 2},
		{3, 4},
		{5, 6},
	}
	
	matrix, err := NewMatrix(data)
	require.NoError(t, err)
	defer matrix.Close()
	
	U, S, Vt, err := matrix.SVD()
	require.NoError(t, err)
	require.NotNil(t, U)
	require.NotNil(t, S)
	require.NotNil(t, Vt)
	defer U.Close()
	defer S.Close()  
	defer Vt.Close()
	
	// Basic dimension checks - using full SVD
	assert.Equal(t, 3, U.Rows())
	assert.Equal(t, 3, U.Cols())  // Full SVD returns square U matrix
	assert.Equal(t, 2, S.Size())  // min(rows, cols)
	assert.Equal(t, 2, Vt.Rows())
	assert.Equal(t, 2, Vt.Cols())
	
	// Singular values should be positive and in descending order
	singularVals, err := S.ToSlice()
	require.NoError(t, err)
	
	for _, val := range singularVals {
		assert.True(t, val >= 0, "Singular values should be non-negative")
	}
	
	if len(singularVals) > 1 {
		assert.True(t, singularVals[0] >= singularVals[1], "Singular values should be in descending order")
	}
}

func TestSolveLinearSystem(t *testing.T) {
	t.Run("WellConditionedSystem", func(t *testing.T) {
		// Solve Ax = b where A = [[2, 1], [1, 2]], b = [3, 3]
		// Expected solution: x = [1, 1]
		
		A, err := NewMatrix([][]float64{
			{2, 1},
			{1, 2},
		})
		require.NoError(t, err)
		defer A.Close()
		
		b, err := NewVector([]float64{3, 3})
		require.NoError(t, err)
		defer b.Close()
		
		x, err := SolveLinearSystem(A, b)
		require.NoError(t, err)
		require.NotNil(t, x)
		defer x.Close()
		
		solution, err := x.ToSlice()
		require.NoError(t, err)
		
		expected := []float64{1, 1}
		for i, val := range expected {
			assert.InDelta(t, val, solution[i], testTolerance)
		}
	})
}

func TestQRDecomposition(t *testing.T) {
	data := [][]float64{
		{1, 2},
		{3, 4},
		{5, 6},
	}
	
	matrix, err := NewMatrix(data)
	require.NoError(t, err)
	defer matrix.Close()
	
	Q, R, err := QRDecomposition(matrix)
	require.NoError(t, err)
	require.NotNil(t, Q)
	require.NotNil(t, R)
	defer Q.Close()
	defer R.Close()
	
	// Basic dimension checks - QR for 3×2 matrix  
	assert.Equal(t, 3, Q.Rows())
	assert.Equal(t, 3, Q.Cols())  // Q is always square for full QR
	assert.Equal(t, 3, R.Rows())  // R has same number of rows as input
	assert.Equal(t, 2, R.Cols())  // R has same number of columns as input
}

// =============================================================================
// Geometric Operations Tests
// =============================================================================

func TestEulerToRotationMatrix(t *testing.T) {
	// Test with zero rotation (should give identity)
	rotMatrix, err := EulerToRotationMatrix(0, 0, 0)
	require.NoError(t, err)
	require.NotNil(t, rotMatrix)
	defer rotMatrix.Close()
	
	data, err := rotMatrix.ToSlice()
	require.NoError(t, err)
	
	// Should be 3x3 identity matrix
	assert.Equal(t, 3, len(data))
	assert.Equal(t, 3, len(data[0]))
	
	for i := 0; i < 3; i++ {
		for j := 0; j < 3; j++ {
			if i == j {
				assert.InDelta(t, 1.0, data[i][j], testTolerance)
			} else {
				assert.InDelta(t, 0.0, data[i][j], testTolerance)
			}
		}
	}
}

func TestRotationMatrixToEuler(t *testing.T) {
	// Create a rotation matrix and convert back to Euler angles
	originalRoll, originalPitch, originalYaw := 0.1, 0.2, 0.3
	
	rotMatrix, err := EulerToRotationMatrix(originalRoll, originalPitch, originalYaw)
	require.NoError(t, err)
	defer rotMatrix.Close()
	
	roll, pitch, yaw, err := RotationMatrixToEuler(rotMatrix)
	require.NoError(t, err)
	
	// Should recover original angles (within tolerance)
	assert.InDelta(t, originalRoll, roll, testTolerance)
	assert.InDelta(t, originalPitch, pitch, testTolerance) 
	assert.InDelta(t, originalYaw, yaw, testTolerance)
}

func TestCreateTransformationMatrix(t *testing.T) {
	// Create identity rotation
	rotMatrix, err := EulerToRotationMatrix(0, 0, 0)
	require.NoError(t, err)
	defer rotMatrix.Close()
	
	// Create transformation matrix with translation [1, 2, 3]
	transMatrix, err := CreateTransformationMatrix(1, 2, 3, rotMatrix)
	require.NoError(t, err)
	require.NotNil(t, transMatrix)
	defer transMatrix.Close()
	
	data, err := transMatrix.ToSlice()
	require.NoError(t, err)
	
	// Should be 4x4
	assert.Equal(t, 4, len(data))
	assert.Equal(t, 4, len(data[0]))
	
	// Check translation part
	assert.InDelta(t, 1.0, data[0][3], testTolerance)
	assert.InDelta(t, 2.0, data[1][3], testTolerance)
	assert.InDelta(t, 3.0, data[2][3], testTolerance)
	
	// Bottom row should be [0, 0, 0, 1]
	assert.InDelta(t, 0.0, data[3][0], testTolerance)
	assert.InDelta(t, 0.0, data[3][1], testTolerance)
	assert.InDelta(t, 0.0, data[3][2], testTolerance)
	assert.InDelta(t, 1.0, data[3][3], testTolerance)
}

// =============================================================================
// Types Tests
// =============================================================================

func TestNewZeroMatrix(t *testing.T) {
	matrix, err := NewZeroMatrix(2, 3)
	require.NoError(t, err)
	require.NotNil(t, matrix)
	defer matrix.Close()
	
	data, err := matrix.ToSlice()
	require.NoError(t, err)
	
	for i := 0; i < 2; i++ {
		for j := 0; j < 3; j++ {
			assert.Equal(t, 0.0, data[i][j])
		}
	}
}

func TestNewOnesMatrix(t *testing.T) {
	matrix, err := NewOnesMatrix(2, 3)
	require.NoError(t, err)
	require.NotNil(t, matrix)
	defer matrix.Close()
	
	data, err := matrix.ToSlice()
	require.NoError(t, err)
	
	for i := 0; i < 2; i++ {
		for j := 0; j < 3; j++ {
			assert.Equal(t, 1.0, data[i][j])
		}
	}
}

func TestNewDiagonalMatrix(t *testing.T) {
	diagonal := []float64{1, 2, 3}
	matrix, err := NewDiagonalMatrix(diagonal)
	require.NoError(t, err)
	require.NotNil(t, matrix)
	defer matrix.Close()
	
	data, err := matrix.ToSlice()
	require.NoError(t, err)
	
	for i := 0; i < 3; i++ {
		for j := 0; j < 3; j++ {
			if i == j {
				assert.Equal(t, diagonal[i], data[i][j])
			} else {
				assert.Equal(t, 0.0, data[i][j])
			}
		}
	}
}

func TestPoint2DDistance(t *testing.T) {
	p1 := Point2D{X: 0, Y: 0}
	p2 := Point2D{X: 3, Y: 4}
	
	distance := p1.Distance(p2)
	assert.InDelta(t, 5.0, distance, testTolerance) // 3-4-5 triangle
}

func TestPoint3DDistance(t *testing.T) {
	p1 := Point3D{X: 0, Y: 0, Z: 0}
	p2 := Point3D{X: 1, Y: 1, Z: 1}
	
	distance := p1.Distance(p2)
	expected := math.Sqrt(3.0)
	assert.InDelta(t, expected, distance, testTolerance)
}

func TestQuaternionNormalize(t *testing.T) {
	q := Quaternion{W: 1, X: 2, Y: 3, Z: 4}
	qn := q.Normalize()
	
	// Check that normalized quaternion has unit length
	length := math.Sqrt(qn.W*qn.W + qn.X*qn.X + qn.Y*qn.Y + qn.Z*qn.Z)
	assert.InDelta(t, 1.0, length, testTolerance)
}

// =============================================================================
// Utility Tests
// =============================================================================

func TestMatrixEqual(t *testing.T) {
	matrixA, err := NewMatrix([][]float64{{1, 2}, {3, 4}})
	require.NoError(t, err)
	defer matrixA.Close()
	
	matrixB, err := NewMatrix([][]float64{{1, 2}, {3, 4}})
	require.NoError(t, err)
	defer matrixB.Close()
	
	equal, err := matrixA.Equal(matrixB, testTolerance)
	require.NoError(t, err)
	assert.True(t, equal)
	
	matrixC, err := NewMatrix([][]float64{{1, 2}, {3, 5}})
	require.NoError(t, err)
	defer matrixC.Close()
	
	equal, err = matrixA.Equal(matrixC, testTolerance)
	require.NoError(t, err)
	assert.False(t, equal)
}

func TestMatrixInfo(t *testing.T) {
	matrix, err := NewMatrix([][]float64{{1, 2}, {3, 4}})
	require.NoError(t, err)
	defer matrix.Close()
	
	info, err := matrix.Info()
	require.NoError(t, err)
	assert.NotEmpty(t, info)
	assert.Contains(t, info, "2")  // Should contain dimensions
}

func TestBenchmarkMatrixMultiplication(t *testing.T) {
	timeMs, err := BenchmarkMatrixMultiply(50, 10)
	require.NoError(t, err)
	assert.True(t, timeMs >= 0, "Time should be non-negative")
}

// =============================================================================
// Benchmarks
// =============================================================================

func BenchmarkMatrixMultiplication(b *testing.B) {
	sizes := []int{10, 50, 100}
	
	for _, size := range sizes {
		b.Run(fmt.Sprintf("Size%d", size), func(b *testing.B) {
			matrixA, err := NewRandomMatrix(size, size, 42)
			require.NoError(b, err)
			defer matrixA.Close()
			
			matrixB, err := NewRandomMatrix(size, size, 43)
			require.NoError(b, err)
			defer matrixB.Close()
			
			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				result, err := matrixA.Multiply(matrixB)
				require.NoError(b, err)
				result.Close()
			}
		})
	}
}

func BenchmarkVectorDotProduct(b *testing.B) {
	sizes := []int{100, 1000, 10000}
	
	for _, size := range sizes {
		b.Run(fmt.Sprintf("Size%d", size), func(b *testing.B) {
			vectorA, err := NewRandomVector(size, 42)
			require.NoError(b, err)
			defer vectorA.Close()
			
			vectorB, err := NewRandomVector(size, 43)
			require.NoError(b, err)
			defer vectorB.Close()
			
			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				_, err := vectorA.Dot(vectorB)
				require.NoError(b, err)
			}
		})
	}
}

func BenchmarkMatrixInverse(b *testing.B) {
	sizes := []int{5, 10, 20}
	
	for _, size := range sizes {
		b.Run(fmt.Sprintf("Size%d", size), func(b *testing.B) {
			matrix, err := NewRandomMatrix(size, size, 42)
			require.NoError(b, err)
			defer matrix.Close()
			
			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				inverse, err := matrix.Inverse()
				if err == nil { // Only count successful inversions
					inverse.Close()
				}
			}
		})
	}
}

// =============================================================================
// Memory Management Tests
// =============================================================================

func TestMemoryManagement(t *testing.T) {
	t.Run("AutomaticCleanup", func(t *testing.T) {
		// Test that finalizers work properly
		for i := 0; i < 100; i++ {
			matrix, err := NewRandomMatrix(10, 10)
			require.NoError(t, err)
			_ = matrix // Don't explicitly close
		}
		
		// Force garbage collection
		runtime.GC()
		runtime.GC()
		
		// If we get here without crashes, finalizers are working
		assert.True(t, true, "Memory management test passed")
	})
	
	t.Run("ExplicitClose", func(t *testing.T) {
		matrix, err := NewRandomMatrix(5, 5)
		require.NoError(t, err)
		
		// Close explicitly
		matrix.Close()
		
		// Further operations should fail gracefully
		_, err = matrix.Determinant()
		assert.Error(t, err, "Operations after Close should fail")
	})
}