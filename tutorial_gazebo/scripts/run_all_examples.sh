#!/bin/bash

#
# Gazebo Tutorial: Run All Examples Script
#
# This script provides an automated way to run all tutorial examples
# in sequence, with options for different execution modes and configurations.
#

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
EXAMPLES_DIR="$PROJECT_ROOT/examples"

# Default settings
RUN_MODE="demo"        # demo, build-only, interactive
TIMEOUT=30             # Seconds per example
CONTINUE_ON_ERROR=false
VERBOSE=false

# Example configurations
declare -a EXAMPLES=(
    "01_basic_world:basic_world_demo:Basic World Creation"
    "01_basic_world:world_viewer:Interactive World Viewer" 
    "01_basic_world:sdf_validator:SDF Validation Tool"
    "02_physics_demo:physics_comparison:Physics Engine Comparison"
    "03_rendering_demo:material_showcase:Rendering and Materials"
    "04_sensors_demo:sensor_platform:Sensor Simulation"
    "05_transport_demo:basic_messaging:Communication Demo"
    "06_plugins_demo:system_plugin_demo:Plugin Development"
    "07_robot_simulation:mobile_robot_demo:Robot Simulation"
    "08_advanced_features:actor_demo:Advanced Features"
)

# Print usage information
print_usage() {
    echo -e "${BLUE}Gazebo Tutorial - Run All Examples${NC}"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -m, --mode MODE          Execution mode: demo|build-only|interactive (default: demo)"
    echo "  -t, --timeout SECONDS    Timeout per example (default: 30)"
    echo "  -c, --continue-on-error  Continue running even if an example fails"
    echo "  -v, --verbose            Enable verbose output"
    echo "  -l, --list              List all available examples"
    echo "  -e, --example NUMBER     Run only specific example (1-based index)"
    echo "  -h, --help              Show this help message"
    echo ""
    echo "Modes:"
    echo "  demo         - Build and run each example briefly"
    echo "  build-only   - Only build examples, don't execute"
    echo "  interactive  - Run examples and wait for user input"
    echo ""
    echo "Examples:"
    echo "  $0                              # Run all demos"
    echo "  $0 --mode build-only           # Only build examples"
    echo "  $0 --mode interactive          # Interactive mode"
    echo "  $0 --example 1                 # Run only first example"
    echo "  $0 --continue-on-error         # Don't stop on failures"
}

# List all available examples
list_examples() {
    echo -e "${BLUE}Available Tutorial Examples:${NC}"
    echo ""
    
    local index=1
    for example in "${EXAMPLES[@]}"; do
        IFS=':' read -r dir target description <<< "$example"
        printf "%2d. %-25s %s\n" $index "$dir/$target" "$description"
        ((index++))
    done
    echo ""
}

# Print section header
print_section() {
    local title="$1"
    local width=80
    local padding=$(( (width - ${#title} - 2) / 2 ))
    
    echo ""
    echo -e "${PURPLE}$(printf '%*s' $width | tr ' ' '=')${NC}"
    echo -e "${PURPLE}$(printf '%*s' $padding)${CYAN} $title ${PURPLE}$(printf '%*s' $padding)${NC}"
    echo -e "${PURPLE}$(printf '%*s' $width | tr ' ' '=')${NC}"
    echo ""
}

# Run a single example
run_example() {
    local index="$1"
    local example_config="$2"
    
    IFS=':' read -r dir target description <<< "$example_config"
    local full_target="//examples/$dir:$target"
    
    print_section "Example $index: $description"
    
    echo -e "${BLUE}Directory:${NC} examples/$dir"
    echo -e "${BLUE}Target:${NC} $target"
    echo -e "${BLUE}Description:${NC} $description"
    echo ""
    
    # Build phase
    echo -e "${YELLOW}Building example...${NC}"
    if [ "$VERBOSE" = true ]; then
        bazel build "$full_target"
    else
        bazel build "$full_target" &>/dev/null
    fi
    
    local build_result=$?
    if [ $build_result -ne 0 ]; then
        echo -e "${RED}✗ Build failed for $full_target${NC}"
        if [ "$CONTINUE_ON_ERROR" = false ]; then
            exit 1
        fi
        return 1
    fi
    
    echo -e "${GREEN}✓ Build successful${NC}"
    
    # Execution phase (skip for build-only mode)
    if [ "$RUN_MODE" = "build-only" ]; then
        return 0
    fi
    
    echo -e "${YELLOW}Running example...${NC}"
    
    # Special handling for different example types
    case "$target" in
        *_validator|*validator*)
            # Validators need SDF files as arguments
            run_validator_example "$full_target"
            ;;
        *_viewer|*viewer*)
            # Viewers are interactive and need special handling
            run_viewer_example "$full_target"
            ;;
        *)
            # Regular demo applications
            run_demo_example "$full_target"
            ;;
    esac
    
    local run_result=$?
    if [ $run_result -eq 0 ]; then
        echo -e "${GREEN}✓ Example completed successfully${NC}"
    else
        echo -e "${RED}✗ Example failed or was interrupted${NC}"
        if [ "$CONTINUE_ON_ERROR" = false ]; then
            exit 1
        fi
    fi
    
    # Interactive mode - wait for user input
    if [ "$RUN_MODE" = "interactive" ]; then
        echo ""
        echo -e "${CYAN}Press Enter to continue to the next example, or 'q' to quit...${NC}"
        read -r response
        if [ "$response" = "q" ] || [ "$response" = "Q" ]; then
            echo -e "${YELLOW}Stopping at user request${NC}"
            exit 0
        fi
    fi
    
    return $run_result
}

# Run demo example with timeout
run_demo_example() {
    local target="$1"
    
    if [ "$RUN_MODE" = "interactive" ]; then
        # Interactive mode - let it run until user stops it
        timeout 300 bazel run "$target" || true
    else
        # Demo mode - run with timeout
        timeout "$TIMEOUT" bazel run "$target" || true
    fi
    
    return 0  # Don't fail on timeout in demo mode
}

# Run viewer example (interactive by nature)
run_viewer_example() {
    local target="$1"
    
    echo -e "${CYAN}Starting interactive viewer (will timeout after ${TIMEOUT}s in demo mode)...${NC}"
    
    if [ "$RUN_MODE" = "interactive" ]; then
        echo -e "${CYAN}Close the viewer window when you're done exploring${NC}"
        bazel run "$target" || true
    else
        # In demo mode, just start and let it timeout
        timeout "$TIMEOUT" bazel run "$target" &>/dev/null || true
        echo -e "${CYAN}Viewer demo completed (timed out after ${TIMEOUT}s)${NC}"
    fi
    
    return 0
}

# Run validator example with sample files
run_validator_example() {
    local target="$1"
    
    echo -e "${CYAN}Running validation on available SDF files...${NC}"
    
    # Find SDF files to validate
    local sdf_files=()
    if [ -d "$PROJECT_ROOT/worlds" ]; then
        while IFS= read -r -d '' file; do
            sdf_files+=("$file")
        done < <(find "$PROJECT_ROOT/worlds" -name "*.sdf" -print0)
    fi
    
    if [ ${#sdf_files[@]} -eq 0 ]; then
        echo -e "${YELLOW}No SDF files found, running validator without arguments${NC}"
        timeout "$TIMEOUT" bazel run "$target" || true
    else
        echo -e "${CYAN}Validating ${#sdf_files[@]} SDF files${NC}"
        timeout "$TIMEOUT" bazel run "$target" -- "${sdf_files[@]}" || true
    fi
    
    return 0
}

# Check prerequisites
check_prerequisites() {
    # Check if bazel is available
    if ! command -v bazel &> /dev/null; then
        echo -e "${RED}Error: Bazel is not installed or not in PATH${NC}"
        echo "Please install Bazel from https://bazel.build/"
        exit 1
    fi
    
    # Check if we're in a Bazel workspace
    if [ ! -f "$PROJECT_ROOT/MODULE.bazel" ]; then
        echo -e "${RED}Error: Not in a Bazel workspace (MODULE.bazel not found)${NC}"
        echo "Please run this script from the tutorial project root"
        exit 1
    fi
    
    echo -e "${GREEN}✓ Prerequisites check passed${NC}"
}

# Print summary statistics
print_summary() {
    local total="$1"
    local successful="$2"
    local failed="$3"
    
    echo ""
    print_section "Execution Summary"
    
    echo -e "${BLUE}Mode:${NC} $RUN_MODE"
    echo -e "${BLUE}Total examples:${NC} $total"
    echo -e "${GREEN}Successful:${NC} $successful"
    if [ $failed -gt 0 ]; then
        echo -e "${RED}Failed:${NC} $failed"
    else
        echo -e "${GREEN}Failed:${NC} $failed"
    fi
    
    local percentage=$((successful * 100 / total))
    echo -e "${BLUE}Success rate:${NC} ${percentage}%"
    
    if [ $failed -eq 0 ]; then
        echo -e "${GREEN}🎉 All examples completed successfully!${NC}"
    else
        echo -e "${YELLOW}⚠ Some examples failed - check the output above${NC}"
    fi
}

# Main execution function
main() {
    local run_single_example=""
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -m|--mode)
                RUN_MODE="$2"
                shift 2
                ;;
            -t|--timeout)
                TIMEOUT="$2"
                shift 2
                ;;
            -c|--continue-on-error)
                CONTINUE_ON_ERROR=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -l|--list)
                list_examples
                exit 0
                ;;
            -e|--example)
                run_single_example="$2"
                shift 2
                ;;
            -h|--help)
                print_usage
                exit 0
                ;;
            *)
                echo -e "${RED}Unknown option: $1${NC}"
                print_usage
                exit 1
                ;;
        esac
    done
    
    # Validate mode
    case "$RUN_MODE" in
        demo|build-only|interactive)
            ;;
        *)
            echo -e "${RED}Invalid mode: $RUN_MODE${NC}"
            echo "Valid modes: demo, build-only, interactive"
            exit 1
            ;;
    esac
    
    print_section "Gazebo Tutorial - All Examples Runner"
    
    echo -e "${BLUE}Configuration:${NC}"
    echo -e "  Mode: $RUN_MODE"
    echo -e "  Timeout: ${TIMEOUT}s per example"
    echo -e "  Continue on error: $CONTINUE_ON_ERROR"
    echo -e "  Verbose: $VERBOSE"
    echo ""
    
    # Check prerequisites
    check_prerequisites
    
    # Handle single example execution
    if [ -n "$run_single_example" ]; then
        if [ "$run_single_example" -lt 1 ] || [ "$run_single_example" -gt ${#EXAMPLES[@]} ]; then
            echo -e "${RED}Invalid example number: $run_single_example${NC}"
            echo "Valid range: 1-${#EXAMPLES[@]}"
            exit 1
        fi
        
        local index=$((run_single_example - 1))
        run_example "$run_single_example" "${EXAMPLES[$index]}"
        exit $?
    fi
    
    # Run all examples
    local total=${#EXAMPLES[@]}
    local successful=0
    local failed=0
    local start_time=$(date +%s)
    
    for i in "${!EXAMPLES[@]}"; do
        local example_num=$((i + 1))
        
        if run_example "$example_num" "${EXAMPLES[$i]}"; then
            ((successful++))
        else
            ((failed++))
        fi
        
        # Add separator between examples (except for the last one)
        if [ $i -lt $((total - 1)) ]; then
            echo ""
            echo -e "${CYAN}$(printf '%*s' 80 | tr ' ' '-')${NC}"
        fi
    done
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    print_summary "$total" "$successful" "$failed"
    
    echo ""
    echo -e "${BLUE}Total execution time:${NC} ${duration} seconds"
    
    # Exit with appropriate code
    if [ $failed -eq 0 ]; then
        exit 0
    else
        exit 1
    fi
}

# Handle script interruption
trap 'echo -e "\n${YELLOW}Script interrupted by user${NC}"; exit 130' INT TERM

# Run main function with all arguments
main "$@"