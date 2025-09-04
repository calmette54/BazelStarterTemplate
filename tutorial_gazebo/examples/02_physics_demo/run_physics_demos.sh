#!/bin/bash

#
# Physics Demo Runner Script
#
# This script runs all physics demonstration examples in sequence,
# providing an automated way to explore different physics concepts
# and engine behaviors.
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
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

# Default settings
RUN_MODE="demo"        # demo, individual, interactive
TIMEOUT=30             # Seconds per demo
CONTINUE_ON_ERROR=false
VERBOSE=false

# Physics demo configurations
declare -a PHYSICS_DEMOS=(
    "physics_comparison:Physics Engine Comparison:Compare DART, Bullet, and TPE engines"
    "collision_demo:Collision Detection Demo:Various collision shapes and behaviors"
    "joint_demo:Joint Mechanisms Demo:Different joint types and constraints"
    "physics_benchmark:Performance Benchmark:Comprehensive physics performance analysis"
    "physics_configurator:Configuration Tool:Physics parameter tuning and optimization"
)

# Print usage information
print_usage() {
    echo -e "${BLUE}Gazebo Physics Demo Runner${NC}"
    echo ""
    echo "Usage: $0 [OPTIONS] [DEMO_NAME]"
    echo ""
    echo "Options:"
    echo "  -m, --mode MODE          Execution mode: demo|individual|interactive (default: demo)"
    echo "  -t, --timeout SECONDS    Timeout per demo (default: 30)"
    echo "  -c, --continue-on-error  Continue running even if a demo fails"
    echo "  -v, --verbose            Enable verbose output"
    echo "  -l, --list              List all available physics demos"
    echo "  -h, --help              Show this help message"
    echo ""
    echo "Modes:"
    echo "  demo         - Run all demos in sequence with brief execution"
    echo "  individual   - Run specific demo by name"
    echo "  interactive  - Run demos interactively with user prompts"
    echo ""
    echo "Available Demos:"
    echo "  physics_comparison  - Compare physics engine performance"
    echo "  collision_demo      - Collision detection and response"
    echo "  joint_demo         - Joint mechanisms and constraints"
    echo "  physics_benchmark  - Comprehensive performance analysis"
    echo "  physics_configurator - Interactive parameter tuning"
    echo ""
    echo "Examples:"
    echo "  $0                                # Run all demos"
    echo "  $0 --mode interactive            # Interactive mode"
    echo "  $0 physics_comparison            # Run specific demo"
    echo "  $0 --timeout 60 --verbose       # Extended timeout, verbose"
}

# List all available physics demos
list_demos() {
    echo -e "${BLUE}Available Physics Demonstration Examples:${NC}"
    echo ""
    
    local index=1
    for demo in "${PHYSICS_DEMOS[@]}"; do
        IFS=':' read -r target title description <<< "$demo"
        printf "%2d. %-25s %s\n   %s\n" $index "$target" "$title" "$description"
        echo ""
        ((index++))
    done
}

# Print section header
print_section() {
    local title="$1"
    local width=70
    local padding=$(( (width - ${#title} - 2) / 2 ))
    
    echo ""
    echo -e "${PURPLE}$(printf '%*s' $width | tr ' ' '=')${NC}"
    echo -e "${PURPLE}$(printf '%*s' $padding)${CYAN} $title ${PURPLE}$(printf '%*s' $padding)${NC}"
    echo -e "${PURPLE}$(printf '%*s' $width | tr ' ' '=')${NC}"
    echo ""
}

# Run a single physics demo
run_physics_demo() {
    local demo_target="$1"
    local demo_title="$2"
    local demo_description="$3"
    
    print_section "$demo_title"
    
    echo -e "${BLUE}Demo:${NC} $demo_target"
    echo -e "${BLUE}Title:${NC} $demo_title"
    echo -e "${BLUE}Description:${NC} $demo_description"
    echo ""
    
    local full_target="//examples/02_physics_demo:$demo_target"
    
    # Build phase
    echo -e "${YELLOW}Building physics demo...${NC}"
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
    
    # Execution phase
    echo -e "${YELLOW}Running physics demo...${NC}"
    
    case "$demo_target" in
        physics_benchmark)
            # Benchmark takes longer to run
            run_benchmark_demo "$full_target"
            ;;
        physics_configurator)
            # Configuration tool is interactive
            run_interactive_demo "$full_target"
            ;;
        *)
            # Regular physics demos
            run_standard_demo "$full_target"
            ;;
    esac
    
    local run_result=$?
    if [ $run_result -eq 0 ]; then
        echo -e "${GREEN}✓ Physics demo completed successfully${NC}"
    else
        echo -e "${RED}✗ Physics demo failed or was interrupted${NC}"
        if [ "$CONTINUE_ON_ERROR" = false ]; then
            exit 1
        fi
    fi
    
    # Interactive mode - wait for user input
    if [ "$RUN_MODE" = "interactive" ]; then
        echo ""
        echo -e "${CYAN}Demo completed. Press Enter to continue, or 'q' to quit...${NC}"
        read -r response
        if [ "$response" = "q" ] || [ "$response" = "Q" ]; then
            echo -e "${YELLOW}Stopping at user request${NC}"
            exit 0
        fi
    fi
    
    return $run_result
}

# Run standard physics demo with timeout
run_standard_demo() {
    local target="$1"
    
    if [ "$RUN_MODE" = "interactive" ]; then
        echo -e "${CYAN}Running physics demo (press Ctrl+C when done)...${NC}"
        timeout 300 bazel run "$target" || true
    else
        echo -e "${CYAN}Running physics demo for ${TIMEOUT} seconds...${NC}"
        timeout "$TIMEOUT" bazel run "$target" || true
    fi
    
    return 0  # Don't fail on timeout in demo mode
}

# Run benchmark demo (takes longer)
run_benchmark_demo() {
    local target="$1"
    
    echo -e "${CYAN}Running comprehensive physics benchmark...${NC}"
    echo -e "${YELLOW}Note: This benchmark may take several minutes to complete${NC}"
    
    if [ "$RUN_MODE" = "interactive" ]; then
        bazel run "$target" || true
    else
        # Allow more time for benchmarks
        timeout $((TIMEOUT * 4)) bazel run "$target" || true
    fi
    
    return 0
}

# Run interactive demo
run_interactive_demo() {
    local target="$1"
    
    echo -e "${CYAN}Running interactive physics configuration tool...${NC}"
    
    if [ "$RUN_MODE" = "interactive" ]; then
        bazel run "$target" || true
    else
        # Shorter timeout for automated runs
        timeout "$TIMEOUT" bazel run "$target" || true
    fi
    
    return 0
}

# Find demo by target name
find_demo_by_target() {
    local target_name="$1"
    
    for demo in "${PHYSICS_DEMOS[@]}"; do
        IFS=':' read -r target title description <<< "$demo"
        if [ "$target" = "$target_name" ]; then
            echo "$demo"
            return 0
        fi
    done
    
    return 1
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

# Print demo introduction
print_introduction() {
    print_section "Gazebo Physics Demonstrations"
    
    echo -e "${BLUE}This suite demonstrates various aspects of physics simulation in Gazebo:${NC}"
    echo ""
    echo -e "${CYAN}• Physics Engine Comparison${NC} - Compare DART, Bullet, and TPE engines"
    echo -e "${CYAN}• Collision Detection Demo${NC} - Explore collision shapes and surface properties"
    echo -e "${CYAN}• Joint Mechanisms Demo${NC} - Understand different joint types and constraints"
    echo -e "${CYAN}• Performance Benchmark${NC} - Comprehensive physics performance analysis"
    echo -e "${CYAN}• Configuration Tool${NC} - Interactive physics parameter tuning"
    echo ""
    echo -e "${YELLOW}Each demo showcases key concepts and provides practical insights${NC}"
    echo -e "${YELLOW}into physics simulation with Gazebo Harmonic.${NC}"
    echo ""
}

# Print summary statistics
print_summary() {
    local total="$1"
    local successful="$2"
    local failed="$3"
    
    echo ""
    print_section "Physics Demo Execution Summary"
    
    echo -e "${BLUE}Mode:${NC} $RUN_MODE"
    echo -e "${BLUE}Total demos:${NC} $total"
    echo -e "${GREEN}Successful:${NC} $successful"
    if [ $failed -gt 0 ]; then
        echo -e "${RED}Failed:${NC} $failed"
    else
        echo -e "${GREEN}Failed:${NC} $failed"
    fi
    
    local percentage=$((successful * 100 / total))
    echo -e "${BLUE}Success rate:${NC} ${percentage}%"
    
    if [ $failed -eq 0 ]; then
        echo -e "${GREEN}🎉 All physics demos completed successfully!${NC}"
        echo -e "${CYAN}You now have a comprehensive understanding of Gazebo physics simulation.${NC}"
    else
        echo -e "${YELLOW}⚠ Some demos failed - check the output above${NC}"
    fi
}

# Main execution function
main() {
    local demo_name=""
    
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
                list_demos
                exit 0
                ;;
            -h|--help)
                print_usage
                exit 0
                ;;
            -*)
                echo -e "${RED}Unknown option: $1${NC}"
                print_usage
                exit 1
                ;;
            *)
                demo_name="$1"
                RUN_MODE="individual"
                shift
                ;;
        esac
    done
    
    # Validate mode
    case "$RUN_MODE" in
        demo|individual|interactive)
            ;;
        *)
            echo -e "${RED}Invalid mode: $RUN_MODE${NC}"
            echo "Valid modes: demo, individual, interactive"
            exit 1
            ;;
    esac
    
    print_introduction
    
    echo -e "${BLUE}Configuration:${NC}"
    echo -e "  Mode: $RUN_MODE"
    echo -e "  Timeout: ${TIMEOUT}s per demo"
    echo -e "  Continue on error: $CONTINUE_ON_ERROR"
    echo -e "  Verbose: $VERBOSE"
    echo ""
    
    # Check prerequisites
    check_prerequisites
    
    # Handle individual demo execution
    if [ "$RUN_MODE" = "individual" ] && [ -n "$demo_name" ]; then
        demo_info=$(find_demo_by_target "$demo_name")
        if [ $? -ne 0 ]; then
            echo -e "${RED}Demo not found: $demo_name${NC}"
            echo "Available demos:"
            list_demos
            exit 1
        fi
        
        IFS=':' read -r target title description <<< "$demo_info"
        run_physics_demo "$target" "$title" "$description"
        exit $?
    fi
    
    # Run all physics demos
    local total=${#PHYSICS_DEMOS[@]}
    local successful=0
    local failed=0
    local start_time=$(date +%s)
    
    for i in "${!PHYSICS_DEMOS[@]}"; do
        IFS=':' read -r target title description <<< "${PHYSICS_DEMOS[$i]}"
        
        if run_physics_demo "$target" "$title" "$description"; then
            ((successful++))
        else
            ((failed++))
        fi
        
        # Add separator between demos (except for the last one)
        if [ $i -lt $((total - 1)) ]; then
            echo ""
            echo -e "${CYAN}$(printf '%*s' 70 | tr ' ' '-')${NC}"
        fi
    done
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    print_summary "$total" "$successful" "$failed"
    
    echo ""
    echo -e "${BLUE}Total execution time:${NC} ${duration} seconds"
    echo ""
    echo -e "${CYAN}Physics simulation concepts covered:${NC}"
    echo -e "• Physics engine architecture and comparison"
    echo -e "• Collision detection and surface properties"
    echo -e "• Joint mechanisms and mechanical constraints"
    echo -e "• Performance optimization and benchmarking"
    echo -e "• Parameter tuning and configuration"
    echo ""
    echo -e "${GREEN}Continue to Example 03: Rendering Demo to learn about visual simulation!${NC}"
    
    # Exit with appropriate code
    if [ $failed -eq 0 ]; then
        exit 0
    else
        exit 1
    fi
}

# Handle script interruption
trap 'echo -e "\n${YELLOW}Physics demo runner interrupted by user${NC}"; exit 130' INT TERM

# Run main function with all arguments
main "$@"