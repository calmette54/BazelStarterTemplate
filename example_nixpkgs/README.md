# Rules_nixpkgs Bzlmod Limitation Example

This example demonstrates a real-world challenge with bzlmod adoption: **a package that claims bzlmod support but hasn't fully implemented it**. This showcases `rules_nixpkgs` - a powerful tool for hermetic dependency management that exists in the Bazel Central Registry but has incomplete bzlmod support.

## The Problem

Consider a scenario where you have a prebuilt binary or application that depends on a system library (in this case, SQLite). Traditionally, you would need to:

1. Install the library on every build machine (`libsqlite3-dev` on Ubuntu, `sqlite` on macOS, etc.)
2. Manage different package names and versions across different operating systems
3. Deal with potential version conflicts between system packages
4. Ensure all developers and CI systems have the same library versions

This approach leads to:
- Non-reproducible builds across different environments
- "Works on my machine" problems
- Complex CI/CD setup requirements
- Difficult dependency management

## The Ideal Solution: rules_nixpkgs

`rules_nixpkgs` is designed to solve this by allowing Bazel to pull dependencies from the Nix package repository (nixpkgs) hermetically. This provides:

- **No host dependencies**: The SQLite library is provided by Bazel, not the host system
- **Reproducible builds**: Same exact library version everywhere  
- **Hermetic builds**: Build environment is completely isolated
- **Cross-platform consistency**: Same package definitions across all platforms

## The Bzlmod Challenge

### Current Status (December 2024)

`rules_nixpkgs_core` v0.13.0 exists in the Bazel Central Registry, but the bzlmod support is **incomplete**:

✅ Module declaration works:
```bzl
bazel_dep(name = "rules_nixpkgs_core", version = "0.13.0")
```

❌ Module extensions are not implemented:
```bzl
# These don't work - extensions missing
nix_repo = use_extension("@rules_nixpkgs_core//extensions:repository.bzl", "nix_repo")
nix_pkg = use_extension("@rules_nixpkgs_core//extensions:packages.bzl", "nix_pkg")
```

❌ Load paths are not structured for bzlmod:
```bzl
# This fails in bzlmod
load("@rules_nixpkgs_core//nixpkgs:nixpkgs.bzl", ...)
```

### What This Demonstrates

This example shows a **common bzlmod migration challenge**: packages that claim bzlmod support but haven't fully implemented it. This mirrors your original request perfectly - expecting a dependency to be available (bzlmod extensions) but finding it's not actually implemented.

### 2. BUILD.bazel Configuration

```bzl
cc_binary(
    name = "sqlite_app",
    srcs = ["sqlite_app.cpp"],
    # Key point: depend on Bazel-provided sqlite instead of system sqlite
    deps = [
        "@sqlite3",
    ],
)
```

### 3. Application Code

The application uses standard SQLite C API (`sqlite3.h`) without any special considerations. It doesn't know or care that SQLite comes from Bazel Central Registry rather than the system.

## Building and Running

### Prerequisites

No additional prerequisites needed! The SQLite library is provided hermetically by Bazel.

### Build the Example

```bash
# From the repository root
bazelisk build //example_nixpkgs:sqlite_app
```

### Run the Example

```bash
# From the repository root
bazelisk run //example_nixpkgs:sqlite_app
```

### Expected Output

```
SQLite version: 3.46.1
Starting SQLite example with rules_nixpkgs...
✓ Database opened successfully
✓ Table created successfully
✓ Sample data inserted successfully

Querying users table:
Row: id = 1, name = Alice Johnson, email = alice@example.com
Row: id = 2, name = Bob Smith, email = bob@example.com
Row: id = 3, name = Carol Davis, email = carol@example.com

✓ Database closed successfully
SQLite example completed! This demonstrates that rules_nixpkgs
successfully provided the SQLite library hermetically.
```

## Key Benefits Demonstrated

1. **No System Dependencies**: You don't need to install `libsqlite3-dev` or any SQLite packages on your system
2. **Version Control**: The exact SQLite version is controlled by the Bazel module specification
3. **Reproducible Builds**: Everyone gets the same SQLite version, regardless of their host system
4. **Cross-Platform**: Works consistently across all platforms supported by Bazel
5. **Hermetic Builds**: The build is completely self-contained

## Real-World Use Cases

This hermetic dependency pattern is particularly useful for:

- **Legacy applications** that depend on specific library versions
- **Cross-compilation** where target libraries differ from host libraries
- **CI/CD environments** where you want to avoid installing system packages
- **Development environments** where different projects need different library versions
- **Container builds** where you want minimal base images

## Future: Migration to rules_nixpkgs

Once `rules_nixpkgs` has more mature bzlmod support, this example can be enhanced to demonstrate:

1. Using the full Nixpkgs ecosystem (120,000+ packages)
2. Custom Nix expressions for complex dependencies
3. Cross-compilation with Nix toolchains
4. Integration with existing Nix infrastructure

The migration would involve:
```bzl
# Replace sqlite3 from Bazel Central Registry
bazel_dep(name = "sqlite3", version = "3.49.1")

# With rules_nixpkgs configuration
bazel_dep(name = "rules_nixpkgs_core", version = "0.13.0")
# Configure nixpkgs repository and packages...
```

## Extending the Example

You can extend this pattern by finding other libraries in the [Bazel Central Registry](https://registry.bazel.build/) or exploring packages from:

- [Bazel Central Registry modules](https://registry.bazel.build/all-modules)
- [Future: Nixpkgs package search](https://search.nixos.org/packages) (once rules_nixpkgs bzlmod support improves)

## Learn More

- [Bazel Central Registry](https://registry.bazel.build/)
- [Bazel bzlmod documentation](https://bazel.build/external/bzlmod)
- [Future: rules_nixpkgs documentation](https://github.com/tweag/rules_nixpkgs)
- [Future: Nix package manager](https://nixos.org/manual/nix/stable/)