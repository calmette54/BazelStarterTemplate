# Hermetic Python Development with Bazel and Gazelle

A complete guide to building Python applications with Bazel using hermetic, deterministic builds and automatic BUILD file generation.

## What You'll Learn

By following this tutorial, you'll set up a production-ready Python development environment with:
- **Hermetic builds**: All dependencies bundled, no host system Python required
- **Automatic BUILD file generation**: Gazelle automatically creates and updates BUILD files
- **Deterministic builds**: Same results every time, across all environments
- **Modern Python frameworks**: Working example with PydanticAI and PyTorch

## Prerequisites

- **Bazel/Bazelisk** installed ([Installation Guide](https://bazel.build/install))
- **Basic Bazel knowledge**: Understanding of BUILD files and targets
- **Git repository** initialized in your project

## Quick Start

If you just want to see it working:

```bash
# 1. Generate requirements lock file
bazelisk run //python:requirements.update

# 2. Generate Gazelle Python manifest
bazelisk run //python:gazelle_python_manifest.update

# 3. Auto-generate all BUILD files
bazelisk run //:gazelle

# 4. Build and run the example
bazelisk run //python/ai_example:main
```

## Step-by-Step Guide

### Step 1: Configure Bazel Dependencies

Add these dependencies to your `MODULE.bazel` file:

```starlark
#--------------------- Python Toolchain --------------
bazel_dep(name = "rules_python", version = "1.6.0-rc0")

#--------------------- Go Toolchain for Gazelle --------------
bazel_dep(name = "rules_go", version = "0.53.0", repo_name = "io_bazel_rules_go")

#--------------------- Gazelle for BUILD file generation --------------
bazel_dep(name = "gazelle", version = "0.45.0", repo_name = "bazel_gazelle")

#--------------------- Python Gazelle plugin --------------
bazel_dep(name = "rules_python_gazelle_plugin", version = "1.6.0-rc0")

# Configure Go SDK for Gazelle (requires Go 1.23.0+)
go_sdk = use_extension("@io_bazel_rules_go//go:extensions.bzl", "go_sdk")
go_sdk.download(version = "1.23.1")
use_repo(go_sdk, "go_default_sdk")

# Configure Python toolchain
python = use_extension("@rules_python//python/extensions:python.bzl", "python")
python.toolchain(
    is_default = True,
    python_version = "3.12",
)

# Configure pip extension for dependency management
pip = use_extension("@rules_python//python/extensions:pip.bzl", "pip")
pip.parse(
    hub_name = "python_ai_deps",
    python_version = "3.12",
    requirements_lock = "//python:requirements_lock.txt",
)
use_repo(pip, "python_ai_deps")
```

### Step 2: Set Up Custom Gazelle Binary

Create the proper Gazelle configuration in your root `BUILD.bazel`:

```starlark
load("@bazel_gazelle//:def.bzl", "gazelle", "gazelle_binary")

# Custom Gazelle binary with Python language support
# This is essential for proper Python directive recognition
gazelle_binary(
    name = "gazelle_python_binary",
    languages = [
        "@rules_python_gazelle_plugin//python",
    ],
    visibility = ["//visibility:public"],
)

# Main Gazelle target for BUILD file generation
gazelle(
    name = "gazelle",
    data = [
        "//python:gazelle_python.yaml",
        "//python:requirements_lock.txt",
    ],
    gazelle = ":gazelle_python_binary",
)

# Gazelle Python manifest update
gazelle(
    name = "gazelle_python_manifest.update",
    data = ["@rules_python_gazelle_plugin//python:std_modules"],
    extra_args = [
        "-from_file=python/gazelle_python.yaml",
        "-to_macro=python/gazelle_python_manifest.update",
    ],
    gazelle = ":gazelle_python_binary",
)
```

### Step 3: Create Python Package Structure

Set up your Python package directory structure:

```bash
mkdir -p python/{ai_example/{agents,models},tests}
touch python/ai_example/{__init__.py,main.py}
touch python/ai_example/agents/{__init__.py,data_analyzer.py}
touch python/ai_example/models/{__init__.py,simple_mlp.py}
touch python/tests/{__init__.py,test_ai_example.py}
```

### Step 4: Define Your Dependencies

Create `python/requirements.in` with your Python dependencies:

```python
# Core AI frameworks
pydantic-ai-slim>=0.0.49    # AI agent framework (slim version avoids circular deps)
torch>=2.0.0               # Deep learning framework
numpy>=1.24.0              # Numerical computing
pydantic>=2.0.0            # Data validation

# Testing
pytest>=7.0.0
pytest-asyncio>=0.21.0
```

### Step 5: Configure Gazelle for Python

Create `python/BUILD.bazel` with Gazelle configuration:

```starlark
load("@python_ai_deps//:requirements.bzl", all_ai_requirements = "all_whl_requirements")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")
load("@rules_python_gazelle_plugin//manifest:defs.bzl", "gazelle_python_manifest")
load("@rules_python_gazelle_plugin//modules_mapping:def.bzl", "modules_mapping")

# Export files for Gazelle to access from root BUILD.bazel
exports_files([
    "gazelle_python.yaml",
    "requirements_lock.txt",
])

# Gazelle configuration directives
# gazelle:python_root
# gazelle:python_generation_mode package
# gazelle:python_default_visibility //:__subpackages__
# gazelle:python_label_convention $distribution_name$
# gazelle:python_label_normalization none

# External dependencies
# gazelle:resolve py numpy @python_ai_deps//numpy:pkg
# gazelle:resolve py pydantic @python_ai_deps//pydantic:pkg
# gazelle:resolve py torch @python_ai_deps//torch:pkg
# gazelle:resolve py pytest @python_ai_deps//pytest:pkg

# Internal dependencies (adjust to your package structure)
# gazelle:resolve py python.ai_example.agents.data_analyzer //python/ai_example/agents:agents
# gazelle:resolve py python.ai_example.models.simple_mlp //python/ai_example/models:models
# gazelle:resolve py python.ai_example //python/ai_example:ai_example

# Generate requirements lock file hermetically
compile_pip_requirements(
    name = "requirements",
    src = "requirements.in",
    extra_args = [
        "--allow-unsafe",
        "--resolver=backtracking",
    ],
    requirements_txt = "requirements_lock.txt",
)

# Generate modules mapping for Gazelle
modules_mapping(
    name = "modules_map",
    wheels = all_ai_requirements,
)

# Generate the Python manifest for Gazelle
gazelle_python_manifest(
    name = "gazelle_python_manifest",
    modules_mapping = ":modules_map",
    pip_repository_name = "python_ai_deps",
    requirements = ":requirements_lock.txt",
)
```

### Step 6: Generate Lock File and Manifest

Run these commands to set up the hermetic dependency system:

```bash
# Generate hermetic requirements lock file with SHA256 hashes
bazelisk run //python:requirements.update

# Generate Python manifest for Gazelle
bazelisk run //python:gazelle_python_manifest.update
```

This creates:
- `python/requirements_lock.txt`: Hermetic dependency list with cryptographic hashes
- `python/gazelle_python.yaml`: Import-to-package mapping for Gazelle

### Step 7: Write Your Python Code

Here's a complete example using PydanticAI and PyTorch:

**`python/ai_example/main.py`:**
```python
#!/usr/bin/env python3
"""
AI Example Application - Hermetic Python with Bazel
Demonstrates PydanticAI agent + PyTorch model integration
"""

import sys
import time
from python.ai_example.agents.data_analyzer import analyze_dataset_sync, DataSummary
from python.ai_example.models.simple_mlp import SimpleMLP, create_synthetic_dataset

def main():
    print("AI Example Application - Hermetic Python with Bazel")
    print("=" * 55)
    
    start_time = time.time()
    
    # Create synthetic dataset
    print("Running PydanticAI agent analysis...")
    data = create_synthetic_dataset(20, "sample_metrics")
    
    # Analyze with PydanticAI
    summary: DataSummary = analyze_dataset_sync(data, "sample_metrics")
    
    # Train PyTorch model
    print("Running PyTorch model processing...")
    model = SimpleMLP(input_size=1, hidden_size=32, output_size=1)
    train_loss = model.train_model(data, epochs=20, print_every=10)
    test_loss = model.evaluate(data)
    predictions = model.predict(data[:3])
    
    # Results
    processing_time = time.time() - start_time
    
    print(f"\n🔍 Workflow Results:")
    print(f"Status: SUCCESS")
    print(f"Processing Time: {processing_time:.2f}s")
    print(f"\n📊 Input Summary:")
    print(f"  sample_count: {len(data)}")
    print(f"  label: sample_metrics")
    print(f"  train_model: True")
    print(f"  prediction_mode: True")
    print(f"  data_range: [{min(data):.1f}, {max(data):.1f}]")
    print(f"\n🤖 AI Agent Analysis:")
    print(f"  Mean: {summary.mean:.3f}")
    print(f"  Std Dev: {summary.std_dev:.3f}")
    print(f"  Range: [{summary.min_val:.3f}, {summary.max_val:.3f}]")
    print(f"  Sample Count: {summary.sample_count}")
    print(f"  Analysis: {summary.analysis}")
    print(f"\n🧠 PyTorch Model Results:")
    print(f"  Model Trained: True")
    print(f"  Final Training Loss: {train_loss:.6f}")
    print(f"  Total Parameters: {sum(p.numel() for p in model.parameters())}")
    print(f"  Test Loss: {test_loss:.6f}")
    print(f"  Sample Predictions: {predictions}")
    
    print(f"\n✅ Workflow completed successfully!")
    return 0

if __name__ == "__main__":
    sys.exit(main())
```

### Step 8: Generate BUILD Files Automatically

Now run Gazelle to automatically generate all BUILD files:

```bash
bazelisk run //:gazelle
```

Gazelle will create BUILD files like:

**`python/ai_example/BUILD.bazel`:**
```starlark
load("@rules_python//python:defs.bzl", "py_binary", "py_library")

py_binary(
    name = "main",
    srcs = ["main.py"],
    imports = [".."],
    visibility = ["//:__subpackages__"],
    deps = [
        "//python/ai_example/agents",
        "//python/ai_example/models",
        "@python_ai_deps//numpy:pkg",
        "@python_ai_deps//pydantic:pkg",
        "@python_ai_deps//torch:pkg",
    ],
)

py_library(
    name = "ai_example",
    srcs = ["__init__.py", "main.py"],
    imports = [".."],
    visibility = ["//:__subpackages__"],
    deps = [
        "//python/ai_example/agents",
        "//python/ai_example/models",
        "@python_ai_deps//numpy:pkg",
        "@python_ai_deps//pydantic:pkg", 
        "@python_ai_deps//torch:pkg",
    ],
)
```

### Step 9: Build and Run

Test your hermetic Python application:

```bash
# Build the application
bazelisk build //python/ai_example:main

# Run the application
bazelisk run //python/ai_example:main

# Run tests
bazelisk test //python/tests:test_ai_example
```

## Development Workflow

### Adding New Dependencies

1. Add to `python/requirements.in`
2. Update lock file: `bazelisk run //python:requirements.update`
3. Update manifest: `bazelisk run //python:gazelle_python_manifest.update`
4. Regenerate BUILD files: `bazelisk run //:gazelle`

### Adding New Python Files

Just create the Python files - Gazelle automatically updates BUILD files:

```bash
# Create new Python module
touch python/ai_example/new_module.py

# Gazelle automatically updates BUILD files
bazelisk run //:gazelle
```

### Adding Internal Imports

If you add imports between your own modules, add resolve directives to `python/BUILD.bazel`:

```starlark
# gazelle:resolve py python.your_package.module //python/your_package:module
```

## Key Benefits

### Hermeticity
- **No host Python required**: Bazel downloads and manages Python 3.12
- **All dependencies bundled**: No pip install needed on build machines
- **SHA256 verification**: All packages cryptographically verified

### Reproducibility  
- **Lock files**: Exact versions pinned with `requirements_lock.txt`
- **Cross-platform**: Same build results on Linux, macOS, Windows
- **Deterministic**: Identical builds from same source code

### Automation
- **BUILD file generation**: Gazelle creates and maintains BUILD files
- **Dependency resolution**: Automatic detection of imports
- **Test discovery**: Automatic py_test target generation

## Troubleshooting

### Common Issues

**1. Import errors in Python code**
- Make sure you use full import paths: `from python.your_package.module import ...`
- Add resolve directives for internal imports in `python/BUILD.bazel`

**2. Dependencies not found**
- Verify package names with: `bazelisk query '@python_ai_deps//...' --output=package`
- Use underscores, not hyphens: `pydantic_ai_slim`, not `pydantic-ai-slim`

**3. Gazelle not updating BUILD files**
- Ensure you're using the custom `gazelle_python_binary`
- Check that `gazelle_python.yaml` exists and is up to date

### Useful Commands

```bash
# Query available Python packages
bazelisk query '@python_ai_deps//...' --output=package

# Build all Python targets
bazelisk build //python/...

# Run specific tests
bazelisk test //python/tests:test_ai_example

# Clean build cache
bazelisk clean

# View dependency graph
bazelisk query --output=graph //python/ai_example:main | dot -Tpng > deps.png
```

## Best Practices

### Package Structure
- Follow Python package conventions with `__init__.py` files
- Use descriptive module and package names
- Group related functionality in separate packages

### Dependencies
- Pin major versions in `requirements.in`
- Use slim versions of packages when available (e.g., `pydantic-ai-slim`)
- Regularly update dependencies with security patches

### Testing
- Create comprehensive test suites for all modules
- Use pytest fixtures for consistent test setup
- Test both unit and integration scenarios

### BUILD Files
- Let Gazelle manage BUILD files - avoid manual editing
- Use resolve directives for complex import patterns
- Set appropriate visibility for your targets

## Conclusion

You now have a production-ready Python development environment with:
- ✅ Hermetic builds independent of host system
- ✅ Automatic BUILD file generation and maintenance
- ✅ Deterministic, reproducible builds
- ✅ Modern AI framework integration (PydanticAI, PyTorch)
- ✅ Comprehensive testing infrastructure

This setup scales from small Python scripts to large ML applications while maintaining build reliability and developer productivity.