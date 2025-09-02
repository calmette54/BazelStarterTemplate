// Package eigen_math - Additional types and utilities
package eigen_math

import (
	"fmt"
	"math"
	"strings"
)

// ============================================================================
// Mathematical Constants
// ============================================================================

const (
	// Mathematical constants
	Pi       = math.Pi
	E        = math.E
	Sqrt2    = math.Sqrt2
	Ln2      = math.Ln2
	Ln10     = math.Ln10
	Log2E    = math.Log2E
	Log10E   = math.Log10E
	
	// Precision constants
	DefaultTolerance = 1e-10
	LooseTolerance   = 1e-6
	TightTolerance   = 1e-12
)

// ============================================================================
// Convenience Types
// ============================================================================

// Point2D represents a 2D point
type Point2D struct {
	X, Y float64
}

// Point3D represents a 3D point
type Point3D struct {
	X, Y, Z float64
}

// Quaternion represents a quaternion for 3D rotations
type Quaternion struct {
	W, X, Y, Z float64
}

// EulerAngles represents Euler angles in radians
type EulerAngles struct {
	Roll, Pitch, Yaw float64
}

// ============================================================================
// Matrix String Representation
// ============================================================================

// String returns a string representation of the matrix
func (m *Matrix) String() string {
	if m.handle == nil {
		return "Matrix(nil)"
	}
	
	data, err := m.ToSlice()
	if err != nil {
		return fmt.Sprintf("Matrix(error: %v)", err)
	}
	
	var builder strings.Builder
	builder.WriteString("Matrix(\n")
	
	for i, row := range data {
		builder.WriteString("  [")
		for j, val := range row {
			if j > 0 {
				builder.WriteString(", ")
			}
			builder.WriteString(fmt.Sprintf("%8.4f", val))
		}
		builder.WriteString("]")
		if i < len(data)-1 {
			builder.WriteString(",")
		}
		builder.WriteString("\n")
	}
	
	builder.WriteString(")")
	return builder.String()
}

// GoString returns a Go syntax representation of the matrix
func (m *Matrix) GoString() string {
	if m.handle == nil {
		return "(*Matrix)(nil)"
	}
	
	data, err := m.ToSlice()
	if err != nil {
		return fmt.Sprintf("(*Matrix)(error: %v)", err)
	}
	
	var builder strings.Builder
	builder.WriteString("&Matrix{")
	builder.WriteString(fmt.Sprintf("rows: %d, cols: %d, data: [][]float64{", m.rows, m.cols))
	
	for i, row := range data {
		if i > 0 {
			builder.WriteString(", ")
		}
		builder.WriteString("{")
		for j, val := range row {
			if j > 0 {
				builder.WriteString(", ")
			}
			builder.WriteString(fmt.Sprintf("%g", val))
		}
		builder.WriteString("}")
	}
	
	builder.WriteString("}}")
	return builder.String()
}

// ============================================================================
// Vector String Representation
// ============================================================================

// String returns a string representation of the vector
func (v *Vector) String() string {
	if v.handle == nil {
		return "Vector(nil)"
	}
	
	data, err := v.ToSlice()
	if err != nil {
		return fmt.Sprintf("Vector(error: %v)", err)
	}
	
	var builder strings.Builder
	builder.WriteString("Vector([")
	
	for i, val := range data {
		if i > 0 {
			builder.WriteString(", ")
		}
		builder.WriteString(fmt.Sprintf("%8.4f", val))
	}
	
	builder.WriteString("])")
	return builder.String()
}

// GoString returns a Go syntax representation of the vector
func (v *Vector) GoString() string {
	if v.handle == nil {
		return "(*Vector)(nil)"
	}
	
	data, err := v.ToSlice()
	if err != nil {
		return fmt.Sprintf("(*Vector)(error: %v)", err)
	}
	
	var builder strings.Builder
	builder.WriteString("&Vector{")
	builder.WriteString(fmt.Sprintf("size: %d, data: []float64{", v.size))
	
	for i, val := range data {
		if i > 0 {
			builder.WriteString(", ")
		}
		builder.WriteString(fmt.Sprintf("%g", val))
	}
	
	builder.WriteString("}}")
	return builder.String()
}

// ============================================================================
// Convenience Matrix Constructors
// ============================================================================

// NewZeroMatrix creates a zero matrix with specified dimensions
func NewZeroMatrix(rows, cols int) (*Matrix, error) {
	if rows <= 0 || cols <= 0 {
		return nil, fmt.Errorf("matrix dimensions must be positive, got %dx%d", rows, cols)
	}
	
	data := make([][]float64, rows)
	for i := range data {
		data[i] = make([]float64, cols)
	}
	
	return NewMatrix(data)
}

// NewOnesMatrix creates a matrix filled with ones
func NewOnesMatrix(rows, cols int) (*Matrix, error) {
	if rows <= 0 || cols <= 0 {
		return nil, fmt.Errorf("matrix dimensions must be positive, got %dx%d", rows, cols)
	}
	
	data := make([][]float64, rows)
	for i := range data {
		data[i] = make([]float64, cols)
		for j := range data[i] {
			data[i][j] = 1.0
		}
	}
	
	return NewMatrix(data)
}

// NewDiagonalMatrix creates a diagonal matrix from a vector of diagonal elements
func NewDiagonalMatrix(diagonal []float64) (*Matrix, error) {
	if len(diagonal) == 0 {
		return nil, fmt.Errorf("diagonal vector cannot be empty")
	}
	
	size := len(diagonal)
	data := make([][]float64, size)
	
	for i := 0; i < size; i++ {
		data[i] = make([]float64, size)
		data[i][i] = diagonal[i]
	}
	
	return NewMatrix(data)
}

// NewRotationMatrix2D creates a 2D rotation matrix
func NewRotationMatrix2D(angle float64) (*Matrix, error) {
	cos := math.Cos(angle)
	sin := math.Sin(angle)
	
	data := [][]float64{
		{cos, -sin},
		{sin, cos},
	}
	
	return NewMatrix(data)
}

// ============================================================================
// Convenience Vector Constructors
// ============================================================================

// NewZeroVector creates a zero vector with specified dimension
func NewZeroVector(size int) (*Vector, error) {
	if size <= 0 {
		return nil, fmt.Errorf("vector size must be positive, got %d", size)
	}
	
	data := make([]float64, size)
	return NewVector(data)
}

// NewOnesVector creates a vector filled with ones
func NewOnesVector(size int) (*Vector, error) {
	if size <= 0 {
		return nil, fmt.Errorf("vector size must be positive, got %d", size)
	}
	
	data := make([]float64, size)
	for i := range data {
		data[i] = 1.0
	}
	
	return NewVector(data)
}

// NewVector3D creates a 3D vector from individual components
func NewVector3D(x, y, z float64) (*Vector, error) {
	return NewVector([]float64{x, y, z})
}

// NewVector2D creates a 2D vector from individual components
func NewVector2D(x, y float64) (*Vector, error) {
	return NewVector([]float64{x, y})
}

// NewUnitVector creates a unit vector in the specified direction (zero-indexed)
func NewUnitVector(size, direction int) (*Vector, error) {
	if size <= 0 {
		return nil, fmt.Errorf("vector size must be positive, got %d", size)
	}
	if direction < 0 || direction >= size {
		return nil, fmt.Errorf("direction %d out of range [0, %d)", direction, size)
	}
	
	data := make([]float64, size)
	data[direction] = 1.0
	
	return NewVector(data)
}

// ============================================================================
// Vector Utility Methods
// ============================================================================

// ToPoint2D converts a 2D vector to a Point2D
func (v *Vector) ToPoint2D() (Point2D, error) {
	if v.size != 2 {
		return Point2D{}, fmt.Errorf("vector must be 2D, got size %d", v.size)
	}
	
	data, err := v.ToSlice()
	if err != nil {
		return Point2D{}, err
	}
	
	return Point2D{X: data[0], Y: data[1]}, nil
}

// ToPoint3D converts a 3D vector to a Point3D
func (v *Vector) ToPoint3D() (Point3D, error) {
	if v.size != 3 {
		return Point3D{}, fmt.Errorf("vector must be 3D, got size %d", v.size)
	}
	
	data, err := v.ToSlice()
	if err != nil {
		return Point3D{}, err
	}
	
	return Point3D{X: data[0], Y: data[1], Z: data[2]}, nil
}

// ============================================================================
// Point Utility Functions
// ============================================================================

// ToVector converts a Point2D to a vector
func (p Point2D) ToVector() (*Vector, error) {
	return NewVector([]float64{p.X, p.Y})
}

// ToVector converts a Point3D to a vector
func (p Point3D) ToVector() (*Vector, error) {
	return NewVector([]float64{p.X, p.Y, p.Z})
}

// Distance calculates the Euclidean distance between two 2D points
func (p Point2D) Distance(other Point2D) float64 {
	dx := p.X - other.X
	dy := p.Y - other.Y
	return math.Sqrt(dx*dx + dy*dy)
}

// Distance calculates the Euclidean distance between two 3D points
func (p Point3D) Distance(other Point3D) float64 {
	dx := p.X - other.X
	dy := p.Y - other.Y
	dz := p.Z - other.Z
	return math.Sqrt(dx*dx + dy*dy + dz*dz)
}

// String returns string representation of Point2D
func (p Point2D) String() string {
	return fmt.Sprintf("Point2D(%.4f, %.4f)", p.X, p.Y)
}

// String returns string representation of Point3D
func (p Point3D) String() string {
	return fmt.Sprintf("Point3D(%.4f, %.4f, %.4f)", p.X, p.Y, p.Z)
}

// ============================================================================
// Quaternion Utility Functions
// ============================================================================

// NewQuaternion creates a quaternion with specified components
func NewQuaternion(w, x, y, z float64) Quaternion {
	return Quaternion{W: w, X: x, Y: y, Z: z}
}

// NewQuaternionFromAxisAngle creates a quaternion from axis-angle representation
func NewQuaternionFromAxisAngle(axis Point3D, angle float64) Quaternion {
	halfAngle := angle * 0.5
	sinHalf := math.Sin(halfAngle)
	cosHalf := math.Cos(halfAngle)
	
	// Normalize axis
	length := math.Sqrt(axis.X*axis.X + axis.Y*axis.Y + axis.Z*axis.Z)
	if length == 0 {
		return Quaternion{W: 1, X: 0, Y: 0, Z: 0}
	}
	
	axis.X /= length
	axis.Y /= length
	axis.Z /= length
	
	return Quaternion{
		W: cosHalf,
		X: axis.X * sinHalf,
		Y: axis.Y * sinHalf,
		Z: axis.Z * sinHalf,
	}
}

// Normalize returns a normalized quaternion
func (q Quaternion) Normalize() Quaternion {
	length := math.Sqrt(q.W*q.W + q.X*q.X + q.Y*q.Y + q.Z*q.Z)
	if length == 0 {
		return Quaternion{W: 1, X: 0, Y: 0, Z: 0}
	}
	
	return Quaternion{
		W: q.W / length,
		X: q.X / length,
		Y: q.Y / length,
		Z: q.Z / length,
	}
}

// ToRotationMatrix converts quaternion to a 3x3 rotation matrix
func (q Quaternion) ToRotationMatrix() (*Matrix, error) {
	// Normalize quaternion first
	qn := q.Normalize()
	
	w, x, y, z := qn.W, qn.X, qn.Y, qn.Z
	
	data := [][]float64{
		{1 - 2*(y*y + z*z), 2*(x*y - w*z), 2*(x*z + w*y)},
		{2*(x*y + w*z), 1 - 2*(x*x + z*z), 2*(y*z - w*x)},
		{2*(x*z - w*y), 2*(y*z + w*x), 1 - 2*(x*x + y*y)},
	}
	
	return NewMatrix(data)
}

// String returns string representation of Quaternion
func (q Quaternion) String() string {
	return fmt.Sprintf("Quaternion(w=%.4f, x=%.4f, y=%.4f, z=%.4f)", q.W, q.X, q.Y, q.Z)
}

// ============================================================================
// EulerAngles Utility Functions
// ============================================================================

// NewEulerAngles creates Euler angles from individual components (in radians)
func NewEulerAngles(roll, pitch, yaw float64) EulerAngles {
	return EulerAngles{Roll: roll, Pitch: pitch, Yaw: yaw}
}

// NewEulerAnglesDegrees creates Euler angles from individual components (in degrees)
func NewEulerAnglesDegrees(roll, pitch, yaw float64) EulerAngles {
	return EulerAngles{
		Roll:  roll * Pi / 180.0,
		Pitch: pitch * Pi / 180.0,
		Yaw:   yaw * Pi / 180.0,
	}
}

// ToDegrees converts Euler angles from radians to degrees
func (e EulerAngles) ToDegrees() EulerAngles {
	return EulerAngles{
		Roll:  e.Roll * 180.0 / Pi,
		Pitch: e.Pitch * 180.0 / Pi,
		Yaw:   e.Yaw * 180.0 / Pi,
	}
}

// ToRotationMatrix converts Euler angles to a rotation matrix
func (e EulerAngles) ToRotationMatrix() (*Matrix, error) {
	return EulerToRotationMatrix(e.Roll, e.Pitch, e.Yaw)
}

// String returns string representation of EulerAngles (in radians)
func (e EulerAngles) String() string {
	return fmt.Sprintf("EulerAngles(roll=%.4f, pitch=%.4f, yaw=%.4f)", e.Roll, e.Pitch, e.Yaw)
}

// StringDegrees returns string representation of EulerAngles (in degrees)
func (e EulerAngles) StringDegrees() string {
	deg := e.ToDegrees()
	return fmt.Sprintf("EulerAngles(roll=%.2f°, pitch=%.2f°, yaw=%.2f°)", deg.Roll, deg.Pitch, deg.Yaw)
}

// ============================================================================
// Utility Functions
// ============================================================================

// DegreesToRadians converts degrees to radians
func DegreesToRadians(degrees float64) float64 {
	return degrees * Pi / 180.0
}

// RadiansToDegrees converts radians to degrees
func RadiansToDegrees(radians float64) float64 {
	return radians * 180.0 / Pi
}

// ApproximatelyEqual checks if two floating point numbers are approximately equal
func ApproximatelyEqual(a, b, tolerance float64) bool {
	return math.Abs(a-b) <= tolerance
}

// Clamp clamps a value between min and max
func Clamp(value, min, max float64) float64 {
	if value < min {
		return min
	}
	if value > max {
		return max
	}
	return value
}

// Lerp performs linear interpolation between two values
func Lerp(a, b, t float64) float64 {
	return a + t*(b-a)
}