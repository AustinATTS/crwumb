# Building UwU-C with Bazel

This project uses [Bazel](https://bazel.build/) as its build system. Bazel provides fast, reproducible builds with excellent caching and incremental compilation.

## Prerequisites

Install Bazel (or Bazelisk, which automatically uses the correct Bazel version):

**macOS:**
```bash
brew install bazelisk
```

**Linux:**
```bash
npm install -g @bazel/bazelisk
```

Or download from [bazel.build/install](https://bazel.build/install)

## Basic Commands

### Build the Compiler

Build the UwUC compiler in release mode (optimized):
```bash
bazel build //:uwucc
```

The compiled binary will be available at `bazel-bin/uwucc`.

### Build in Debug Mode

Build with debug symbols and address sanitizer:
```bash
bazel build --config=debug //:uwucc
```

### Run the Compiler

Run the compiler directly through Bazel:
```bash
bazel run //:uwucc -- input.uwu -o output
```

Or use the built binary:
```bash
./bazel-bin/uwucc input.uwu -o output
```

## Compiling Examples

The build system automatically creates targets for all `.uwu` files in the `example/` directory.

### Build an Example

For a file `example/ez.uwu`:
```bash
bazel build //:example_ez
```

The compiled binary will be at `bazel-bin/ez`.

### Run an Example

```bash
bazel run //:example_ez
```

### List All Available Targets

See all example targets and other build targets:
```bash
bazel query //...
```

## Build Configurations

### Release Build (default)
Optimized with `-O2` and `-march=native`:
```bash
bazel build //:uwucc
# or explicitly:
bazel build --config=release //:uwucc
```

### Debug Build
With debug symbols, no optimization, and AddressSanitizer:
```bash
bazel build --config=debug //:uwucc
```

## Platform Support

The build system automatically detects your platform and architecture:

- **Platforms:** macOS, Linux
- **Architectures:** x86_64, ARM64 (aarch64)

Platform-specific flags are applied automatically:
- macOS: `-DUWUCC_PLATFORM_MACOS`
- Linux: `-DUWUCC_PLATFORM_LINUX`, `-no-pie` linker flag
- ARM64: `-DUWUCC_ARCH_ARM64`
- x86_64: `-DUWUCC_ARCH_X86_64`

## Cleaning Build Artifacts

Remove all build outputs:
```bash
bazel clean
```

Deep clean (removes all cached data):
```bash
bazel clean --expunge
```

## Getting Build Information

View build configuration and paths:
```bash
bazel info
```

## Advantages Over Make

- **Incremental builds:** Only rebuilds what changed
- **Better caching:** Shares build cache across projects
- **Reproducible:** Same inputs always produce same outputs
- **Parallel by default:** Uses all CPU cores efficiently
- **Cross-platform:** Consistent behavior on all platforms

## Configuration Files

- **MODULE.bazel** - Declares dependencies (platforms, bazel_skylib)
- **BUILD.bazel** - Defines build targets and rules
- **.bazelrc** - Build configuration options and flags

## Common Issues

**"No such target"**: Make sure you're in the project root directory.

**"Platform not found"**: Update the `platforms` version in `MODULE.bazel`.

**Slow first build**: Bazel downloads dependencies and builds everything on first run. Subsequent builds are much faster.

## Additional Resources

- [Bazel Documentation](https://bazel.build/docs)
- [Bazel C/C++ Tutorial](https://bazel.build/tutorials/cpp)
- [Bazel Command-Line Reference](https://bazel.build/reference/command-line-reference)