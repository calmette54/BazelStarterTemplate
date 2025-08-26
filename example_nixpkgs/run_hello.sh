#!/bin/bash
# Wrapper script to run the hello_world binary with nixpkgs hello in PATH

# Add the nixpkgs hello binary to PATH
export PATH="external/hello/bin:$PATH"

# Run the main program
exec "$@"