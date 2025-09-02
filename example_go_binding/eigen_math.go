// Package eigen_math provides Go bindings for Eigen linear algebra operations
package eigen_math

/*
#include "eigen_math_wrapper.h"
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"fmt"
	"runtime"
)

// ============================================================================
// Error Handling
// ============================================================================

// EigenError represents an error from the Eigen library
type EigenError struct {
	Code    int
	Message string
}

func (e *EigenError) Error() string {
	return fmt.Sprintf("Eigen error (%d): %s", e.Code, e.Message)
}

// Convert C error code to Go error
func cErrorToGoError(result C.EigenResult) error {
	if result == C.EIGEN_SUCCESS {
		return nil
	}
	
	errorMsg := C.GoString(C.eigen_get_error_message(result))
	return &EigenError{
		Code:    int(result),
		Message: errorMsg,
	}
}

// ============================================================================
// Matrix Type and Operations
// ============================================================================

// Matrix represents an Eigen matrix with automatic memory management
type Matrix struct {
	handle C.EigenMatrixHandle
	rows   int
	cols   int
}

// NewMatrix creates a new matrix from Go slice data (row-major order)
func NewMatrix(data [][]float64) (*Matrix, error) {
	if len(data) == 0 || len(data[0]) == 0 {
		return nil, errors.New("matrix data cannot be empty")
	}
	
	rows := len(data)
	cols := len(data[0])
	
	// Validate that all rows have the same length
	for i, row := range data {
		if len(row) != cols {
			return nil, fmt.Errorf("row %d has length %d, expected %d", i, len(row), cols)
		}
	}
	
	// Flatten to C-compatible array (row-major)
	flatData := make([]C.double, rows*cols)
	for i := 0; i < rows; i++ {
		for j := 0; j < cols; j++ {
			flatData[i*cols+j] = C.double(data[i][j])
		}
	}
	
	var handle C.EigenMatrixHandle
	result := C.eigen_create_matrix_from_data(
		&flatData[0], 
		C.int(rows), 
		C.int(cols), 
		&handle,
	)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	matrix := &Matrix{
		handle: handle,
		rows:   rows,
		cols:   cols,
	}
	
	// Set finalizer for automatic cleanup
	runtime.SetFinalizer(matrix, (*Matrix).finalize)
	return matrix, nil
}

// NewRandomMatrix creates a random matrix with specified dimensions
func NewRandomMatrix(rows, cols int, seed ...int) (*Matrix, error) {
	if rows <= 0 || cols <= 0 {
		return nil, errors.New("matrix dimensions must be positive")
	}
	
	seedValue := 42
	if len(seed) > 0 {
		seedValue = seed[0]
	}
	
	var handle C.EigenMatrixHandle
	result := C.eigen_create_random_matrix(
		C.int(rows),
		C.int(cols),
		C.int(seedValue),
		&handle,
	)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	matrix := &Matrix{
		handle: handle,
		rows:   rows,
		cols:   cols,
	}
	
	runtime.SetFinalizer(matrix, (*Matrix).finalize)
	return matrix, nil
}

// NewIdentityMatrix creates an identity matrix of specified size
func NewIdentityMatrix(size int) (*Matrix, error) {
	if size <= 0 {
		return nil, errors.New("matrix size must be positive")
	}
	
	var handle C.EigenMatrixHandle
	result := C.eigen_create_identity_matrix(C.int(size), &handle)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	matrix := &Matrix{
		handle: handle,
		rows:   size,
		cols:   size,
	}
	
	runtime.SetFinalizer(matrix, (*Matrix).finalize)
	return matrix, nil
}

// Rows returns the number of rows in the matrix
func (m *Matrix) Rows() int {
	return m.rows
}

// Cols returns the number of columns in the matrix
func (m *Matrix) Cols() int {
	return m.cols
}

// ToSlice returns the matrix data as a Go slice (row-major order)
func (m *Matrix) ToSlice() ([][]float64, error) {
	if m.handle == nil {
		return nil, errors.New("matrix handle is nil")
	}
	
	// Get matrix data
	data := make([]C.double, m.rows*m.cols)
	var rows, cols C.int
	
	result := C.eigen_get_matrix_data(m.handle, &data[0], &rows, &cols)
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	// Convert to Go slice
	goData := make([][]float64, int(rows))
	for i := 0; i < int(rows); i++ {
		goData[i] = make([]float64, int(cols))
		for j := 0; j < int(cols); j++ {
			goData[i][j] = float64(data[i*int(cols)+j])
		}
	}
	
	return goData, nil
}

// Multiply performs matrix multiplication
func (m *Matrix) Multiply(other *Matrix) (*Matrix, error) {
	if m.handle == nil || other.handle == nil {
		return nil, errors.New("matrix handles are nil")
	}
	
	var resultHandle C.EigenMatrixHandle
	result := C.eigen_multiply_matrices(m.handle, other.handle, &resultHandle)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	// Get dimensions of result matrix
	var rows, cols C.int
	C.eigen_get_matrix_dimensions(resultHandle, &rows, &cols)
	
	resultMatrix := &Matrix{
		handle: resultHandle,
		rows:   int(rows),
		cols:   int(cols),
	}
	
	runtime.SetFinalizer(resultMatrix, (*Matrix).finalize)
	return resultMatrix, nil
}

// Transpose returns the transpose of the matrix
func (m *Matrix) Transpose() (*Matrix, error) {
	if m.handle == nil {
		return nil, errors.New("matrix handle is nil")
	}
	
	var resultHandle C.EigenMatrixHandle
	result := C.eigen_transpose_matrix(m.handle, &resultHandle)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	resultMatrix := &Matrix{
		handle: resultHandle,
		rows:   m.cols,
		cols:   m.rows,
	}
	
	runtime.SetFinalizer(resultMatrix, (*Matrix).finalize)
	return resultMatrix, nil
}

// Inverse returns the inverse of the matrix
func (m *Matrix) Inverse() (*Matrix, error) {
	if m.handle == nil {
		return nil, errors.New("matrix handle is nil")
	}
	
	var resultHandle C.EigenMatrixHandle
	result := C.eigen_invert_matrix(m.handle, &resultHandle)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	resultMatrix := &Matrix{
		handle: resultHandle,
		rows:   m.rows,
		cols:   m.cols,
	}
	
	runtime.SetFinalizer(resultMatrix, (*Matrix).finalize)
	return resultMatrix, nil
}

// Determinant returns the determinant of the matrix
func (m *Matrix) Determinant() (float64, error) {
	if m.handle == nil {
		return 0, errors.New("matrix handle is nil")
	}
	
	var det C.double
	result := C.eigen_determinant(m.handle, &det)
	
	if err := cErrorToGoError(result); err != nil {
		return 0, err
	}
	
	return float64(det), nil
}

// EigenDecomposition computes eigenvalues and eigenvectors
func (m *Matrix) EigenDecomposition() (*Vector, *Matrix, error) {
	if m.handle == nil {
		return nil, nil, errors.New("matrix handle is nil")
	}
	
	var eigenvaluesHandle C.EigenVectorHandle
	var eigenvectorsHandle C.EigenMatrixHandle
	
	result := C.eigen_eigen_decomposition(m.handle, &eigenvaluesHandle, &eigenvectorsHandle)
	if err := cErrorToGoError(result); err != nil {
		return nil, nil, err
	}
	
	// Create eigenvalues vector
	var valSize C.int
	C.eigen_get_vector_size(eigenvaluesHandle, &valSize)
	eigenvalues := &Vector{
		handle: eigenvaluesHandle,
		size:   int(valSize),
	}
	runtime.SetFinalizer(eigenvalues, (*Vector).finalize)
	
	// Create eigenvectors matrix
	var vecRows, vecCols C.int
	C.eigen_get_matrix_dimensions(eigenvectorsHandle, &vecRows, &vecCols)
	eigenvectors := &Matrix{
		handle: eigenvectorsHandle,
		rows:   int(vecRows),
		cols:   int(vecCols),
	}
	runtime.SetFinalizer(eigenvectors, (*Matrix).finalize)
	
	return eigenvalues, eigenvectors, nil
}

// SVD computes Singular Value Decomposition
func (m *Matrix) SVD() (*Matrix, *Vector, *Matrix, error) {
	if m.handle == nil {
		return nil, nil, nil, errors.New("matrix handle is nil")
	}
	
	var uHandle C.EigenMatrixHandle
	var sHandle C.EigenVectorHandle  
	var vtHandle C.EigenMatrixHandle
	
	result := C.eigen_svd_decomposition(m.handle, &uHandle, &sHandle, &vtHandle)
	if err := cErrorToGoError(result); err != nil {
		return nil, nil, nil, err
	}
	
	// Create U matrix
	var uRows, uCols C.int
	C.eigen_get_matrix_dimensions(uHandle, &uRows, &uCols)
	U := &Matrix{handle: uHandle, rows: int(uRows), cols: int(uCols)}
	runtime.SetFinalizer(U, (*Matrix).finalize)
	
	// Create singular values vector
	var sSize C.int
	C.eigen_get_vector_size(sHandle, &sSize)
	S := &Vector{handle: sHandle, size: int(sSize)}
	runtime.SetFinalizer(S, (*Vector).finalize)
	
	// Create Vt matrix
	var vtRows, vtCols C.int
	C.eigen_get_matrix_dimensions(vtHandle, &vtRows, &vtCols)
	Vt := &Matrix{handle: vtHandle, rows: int(vtRows), cols: int(vtCols)}
	runtime.SetFinalizer(Vt, (*Matrix).finalize)
	
	return U, S, Vt, nil
}

// Equal checks if two matrices are approximately equal
func (m *Matrix) Equal(other *Matrix, tolerance float64) (bool, error) {
	if m.handle == nil || other.handle == nil {
		return false, errors.New("matrix handles are nil")
	}
	
	var equal C.int
	result := C.eigen_matrices_equal(m.handle, other.handle, C.double(tolerance), &equal)
	
	if err := cErrorToGoError(result); err != nil {
		return false, err
	}
	
	return equal != 0, nil
}

// Info returns information about the matrix
func (m *Matrix) Info() (string, error) {
	if m.handle == nil {
		return "", errors.New("matrix handle is nil")
	}
	
	buffer := make([]C.char, 1024)
	result := C.eigen_matrix_info(m.handle, &buffer[0], C.int(len(buffer)))
	
	if err := cErrorToGoError(result); err != nil {
		return "", err
	}
	
	return C.GoString(&buffer[0]), nil
}

// finalize is called by the Go runtime to clean up the matrix
func (m *Matrix) finalize() {
	if m.handle != nil {
		C.eigen_free_matrix(m.handle)
		m.handle = nil
	}
}

// Close manually frees the matrix resources (optional, as finalizer will handle it)
func (m *Matrix) Close() {
	if m.handle != nil {
		runtime.SetFinalizer(m, nil)
		C.eigen_free_matrix(m.handle)
		m.handle = nil
	}
}

// ============================================================================
// Vector Type and Operations
// ============================================================================

// Vector represents an Eigen vector with automatic memory management
type Vector struct {
	handle C.EigenVectorHandle
	size   int
}

// NewVector creates a new vector from Go slice data
func NewVector(data []float64) (*Vector, error) {
	if len(data) == 0 {
		return nil, errors.New("vector data cannot be empty")
	}
	
	// Convert to C array
	cData := make([]C.double, len(data))
	for i, v := range data {
		cData[i] = C.double(v)
	}
	
	var handle C.EigenVectorHandle
	result := C.eigen_create_vector_from_data(&cData[0], C.int(len(data)), &handle)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	vector := &Vector{
		handle: handle,
		size:   len(data),
	}
	
	runtime.SetFinalizer(vector, (*Vector).finalize)
	return vector, nil
}

// NewRandomVector creates a random vector with specified dimension
func NewRandomVector(size int, seed ...int) (*Vector, error) {
	if size <= 0 {
		return nil, errors.New("vector size must be positive")
	}
	
	seedValue := 42
	if len(seed) > 0 {
		seedValue = seed[0]
	}
	
	var handle C.EigenVectorHandle
	result := C.eigen_create_random_vector(C.int(size), C.int(seedValue), &handle)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	vector := &Vector{
		handle: handle,
		size:   size,
	}
	
	runtime.SetFinalizer(vector, (*Vector).finalize)
	return vector, nil
}

// Size returns the size of the vector
func (v *Vector) Size() int {
	return v.size
}

// ToSlice returns the vector data as a Go slice
func (v *Vector) ToSlice() ([]float64, error) {
	if v.handle == nil {
		return nil, errors.New("vector handle is nil")
	}
	
	data := make([]C.double, v.size)
	var size C.int
	
	result := C.eigen_get_vector_data(v.handle, &data[0], &size)
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	// Convert to Go slice
	goData := make([]float64, int(size))
	for i := 0; i < int(size); i++ {
		goData[i] = float64(data[i])
	}
	
	return goData, nil
}

// Dot computes the dot product with another vector
func (v *Vector) Dot(other *Vector) (float64, error) {
	if v.handle == nil || other.handle == nil {
		return 0, errors.New("vector handles are nil")
	}
	
	var result C.double
	cResult := C.eigen_dot_product(v.handle, other.handle, &result)
	
	if err := cErrorToGoError(cResult); err != nil {
		return 0, err
	}
	
	return float64(result), nil
}

// Cross computes the cross product with another 3D vector
func (v *Vector) Cross(other *Vector) (*Vector, error) {
	if v.handle == nil || other.handle == nil {
		return nil, errors.New("vector handles are nil")
	}
	
	var resultHandle C.EigenVectorHandle
	result := C.eigen_cross_product(v.handle, other.handle, &resultHandle)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	resultVector := &Vector{
		handle: resultHandle,
		size:   3,
	}
	
	runtime.SetFinalizer(resultVector, (*Vector).finalize)
	return resultVector, nil
}

// Normalize returns the normalized (unit length) vector
func (v *Vector) Normalize() (*Vector, error) {
	if v.handle == nil {
		return nil, errors.New("vector handle is nil")
	}
	
	var resultHandle C.EigenVectorHandle
	result := C.eigen_normalize_vector(v.handle, &resultHandle)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	resultVector := &Vector{
		handle: resultHandle,
		size:   v.size,
	}
	
	runtime.SetFinalizer(resultVector, (*Vector).finalize)
	return resultVector, nil
}

// Norm returns the Euclidean norm of the vector
func (v *Vector) Norm() (float64, error) {
	if v.handle == nil {
		return 0, errors.New("vector handle is nil")
	}
	
	var norm C.double
	result := C.eigen_vector_norm(v.handle, &norm)
	
	if err := cErrorToGoError(result); err != nil {
		return 0, err
	}
	
	return float64(norm), nil
}

// finalize is called by the Go runtime to clean up the vector
func (v *Vector) finalize() {
	if v.handle != nil {
		C.eigen_free_vector(v.handle)
		v.handle = nil
	}
}

// Close manually frees the vector resources (optional, as finalizer will handle it)
func (v *Vector) Close() {
	if v.handle != nil {
		runtime.SetFinalizer(v, nil)
		C.eigen_free_vector(v.handle)
		v.handle = nil
	}
}

// ============================================================================
// Geometric Operations (Module Functions)
// ============================================================================

// EulerToRotationMatrix creates a rotation matrix from Euler angles
func EulerToRotationMatrix(roll, pitch, yaw float64) (*Matrix, error) {
	var resultHandle C.EigenMatrixHandle
	result := C.eigen_euler_to_rotation_matrix(
		C.double(roll),
		C.double(pitch), 
		C.double(yaw),
		&resultHandle,
	)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	matrix := &Matrix{
		handle: resultHandle,
		rows:   3,
		cols:   3,
	}
	
	runtime.SetFinalizer(matrix, (*Matrix).finalize)
	return matrix, nil
}

// RotationMatrixToEuler extracts Euler angles from a rotation matrix
func RotationMatrixToEuler(rotation *Matrix) (roll, pitch, yaw float64, err error) {
	if rotation.handle == nil {
		return 0, 0, 0, errors.New("rotation matrix handle is nil")
	}
	
	var cRoll, cPitch, cYaw C.double
	result := C.eigen_rotation_matrix_to_euler(rotation.handle, &cRoll, &cPitch, &cYaw)
	
	if err := cErrorToGoError(result); err != nil {
		return 0, 0, 0, err
	}
	
	return float64(cRoll), float64(cPitch), float64(cYaw), nil
}

// CreateTransformationMatrix creates a 4x4 transformation matrix
func CreateTransformationMatrix(tx, ty, tz float64, rotation *Matrix) (*Matrix, error) {
	if rotation.handle == nil {
		return nil, errors.New("rotation matrix handle is nil")
	}
	
	var resultHandle C.EigenMatrixHandle
	result := C.eigen_create_transformation_matrix(
		C.double(tx),
		C.double(ty),
		C.double(tz),
		rotation.handle,
		&resultHandle,
	)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	matrix := &Matrix{
		handle: resultHandle,
		rows:   4,
		cols:   4,
	}
	
	runtime.SetFinalizer(matrix, (*Matrix).finalize)
	return matrix, nil
}

// ============================================================================
// Linear Algebra Operations (Module Functions)
// ============================================================================

// SolveLinearSystem solves the linear system Ax = b
func SolveLinearSystem(A *Matrix, b *Vector) (*Vector, error) {
	if A.handle == nil || b.handle == nil {
		return nil, errors.New("matrix/vector handles are nil")
	}
	
	var resultHandle C.EigenVectorHandle
	result := C.eigen_solve_linear_system(A.handle, b.handle, &resultHandle)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, err
	}
	
	var size C.int
	C.eigen_get_vector_size(resultHandle, &size)
	
	vector := &Vector{
		handle: resultHandle,
		size:   int(size),
	}
	
	runtime.SetFinalizer(vector, (*Vector).finalize)
	return vector, nil
}

// QRDecomposition computes the QR decomposition of a matrix
func QRDecomposition(matrix *Matrix) (*Matrix, *Matrix, error) {
	if matrix.handle == nil {
		return nil, nil, errors.New("matrix handle is nil")
	}
	
	var qHandle, rHandle C.EigenMatrixHandle
	result := C.eigen_qr_decomposition(matrix.handle, &qHandle, &rHandle)
	
	if err := cErrorToGoError(result); err != nil {
		return nil, nil, err
	}
	
	// Create Q matrix
	var qRows, qCols C.int
	C.eigen_get_matrix_dimensions(qHandle, &qRows, &qCols)
	Q := &Matrix{handle: qHandle, rows: int(qRows), cols: int(qCols)}
	runtime.SetFinalizer(Q, (*Matrix).finalize)
	
	// Create R matrix
	var rRows, rCols C.int
	C.eigen_get_matrix_dimensions(rHandle, &rRows, &rCols)
	R := &Matrix{handle: rHandle, rows: int(rRows), cols: int(rCols)}
	runtime.SetFinalizer(R, (*Matrix).finalize)
	
	return Q, R, nil
}

// ============================================================================
// Utility Functions
// ============================================================================

// BenchmarkMatrixMultiply benchmarks matrix multiplication performance
func BenchmarkMatrixMultiply(size, iterations int) (float64, error) {
	if size <= 0 || iterations <= 0 {
		return 0, errors.New("size and iterations must be positive")
	}
	
	var timeMs C.double
	result := C.eigen_benchmark_matrix_multiplication(C.int(size), C.int(iterations), &timeMs)
	
	if err := cErrorToGoError(result); err != nil {
		return 0, err
	}
	
	return float64(timeMs), nil
}