# LibP2P Example with vcpkg

This example demonstrates how to build and run a simple LibP2P application using vcpkg as the package manager.

## Prerequisites

- CMake 3.15 or higher
- A C++20 compatible compiler
- Git
- vcpkg

## Getting Started

1. Clone vcpkg if you haven't already:

```bash
git clone https://github.com/microsoft/vcpkg.git

./vcpkg/bootstrap-vcpkg.sh # For Linux/macOS
.\vcpkg\bootstrap-vcpkg.bat # For Windows
```

2. Clone this repository:

```bash
git clone https://github.com/libp2p/libp2p.git
cd libp2p/example/00-vcpkg-install
```

3. Install LibP2P dependencies using vcpkg:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

4. Run the example:

```bash
./build/example
```

## Notes

- This example assumes you have a working vcpkg installation. If you encounter any issues, please refer to the [vcpkg documentation](https://vcpkg.io/en/getting-started.html) for troubleshooting.
- The example demonstrates how to build and run a simple LibP2P application using vcpkg as the package manager.
