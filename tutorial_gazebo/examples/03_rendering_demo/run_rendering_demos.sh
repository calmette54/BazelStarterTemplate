#!/bin/bash

#
# Gazebo Rendering Demo Runner
#
# This script runs all the rendering demonstration examples in sequence,
# showcasing different aspects of Gazebo's rendering capabilities.
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
RUN_MODE="demo"        # demo, build-only, interactive
TIMEOUT=60             # Seconds per demo (longer for rendering demos)
CONTINUE_ON_ERROR=false
VERBOSE=false

# Rendering demo configurations
declare -a DEMOS=(
    "material_showcase:Material and PBR Showcase:Demonstrates various material properties and PBR rendering"
    "lighting_demo:Lighting and Shadow Demo:Shows different light types and their effects on materials"
    "camera_demo:Camera and Sensor Demo:Multiple camera types and rendering techniques"
    "pbr_explorer:PBR Parameter Explorer:Interactive tool for exploring PBR parameters"
    "visual_effects:Visual Effects Demo:Advanced rendering features and post-processing effects"
)

# Print usage information
print_usage() {
    echo -e "${BLUE}Gazebo Rendering Demo Runner${NC}"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -m, --mode MODE          Execution mode: demo|build-only|interactive (default: demo)"
    echo "  -t, --timeout SECONDS    Timeout per demo (default: 60)"
    echo "  -c, --continue-on-error  Continue running even if a demo fails"
    echo "  -v, --verbose            Enable verbose output"
    echo "  -l, --list              List all available demos"
    echo "  -d, --demo NUMBER        Run only specific demo (1-based index)"
    echo "  -h, --help              Show this help message"
    echo ""
    echo "Modes:"
    echo "  demo         - Build and run each demo briefly"
    echo "  build-only   - Only build demos, don't execute"
    echo "  interactive  - Run demos and wait for user input"
    echo ""
    echo "Examples:"
    echo "  $0                              # Run all rendering demos"
    echo "  $0 --mode build-only           # Only build demos"
    echo "  $0 --mode interactive          # Interactive mode"
    echo "  $0 --demo 1                    # Run only material showcase"
    echo "  $0 --timeout 90                # Longer timeout for each demo"
}

# List all available demos
list_demos() {
    echo -e "${BLUE}Available Rendering Demos:${NC}"
    echo ""
    
    local index=1
    for demo in "${DEMOS[@]}"; do
        IFS=':' read -r target title description <<< "$demo"
        printf "%2d. %-30s %s\\n" $index "$title" "$description"
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

# Run a single demo
run_demo() {
    local index="$1"
    local demo_config="$2"
    
    IFS=':' read -r target title description <<< "$demo_config"
    local full_target="//examples/03_rendering_demo:$target"
    
    print_section "Demo $index: $title"
    
    echo -e "${BLUE}Target:${NC} $target"
    echo -e "${BLUE}Title:${NC} $title"
    echo -e "${BLUE}Description:${NC} $description"
    echo ""
    
    # Build phase
    echo -e "${YELLOW}Building rendering demo...${NC}"
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
    
    echo -e "${YELLOW}Running rendering demo...${NC}"
    echo -e "${CYAN}This demo will showcase: $description${NC}"
    echo ""
    
    # Run the demo with appropriate timeout and handling
    local run_result=0
    
    if [ "$RUN_MODE" = "interactive" ]; then
        echo -e "${CYAN}Interactive mode - demo will run until you stop it${NC}"
        echo -e "${CYAN}Press Ctrl+C to stop the demo and continue to next one${NC}"
        timeout 300 bazel run "$full_target" || run_result=$?
    else
        echo -e "${CYAN}Demo mode - running for ${TIMEOUT} seconds${NC}"
        timeout "$TIMEOUT" bazel run "$full_target" || run_result=$?
    fi
    
    # Handle timeout gracefully (timeout returns 124)
    if [ $run_result -eq 124 ]; then
        echo -e "${GREEN}✓ Demo completed (timed out as expected)${NC}"
        run_result=0
    elif [ $run_result -eq 0 ]; then
        echo -e "${GREEN}✓ Demo completed successfully${NC}"
    else
        echo -e "${RED}✗ Demo failed or was interrupted${NC}"
        if [ "$CONTINUE_ON_ERROR" = false ]; then
            exit 1
        fi
    fi
    
    # Interactive mode - wait for user input
    if [ "$RUN_MODE" = "interactive" ]; then
        echo ""
        echo -e "${CYAN}Press Enter to continue to the next demo, or 'q' to quit...${NC}"
        read -r response
        if [ "$response" = "q" ] || [ "$response" = "Q" ]; then
            echo -e "${YELLOW}Stopping at user request${NC}"
            exit 0
        fi
    fi
    
    return $run_result
}

# Check prerequisites
check_prerequisites() {
    # Check if bazel is available
    if ! command -v bazel &> /dev/null; then
        echo -e "${RED}Error: Bazel is not installed or not in PATH${NC}"
        echo "Please install Bazel from https://bazel.build/"
        exit 1
    fi
    
    # Check if we're in the correct directory
    if [ ! -f "$PROJECT_ROOT/MODULE.bazel" ]; then
        echo -e "${RED}Error: Not in a Bazel workspace (MODULE.bazel not found)${NC}"
        echo "Please run this script from the tutorial project"
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
    print_section "Rendering Demo Summary"
    
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
        echo -e "${GREEN}🎉 All rendering demos completed successfully!${NC}"
        echo ""
        echo -e "${CYAN}What you've learned:${NC}"
        echo -e "${CYAN}• Material properties and PBR workflows${NC}"
        echo -e "${CYAN}• Lighting types and their effects${NC}"
        echo -e "${CYAN}• Camera systems and sensor rendering${NC}"
        echo -e "${CYAN}• Advanced visual effects and optimization${NC}"
    else
        echo -e "${YELLOW}⚠ Some demos failed - check the output above${NC}"
    fi
}

# Main execution function
main() {
    local run_single_demo=""
    
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
            -d|--demo)
                run_single_demo="$2"
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
    
    print_section "Gazebo Rendering Demo Runner"
    
    echo -e "${BLUE}Configuration:${NC}"
    echo -e "  Mode: $RUN_MODE"
    echo -e "  Timeout: ${TIMEOUT}s per demo"
    echo -e "  Continue on error: $CONTINUE_ON_ERROR"
    echo -e "  Verbose: $VERBOSE"
    echo ""
    
    echo -e "${CYAN}About Rendering Demos:${NC}"
    echo -e "${CYAN}These demos showcase Gazebo's advanced rendering capabilities including${NC}"
    echo -e "${CYAN}materials, lighting, cameras, and visual effects. Each demo focuses on${NC}"
    echo -e "${CYAN}specific rendering concepts with practical examples and explanations.${NC}"
    echo ""
    
    # Check prerequisites
    check_prerequisites
    
    # Handle single demo execution
    if [ -n "$run_single_demo" ]; then
        if [ "$run_single_demo" -lt 1 ] || [ "$run_single_demo" -gt ${#DEMOS[@]} ]; then
            echo -e "${RED}Invalid demo number: $run_single_demo${NC}"
            echo "Valid range: 1-${#DEMOS[@]}"
            exit 1
        fi
        
        local index=$((run_single_demo - 1))
        run_demo "$run_single_demo" "${DEMOS[$index]}"
        exit $?
    fi
    
    # Run all demos
    local total=${#DEMOS[@]}
    local successful=0
    local failed=0
    local start_time=$(date +%s)
    
    for i in "${!DEMOS[@]}"; do
        local demo_num=$((i + 1))
        
        if run_demo "$demo_num" "${DEMOS[$i]}"; then
            ((successful++))
        else
            ((failed++))
        fi
        
        # Add separator between demos (except for the last one)
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
    
    # Provide next steps
    echo ""
    echo -e "${CYAN}Next Steps:${NC}"
    echo -e "${CYAN}• Explore the source code for each demo to understand implementation details${NC}"
    echo -e "${CYAN}• Modify demo parameters to see how they affect visual output${NC}"
    echo -e "${CYAN}• Continue to Example 04: Sensors Demo for camera and LiDAR integration${NC}"
    echo -e "${CYAN}• Check the tutorial README for more advanced rendering topics${NC}"
    
    # Exit with appropriate code
    if [ $failed -eq 0 ]; then
        exit 0
    else
        exit 1
    fi
}

# Handle script interruption
trap 'echo -e "\n${YELLOW}Rendering demo runner interrupted by user${NC}"; exit 130' INT TERM

# Run main function with all arguments
main "$@"