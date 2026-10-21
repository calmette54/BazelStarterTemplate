// Comprehensive example demonstrating Go bindings for Eigen mathematical operations
package main

import (
	"fmt"
	"log"
	"math"
	"runtime"
	"strings"
	"time"

	eigen_math "example.com/eigen_go"
)

func main() {
	fmt.Println("=================================================================")
	fmt.Println("Go Bindings for Eigen Mathematical Operations - Comprehensive Demo")
	fmt.Println("=================================================================\n")

	if err := runDemo(); err != nil {
		log.Fatalf("Demo failed: %v", err)
	}

	fmt.Println("\n=================================================================")
	fmt.Println("Demo completed successfully!")
	fmt.Println("=================================================================")
}

func runDemo() error {
	// Basic Matrix Operations
	if err := demonstrateBasicMatrixOperations(); err != nil {
		return fmt.Errorf("basic matrix operations failed: %w", err)
	}

	// Vector Operations  
	if err := demonstrateVectorOperations(); err != nil {
		return fmt.Errorf("vector operations failed: %w", err)
	}

	// Advanced Linear Algebra
	if err := demonstrateAdvancedLinearAlgebra(); err != nil {
		return fmt.Errorf("advanced linear algebra failed: %w", err)
	}

	// Geometric Operations
	if err := demonstrateGeometricOperations(); err != nil {
		return fmt.Errorf("geometric operations failed: %w", err)
	}

	// Type System and Utilities
	if err := demonstrateTypeSystem(); err != nil {
		return fmt.Errorf("type system demonstration failed: %w", err)
	}

	// Performance Benchmarks
	if err := demonstratePerformance(); err != nil {
		return fmt.Errorf("performance demonstration failed: %w", err)
	}

	return nil
}

func demonstrateBasicMatrixOperations() error {
	fmt.Println("1. Basic Matrix Operations")
	fmt.Println("=========================\n")

	// Create matrices
	fmt.Println("Creating matrices...")
	dataA := [][]float64{
		{1, 2, 3},
		{4, 5, 6},
	}
	matrixA, err := eigen_math.NewMatrix(dataA)
	if err != nil {
		return err
	}
	defer matrixA.Close()

	dataB := [][]float64{
		{7, 8},
		{9, 10},
		{11, 12},
	}
	matrixB, err := eigen_math.NewMatrix(dataB)
	if err != nil {
		return err
	}
	defer matrixB.Close()

	fmt.Printf("Matrix A (%dx%d):\n%v\n", matrixA.Rows(), matrixA.Cols(), matrixA)
	fmt.Printf("Matrix B (%dx%d):\n%v\n", matrixB.Rows(), matrixB.Cols(), matrixB)

	// Matrix multiplication
	fmt.Println("Computing A × B...")
	product, err := matrixA.Multiply(matrixB)
	if err != nil {
		return err
	}
	defer product.Close()

	fmt.Printf("A × B (%dx%d):\n%v\n", product.Rows(), product.Cols(), product)

	// Create a square matrix for more operations
	squareData := [][]float64{
		{2, 1, 3},
		{1, 2, 1},
		{3, 1, 4},
	}
	square, err := eigen_math.NewMatrix(squareData)
	if err != nil {
		return err
	}
	defer square.Close()

	fmt.Printf("Square Matrix:\n%v\n", square)

	// Transpose
	transposed, err := square.Transpose()
	if err != nil {
		return err
	}
	defer transposed.Close()

	fmt.Printf("Transposed:\n%v\n", transposed)

	// Determinant
	det, err := square.Determinant()
	if err != nil {
		return err
	}
	fmt.Printf("Determinant: %.6f\n", det)

	// Inverse (if possible)
	if math.Abs(det) > 1e-10 {
		inverse, err := square.Inverse()
		if err != nil {
			fmt.Printf("Could not compute inverse: %v\n", err)
		} else {
			defer inverse.Close()
			fmt.Printf("Inverse:\n%v\n", inverse)

			// Verify A × A^-1 ≈ I
			identity, err := square.Multiply(inverse)
			if err != nil {
				return err
			}
			defer identity.Close()
			fmt.Printf("Square × Inverse (should be identity):\n%v\n", identity)
		}
	}

	// Create identity and random matrices
	fmt.Println("\nTesting utility constructors...")
	identity, err := eigen_math.NewIdentityMatrix(3)
	if err != nil {
		return err
	}
	defer identity.Close()
	fmt.Printf("3×3 Identity Matrix:\n%v\n", identity)

	random, err := eigen_math.NewRandomMatrix(2, 3, 12345)
	if err != nil {
		return err
	}
	defer random.Close()
	fmt.Printf("2×3 Random Matrix (seed=12345):\n%v\n", random)

	fmt.Println("✓ Basic matrix operations completed successfully\n")
	return nil
}

func demonstrateVectorOperations() error {
	fmt.Println("2. Vector Operations")
	fmt.Println("===================\n")

	// Create vectors
	fmt.Println("Creating vectors...")
	vecA, err := eigen_math.NewVector([]float64{1, 2, 3, 4})
	if err != nil {
		return err
	}
	defer vecA.Close()

	vecB, err := eigen_math.NewVector([]float64{5, 6, 7, 8})
	if err != nil {
		return err
	}
	defer vecB.Close()

	fmt.Printf("Vector A: %v\n", vecA)
	fmt.Printf("Vector B: %v\n", vecB)

	// Dot product
	dot, err := vecA.Dot(vecB)
	if err != nil {
		return err
	}
	fmt.Printf("A · B = %.6f\n", dot)

	// Vector norms
	normA, err := vecA.Norm()
	if err != nil {
		return err
	}
	normB, err := vecB.Norm()
	if err != nil {
		return err
	}

	fmt.Printf("||A|| = %.6f\n", normA)
	fmt.Printf("||B|| = %.6f\n", normB)

	// Normalize vectors
	normalizedA, err := vecA.Normalize()
	if err != nil {
		return err
	}
	defer normalizedA.Close()

	fmt.Printf("Normalized A: %v\n", normalizedA)

	normNormalizedA, err := normalizedA.Norm()
	if err != nil {
		return err
	}
	fmt.Printf("||Normalized A|| = %.6f (should be 1.0)\n", normNormalizedA)

	// 3D vector operations
	fmt.Println("\n3D Vector Operations:")
	vec3DA, err := eigen_math.NewVector3D(1, 0, 0)
	if err != nil {
		return err
	}
	defer vec3DA.Close()

	vec3DB, err := eigen_math.NewVector3D(0, 1, 0)
	if err != nil {
		return err
	}
	defer vec3DB.Close()

	fmt.Printf("3D Vector A: %v\n", vec3DA)
	fmt.Printf("3D Vector B: %v\n", vec3DB)

	// Cross product
	cross, err := vec3DA.Cross(vec3DB)
	if err != nil {
		return err
	}
	defer cross.Close()

	fmt.Printf("A × B: %v\n", cross)

	// Convert to points
	pointA, err := vec3DA.ToPoint3D()
	if err != nil {
		return err
	}
	pointB, err := vec3DB.ToPoint3D()
	if err != nil {
		return err
	}

	fmt.Printf("Point A: %v\n", pointA)
	fmt.Printf("Point B: %v\n", pointB)
	fmt.Printf("Distance A to B: %.6f\n", pointA.Distance(pointB))

	// Utility vector constructors
	fmt.Println("\nUtility constructors:")
	zeros, err := eigen_math.NewZeroVector(5)
	if err != nil {
		return err
	}
	defer zeros.Close()

	ones, err := eigen_math.NewOnesVector(4)
	if err != nil {
		return err
	}
	defer ones.Close()

	unitX, err := eigen_math.NewUnitVector(3, 0)
	if err != nil {
		return err
	}
	defer unitX.Close()

	fmt.Printf("Zero vector (5D): %v\n", zeros)
	fmt.Printf("Ones vector (4D): %v\n", ones)
	fmt.Printf("Unit vector X (3D): %v\n", unitX)

	fmt.Println("✓ Vector operations completed successfully\n")
	return nil
}

func demonstrateAdvancedLinearAlgebra() error {
	fmt.Println("3. Advanced Linear Algebra")
	fmt.Println("=========================\n")

	// Create a symmetric matrix for eigendecomposition
	fmt.Println("Eigenvalue Decomposition:")
	symData := [][]float64{
		{4, 2, 1},
		{2, 3, 0},
		{1, 0, 2},
	}
	symMatrix, err := eigen_math.NewMatrix(symData)
	if err != nil {
		return err
	}
	defer symMatrix.Close()

	fmt.Printf("Symmetric Matrix:\n%v\n", symMatrix)

	eigenvals, eigenvecs, err := symMatrix.EigenDecomposition()
	if err != nil {
		return err
	}
	defer eigenvals.Close()
	defer eigenvecs.Close()

	fmt.Printf("Eigenvalues: %v\n", eigenvals)
	fmt.Printf("Eigenvectors:\n%v\n", eigenvecs)

	// SVD Decomposition
	fmt.Println("Singular Value Decomposition (SVD):")
	testData := [][]float64{
		{1, 2, 3},
		{4, 5, 6},
		{7, 8, 9},
		{10, 11, 12},
	}
	testMatrix, err := eigen_math.NewMatrix(testData)
	if err != nil {
		return err
	}
	defer testMatrix.Close()

	fmt.Printf("Test Matrix:\n%v\n", testMatrix)

	U, S, Vt, err := testMatrix.SVD()
	if err != nil {
		return err
	}
	defer U.Close()
	defer S.Close()
	defer Vt.Close()

	fmt.Printf("U Matrix:\n%v\n", U)
	fmt.Printf("Singular Values: %v\n", S)
	fmt.Printf("V^T Matrix:\n%v\n", Vt)

	// Linear System Solving
	fmt.Println("Linear System Solving (Ax = b):")
	A, err := eigen_math.NewMatrix([][]float64{
		{3, 2, 1},
		{2, 3, 2},
		{1, 2, 3},
	})
	if err != nil {
		return err
	}
	defer A.Close()

	b, err := eigen_math.NewVector([]float64{6, 7, 8})
	if err != nil {
		return err
	}
	defer b.Close()

	fmt.Printf("Coefficient Matrix A:\n%v\n", A)
	fmt.Printf("Right-hand side b: %v\n", b)

	x, err := eigen_math.SolveLinearSystem(A, b)
	if err != nil {
		return err
	}
	defer x.Close()

	fmt.Printf("Solution x: %v\n", x)

	// Verify solution by computing A*x
	fmt.Println("Verification: Computing A*x...")
	// Convert x to a column matrix for multiplication
	xData, err := x.ToSlice()
	if err != nil {
		return err
	}
	xMatrixData := make([][]float64, len(xData))
	for i, val := range xData {
		xMatrixData[i] = []float64{val}
	}
	xMatrix, err := eigen_math.NewMatrix(xMatrixData)
	if err != nil {
		return err
	}
	defer xMatrix.Close()

	verification, err := A.Multiply(xMatrix)
	if err != nil {
		return err
	}
	defer verification.Close()

	fmt.Printf("A*x (should equal b):\n%v\n", verification)

	// QR Decomposition
	fmt.Println("QR Decomposition:")
	Q, R, err := eigen_math.QRDecomposition(A)
	if err != nil {
		return err
	}
	defer Q.Close()
	defer R.Close()

	fmt.Printf("Q Matrix:\n%v\n", Q)
	fmt.Printf("R Matrix:\n%v\n", R)

	fmt.Println("✓ Advanced linear algebra completed successfully\n")
	return nil
}

func demonstrateGeometricOperations() error {
	fmt.Println("4. Geometric Operations")
	fmt.Println("======================\n")

	// Euler angles to rotation matrix
	fmt.Println("Rotation Matrices and Euler Angles:")
	roll, pitch, yaw := 0.1, 0.2, 0.3 // radians
	fmt.Printf("Original Euler angles: roll=%.3f, pitch=%.3f, yaw=%.3f (radians)\n", roll, pitch, yaw)

	rollDeg := eigen_math.RadiansToDegrees(roll)
	pitchDeg := eigen_math.RadiansToDegrees(pitch)
	yawDeg := eigen_math.RadiansToDegrees(yaw)
	fmt.Printf("In degrees: roll=%.1f°, pitch=%.1f°, yaw=%.1f°\n", rollDeg, pitchDeg, yawDeg)

	rotMatrix, err := eigen_math.EulerToRotationMatrix(roll, pitch, yaw)
	if err != nil {
		return err
	}
	defer rotMatrix.Close()

	fmt.Printf("Rotation Matrix:\n%v\n", rotMatrix)

	// Convert back to Euler angles
	recoveredRoll, recoveredPitch, recoveredYaw, err := eigen_math.RotationMatrixToEuler(rotMatrix)
	if err != nil {
		return err
	}

	fmt.Printf("Recovered Euler angles: roll=%.3f, pitch=%.3f, yaw=%.3f (radians)\n", 
		recoveredRoll, recoveredPitch, recoveredYaw)

	// Check if recovery was accurate
	rollDiff := math.Abs(roll - recoveredRoll)
	pitchDiff := math.Abs(pitch - recoveredPitch)
	yawDiff := math.Abs(yaw - recoveredYaw)

	fmt.Printf("Recovery accuracy: roll_diff=%.2e, pitch_diff=%.2e, yaw_diff=%.2e\n", 
		rollDiff, pitchDiff, yawDiff)

	// Transformation matrices
	fmt.Println("\nTransformation Matrices:")
	tx, ty, tz := 1.0, 2.0, 3.0
	fmt.Printf("Translation: [%.1f, %.1f, %.1f]\n", tx, ty, tz)

	transformMatrix, err := eigen_math.CreateTransformationMatrix(tx, ty, tz, rotMatrix)
	if err != nil {
		return err
	}
	defer transformMatrix.Close()

	fmt.Printf("4×4 Transformation Matrix:\n%v\n", transformMatrix)

	// 2D rotation matrix
	fmt.Println("2D Rotation:")
	angle := math.Pi / 4 // 45 degrees
	fmt.Printf("Rotation angle: %.1f° (π/4 radians)\n", eigen_math.RadiansToDegrees(angle))

	rot2D, err := eigen_math.NewRotationMatrix2D(angle)
	if err != nil {
		return err
	}
	defer rot2D.Close()

	fmt.Printf("2D Rotation Matrix:\n%v\n", rot2D)

	// Apply rotation to a 2D point
	point, err := eigen_math.NewVector2D(1, 0) // Unit vector along x-axis
	if err != nil {
		return err
	}
	defer point.Close()

	pointData, err := point.ToSlice()
	if err != nil {
		return err
	}
	pointMatrix, err := eigen_math.NewMatrix([][]float64{{pointData[0]}, {pointData[1]}})
	if err != nil {
		return err
	}
	defer pointMatrix.Close()

	rotatedMatrix, err := rot2D.Multiply(pointMatrix)
	if err != nil {
		return err
	}
	defer rotatedMatrix.Close()

	fmt.Printf("Original point: [%.3f, %.3f]\n", pointData[0], pointData[1])
	
	rotatedData, err := rotatedMatrix.ToSlice()
	if err != nil {
		return err
	}
	fmt.Printf("Rotated point: [%.3f, %.3f]\n", rotatedData[0][0], rotatedData[1][0])

	// Quaternions
	fmt.Println("\nQuaternion Operations:")
	axis := eigen_math.Point3D{X: 0, Y: 0, Z: 1} // Z-axis
	quat := eigen_math.NewQuaternionFromAxisAngle(axis, angle)
	fmt.Printf("Quaternion from axis-angle: %v\n", quat)

	quatNormalized := quat.Normalize()
	fmt.Printf("Normalized quaternion: %v\n", quatNormalized)

	quatRotMatrix, err := quatNormalized.ToRotationMatrix()
	if err != nil {
		return err
	}
	defer quatRotMatrix.Close()

	fmt.Printf("Rotation matrix from quaternion:\n%v\n", quatRotMatrix)

	// EulerAngles type
	fmt.Println("\nEulerAngles Type:")
	euler := eigen_math.NewEulerAngles(roll, pitch, yaw)
	fmt.Printf("EulerAngles: %v\n", euler)
	fmt.Printf("EulerAngles in degrees: %v\n", euler.StringDegrees())

	eulerFromDeg := eigen_math.NewEulerAnglesDegrees(30, 60, 90)
	fmt.Printf("From degrees (30°, 60°, 90°): %v\n", eulerFromDeg.StringDegrees())

	fmt.Println("✓ Geometric operations completed successfully\n")
	return nil
}

func demonstrateTypeSystem() error {
	fmt.Println("5. Type System and Utilities")
	fmt.Println("===========================\n")

	// Convenience matrix constructors
	fmt.Println("Matrix Utility Constructors:")
	
	zeros, err := eigen_math.NewZeroMatrix(2, 3)
	if err != nil {
		return err
	}
	defer zeros.Close()

	ones, err := eigen_math.NewOnesMatrix(2, 3)
	if err != nil {
		return err
	}
	defer ones.Close()

	diagonal, err := eigen_math.NewDiagonalMatrix([]float64{1, 2, 3})
	if err != nil {
		return err
	}
	defer diagonal.Close()

	fmt.Printf("Zero matrix (2×3):\n%v\n", zeros)
	fmt.Printf("Ones matrix (2×3):\n%v\n", ones)
	fmt.Printf("Diagonal matrix:\n%v\n", diagonal)

	// Point operations
	fmt.Println("Point Operations:")
	p1 := eigen_math.Point2D{X: 0, Y: 0}
	p2 := eigen_math.Point2D{X: 3, Y: 4}
	
	fmt.Printf("2D Points: %v and %v\n", p1, p2)
	fmt.Printf("Distance: %.3f\n", p1.Distance(p2))

	p3D1 := eigen_math.Point3D{X: 0, Y: 0, Z: 0}
	p3D2 := eigen_math.Point3D{X: 1, Y: 1, Z: 1}
	
	fmt.Printf("3D Points: %v and %v\n", p3D1, p3D2)
	fmt.Printf("Distance: %.3f\n", p3D1.Distance(p3D2))

	// Convert points to vectors
	vec2D, err := p2.ToVector()
	if err != nil {
		return err
	}
	defer vec2D.Close()

	vec3D, err := p3D2.ToVector()
	if err != nil {
		return err
	}
	defer vec3D.Close()

	fmt.Printf("Point as 2D vector: %v\n", vec2D)
	fmt.Printf("Point as 3D vector: %v\n", vec3D)

	// Utility functions
	fmt.Println("\nUtility Functions:")
	fmt.Printf("π = %.6f\n", eigen_math.Pi)
	fmt.Printf("e = %.6f\n", eigen_math.E)

	degrees := 180.0
	radians := eigen_math.DegreesToRadians(degrees)
	backToDegrees := eigen_math.RadiansToDegrees(radians)
	
	fmt.Printf("%.1f° = %.6f radians = %.1f°\n", degrees, radians, backToDegrees)

	// Approximate equality
	a, b := 0.1 + 0.2, 0.3
	fmt.Printf("0.1 + 0.2 = %.17f\n", a)
	fmt.Printf("0.3 = %.17f\n", b)
	fmt.Printf("Are they equal? %v\n", a == b)
	fmt.Printf("Are they approximately equal? %v\n", 
		eigen_math.ApproximatelyEqual(a, b, 1e-10))

	// Clamp and interpolation
	value := 1.5
	clamped := eigen_math.Clamp(value, 0, 1)
	fmt.Printf("Clamp(%.1f, 0, 1) = %.1f\n", value, clamped)

	interpolated := eigen_math.Lerp(0, 10, 0.3)
	fmt.Printf("Lerp(0, 10, 0.3) = %.1f\n", interpolated)

	// String representations
	fmt.Println("\nString Representations:")
	testMatrix, err := eigen_math.NewMatrix([][]float64{{1, 2}, {3, 4}})
	if err != nil {
		return err
	}
	defer testMatrix.Close()

	testVector, err := eigen_math.NewVector([]float64{1, 2, 3})
	if err != nil {
		return err
	}
	defer testVector.Close()

	fmt.Printf("Matrix String():\n%v\n", testMatrix)
	fmt.Printf("Vector String(): %v\n", testVector)

	// Matrix info
	info, err := testMatrix.Info()
	if err != nil {
		return err
	}
	fmt.Printf("Matrix Info: %s\n", info)

	fmt.Println("✓ Type system demonstration completed successfully\n")
	return nil
}

func demonstratePerformance() error {
	fmt.Println("6. Performance Benchmarks")
	fmt.Println("========================\n")

	// Matrix multiplication benchmarks
	sizes := []int{50, 100, 200}
	iterations := 10

	fmt.Println("Matrix Multiplication Performance:")
	fmt.Printf("%-8s %-15s %-15s\n", "Size", "Time (ms/op)", "Operations/sec")
	fmt.Println(strings.Repeat("-", 40))

	for _, size := range sizes {
		timeMs, err := eigen_math.BenchmarkMatrixMultiply(size, iterations)
		if err != nil {
			return fmt.Errorf("benchmark failed for size %d: %w", size, err)
		}

		opsPerSec := 1000.0 / timeMs
		fmt.Printf("%-8d %-15.3f %-15.1f\n", size, timeMs, opsPerSec)
	}

	// Manual benchmark for comparison
	fmt.Println("\nManual Go Benchmark (Matrix Multiplication):")
	size := 100
	matA, err := eigen_math.NewRandomMatrix(size, size, 42)
	if err != nil {
		return err
	}
	defer matA.Close()

	matB, err := eigen_math.NewRandomMatrix(size, size, 43)
	if err != nil {
		return err
	}
	defer matB.Close()

	// Warm-up
	result, err := matA.Multiply(matB)
	if err != nil {
		return err
	}
	result.Close()

	// Actual benchmark
	start := time.Now()
	numOps := 10
	for i := 0; i < numOps; i++ {
		result, err := matA.Multiply(matB)
		if err != nil {
			return err
		}
		result.Close()
	}
	elapsed := time.Since(start)
	avgTimeMs := float64(elapsed.Milliseconds()) / float64(numOps)

	fmt.Printf("Manual benchmark (%dx%d): %.3f ms/op\n", size, size, avgTimeMs)

	// Memory usage demonstration
	fmt.Println("\nMemory Usage Demonstration:")
	fmt.Println("Creating and destroying matrices...")

	startMem := getMemoryStats()
	
	// Create many matrices
	var matrices []*eigen_math.Matrix
	for i := 0; i < 100; i++ {
		matrix, err := eigen_math.NewRandomMatrix(50, 50, int(time.Now().UnixNano())%1000)
		if err != nil {
			return err
		}
		matrices = append(matrices, matrix)
	}

	midMem := getMemoryStats()

	// Clean up
	for _, matrix := range matrices {
		matrix.Close()
	}

	endMem := getMemoryStats()

	fmt.Printf("Memory before: %d KB\n", startMem/1024)
	fmt.Printf("Memory after creating 100 matrices: %d KB\n", midMem/1024)
	fmt.Printf("Memory after cleanup: %d KB\n", endMem/1024)

	// Vector operations performance
	fmt.Println("\nVector Operations Performance:")
	vectorSize := 10000
	vecA, err := eigen_math.NewRandomVector(vectorSize, 42)
	if err != nil {
		return err
	}
	defer vecA.Close()

	vecB, err := eigen_math.NewRandomVector(vectorSize, 43)
	if err != nil {
		return err
	}
	defer vecB.Close()

	// Dot product benchmark
	start = time.Now()
	numDotOps := 1000
	for i := 0; i < numDotOps; i++ {
		_, err := vecA.Dot(vecB)
		if err != nil {
			return err
		}
	}
	elapsed = time.Since(start)
	dotTimeUs := float64(elapsed.Microseconds()) / float64(numDotOps)
	fmt.Printf("Dot product (%d elements): %.3f μs/op\n", vectorSize, dotTimeUs)

	// Norm benchmark
	start = time.Now()
	numNormOps := 1000
	for i := 0; i < numNormOps; i++ {
		_, err := vecA.Norm()
		if err != nil {
			return err
		}
	}
	elapsed = time.Since(start)
	normTimeUs := float64(elapsed.Microseconds()) / float64(numNormOps)
	fmt.Printf("Vector norm (%d elements): %.3f μs/op\n", vectorSize, normTimeUs)

	fmt.Println("✓ Performance demonstration completed successfully\n")
	return nil
}

// Helper function for memory stats (simplified)
func getMemoryStats() uint64 {
	// This is a simplified version - in a real application,
	// you might use runtime.MemStats for detailed memory statistics
	var m runtime.MemStats
	runtime.ReadMemStats(&m)
	return m.Alloc
}

