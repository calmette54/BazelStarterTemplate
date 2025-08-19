# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Environment Setup

This project uses [devenv](https://devenv.sh/) for the development environment. Enter the dev shell with:

```bash
devenv shell
```

This installs:
- `bazelisk` (Bazel wrapper)
- `commitizen` (for conventional commits)
- `claude-code` (this CLI tool)
- Git hooks for formatting, linting, and security checks

## Build System Commands

This project uses Bazel with the following key commands:

### Building
```bash
bazelisk build //...                    # Build everything
bazelisk build //lib/greeter           # Build greeter library
bazelisk build //standalone:main       # Build standalone executable
```

### Testing
```bash
bazelisk test //...                    # Run all tests
bazelisk test //test/greeter:greeter_test  # Run specific test
bazelisk test //benchmark:benchmark_test   # Run benchmark tests
bazelisk test //integration:integration_test # Run integration tests
```

### Code Quality
```bash
bazelisk run //:format                 # Format C++ code (clang-format)
bazelisk build //documentation:docs    # Generate documentation
```

### Development Utilities
```bash
bazelisk run //:refresh_all            # Refresh compile_commands.json for IDE
```

### Release Builds
```bash
bazelisk build //standalone:tar --config=x86_64 --revision_mode=RELEASE --release_version="1.0.0"
bazelisk run //standalone:push_image --revision_mode=RELEASE --release_version="1.0.0"
```

## Architecture Overview

### Project Structure
- `lib/greeter/` - Core greeter library (C++)
- `standalone/` - Standalone executable using the greeter library
- `test/` - Unit tests using GoogleTest and Doctest
- `benchmark/` - Performance benchmarks using Google Benchmark
- `integration/` - Python integration tests using testcontainers
- `documentation/` - Doxygen documentation generation
- `bzl/` - Custom Bazel rules and macros

### Key Technologies
- **Build System**: Bazel with bzlmod (MODULE.bazel)
- **Language**: C++23 with LLVM 19.1.6 toolchain
- **Testing**: GoogleTest, Doctest, Google Benchmark
- **Documentation**: Doxygen with custom theme
- **Formatting**: clang-format via rules_lint
- **CI/CD**: GitHub Actions with release-please
- **Containerization**: OCI images via rules_oci

### Build Configuration
- Cross-compilation support for x86_64 and aarch64
- Hermetic builds with custom sysroots
- Remote Build Execution (RBE) support via BuildBuddy
- Custom build flags for revision modes (DIRTY/WIP/RELEASE)

### Dependencies
External dependencies are managed through MODULE.bazel:
- `fmt` for string formatting
- `googletest` and `doctest` for testing
- `google_benchmark` for performance testing
- `protobuf` for serialization (if needed)

## Git Workflow

### Commits
Always use `cz commit` instead of `git commit` to ensure conventional commit format.

### Pre-commit Hooks
Automatically run on commit:
- C++ formatting via `bazelisk run //:format`
- Security checks (private keys, secrets)
- YAML/JSON validation
- Markdown linting

### Release Process
- Push to `main` triggers release-please workflow
- Automatic version bumping and CHANGELOG generation
- Cross-platform binary artifacts and container images
- Documentation deployment to GitHub Pages

## IDE Integration

Generate compile_commands.json for IDE support:
```bash
bazelisk run //:refresh_all
```

This enables proper IDE integration for clang-based tools and LSP servers.
