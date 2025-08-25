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

## Claude Code MCP Tool Usage Guidelines

### Documentation Research Priority
**ALWAYS use MCP tools before guessing or assuming:**

1. **Context7 Documentation Lookup** (for any library/framework questions):
   ```bash
   # Pattern: Question about library → Context7 lookup → Implement
   mcp__context7__resolve-library-id → mcp__context7__get-library-docs
   ```
   - Use for Bazel, C++, testing frameworks, build tools
   - Provides current documentation and real code examples
   - Available libraries include: bazelbuild/bazel, bazel-contrib/rules_go, etc.

2. **GitHub Code Search** (for implementation patterns):
   ```bash
   # Pattern: Need implementation example → Search real code → Adapt
   mcp__github__search_code → mcp__grep__searchGitHub
   ```
   - Find production usage patterns across millions of repositories
   - Search for literal code patterns, not keywords

### Agent Utilization Strategy
**Use specialized agents for their expertise domains:**

- **bazel-build-expert**: All Bazel configuration, MODULE.bazel, build optimization
- **documentation-architect**: Creating/improving documentation, API docs
- **sprint-planner**: Breaking down complex features into GitHub issues
- **general-purpose**: Multi-step research and complex searches
- **cicd-automation-architect**: GitHub Actions, CI/CD pipeline design

### Information Persistence
**Document findings in two places:**

1. **CLAUDE.md updates**: Add sections for discovered patterns, commands, configurations
2. **Memory files**: Use `mcp__serena__write_memory` for architectural insights, common patterns

### Workflow Pattern
```
Question/Task → MCP Documentation Lookup → Agent Consultation → Implementation → Document Learnings
```

**Example:** "How do I set up integration tests?"
1. `mcp__context7__get-library-docs` for testing patterns
2. `bazel-build-expert` for Bazel-specific setup
3. Implement based on documentation examples
4. Update CLAUDE.md with discovered commands/patterns
