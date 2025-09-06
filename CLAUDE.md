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

This project uses Bazel with the following essential commands:

### Core Building & Testing
```bash
bazelisk build //...                    # Build everything
bazelisk test //...                     # Run all tests
bazelisk run //:format                  # Format C++ code (clang-format)
bazelisk run //:refresh_all             # Refresh compile_commands.json for IDE
```

### Important: Handling Build Timeouts
When running Bazel builds through Claude Code, builds may timeout during long operations. To prevent this:
- **DO NOT use the default timeout** - Always specify a longer timeout for build commands
- **For complex builds**: Use `timeout=600000` (10 minutes) parameter with the Bash tool
- **Example**: When building large targets or first-time builds that download dependencies
  ```bash
  # Use this approach for builds that may take time:
  Bash tool with timeout=600000: bazelisk build //examples/...
  ```
- **Monitor progress**: Bazel shows progress indicators - if you see activity, let it continue
- **Incremental builds**: After initial build, subsequent builds are much faster

### Python Development
```bash
bazelisk run //python:requirements.update            # Update Python dependencies
bazelisk run //python:gazelle_python_manifest.update # Update Python import mappings
bazelisk run //:gazelle                               # Generate/update BUILD files
bazelisk run //python/ai_example:main                 # Run main AI application
bazelisk test //python/...                           # Run Python tests
```

### Release & Documentation
```bash
bazelisk build //documentation:docs                                          # Generate docs
bazelisk build //standalone:tar --config=x86_64 --revision_mode=RELEASE     # Release build
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
- **Languages**: C++23 with LLVM 19.1.6 toolchain, Python 3.12
- **Testing**: GoogleTest, Doctest, Google Benchmark (C++), pytest (Python)
- **Documentation**: Doxygen with custom theme
- **Formatting**: clang-format via rules_lint
- **CI/CD**: GitHub Actions with release-please
- **Containerization**: OCI images via rules_oci
- **Python AI/ML**: PydanticAI, PyTorch
- **BUILD Generation**: Gazelle for automatic Python BUILD file generation

### Build Configuration
- Cross-compilation support for x86_64 and aarch64
- Hermetic builds with custom sysroots
- Remote Build Execution (RBE) support via BuildBuddy
- Custom build flags for revision modes (DIRTY/WIP/RELEASE)

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

## Python Development Workflow

This project includes comprehensive Python development support with hermetic builds, automatic dependency management, and modern AI/ML frameworks.

### Essential Workflow
```bash
# Adding dependencies: Edit python/requirements.in, then run:
bazelisk run //python:requirements.update
bazelisk run //python:gazelle_python_manifest.update
bazelisk run //:gazelle

# Development cycle:
bazelisk run //python/ai_example:main                 # Run application
bazelisk test //python/tests:test_ai_example          # Run tests
bazelisk test //python/tests:test_ai_example --test_filter="TestName"  # Specific tests
```

### Key Features
- **Hermetic Python Runtime**: Complete Python interpreter and packages bundled
- **AI/ML Integration**: PydanticAI for agent frameworks, PyTorch for neural networks
- **Automatic BUILD Generation**: Gazelle automatically maintains BUILD.bazel files
- **Type Safety**: Full Pydantic integration for structured data and validation
- **Comprehensive Testing**: pytest-based testing with fixtures and async support

## IDE Integration

Generate compile_commands.json for IDE support:
```bash
bazelisk run //:refresh_all
```

This enables proper IDE integration for clang-based tools and LSP servers.

## Claude Code MCP Tool Usage Guidelines

### Specialized Agent Strategy
**Use specialized agents for their expertise domains:**

- **bazel-build-expert**: 
  - All Bazel configuration, MODULE.bazel, build optimization
  - Python/Go bindings integration (pybind11, CGO)
  - Hermetic system library integration (nixpkgs patterns)
  - Cross-compilation, RBE setup, performance tuning
  - Complex dependency management and troubleshooting

- **python-integration-expert**: 
  - Python AI/ML workflows and dependency management
  - Gazelle configuration and BUILD file patterns  
  - pytest testing strategies and fixture patterns
  - PydanticAI agent development and PyTorch integration

- **documentation-architect**: Creating/improving documentation, API docs
- **sprint-planner**: Breaking down complex features into GitHub issues
- **general-purpose**: Multi-step research and complex searches
- **cicd-automation-architect**: GitHub Actions, CI/CD pipeline design

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

### When to Delegate to Specialized Agents

**Immediate delegation triggers:**
- **"How do I configure..."** → bazel-build-expert
- **"My build is slow..."** → bazel-build-expert  
- **"Setting up pybind11/CGO..."** → bazel-build-expert
- **"Python dependencies/AI workflow..."** → python-integration-expert
- **"Cross-compilation/hermetic builds..."** → bazel-build-expert
- **"Documentation generation..."** → documentation-architect

## Development Patterns

## Development Conventions

### TODO Annotations

We use a priority-based TODO annotation system throughout the codebase:

- `TODO(0)`: Critical - never merge
- `TODO(1)`: High - architectural flaws, major bugs
- `TODO(2)`: Medium - minor bugs, missing features
- `TODO(3)`: Low - polish, tests, documentation
- `TODO(4)`: Questions/investigations needed
- `PERF`: Performance optimization opportunities



### Essential Commands Reference
```bash
# Development cycle
devenv shell                           # Enter development environment
bazelisk build //...                   # Build everything
bazelisk test //...                    # Test everything
bazelisk run //:format                 # Format code

# Python development
bazelisk run //python:requirements.update            # Update Python deps
bazelisk run //python:gazelle_python_manifest.update # Update import mappings
bazelisk run //:gazelle                              # Update BUILD files

# Release and documentation
bazelisk build //documentation:docs                  # Generate documentation
bazelisk run //:refresh_all                         # Refresh IDE support
```

### Git and Release
```bash
cz commit                             # Conventional commits
git push origin main                  # Triggers release-please workflow
```

# important-instruction-reminders
Do what has been asked; nothing more, nothing less.
NEVER create files unless they're absolutely necessary for achieving your goal.
ALWAYS prefer editing an existing file to creating a new one.
NEVER proactively create documentation files (*.md) or README files. Only create documentation files if explicitly requested by the User.