
# BranchMorphoGen

BranchMorphoGen is an open source high performance C++ framework for simulating branching morphogenesis and generating realistic synthetic biological morphologies for mechanistic modeling, image analysis, and AI driven inverse problems.


A C++ framework for computational branching morphogenesis.

# BranchMorphoGen Installation Instructions
## Prerequisites
Before building BranchMorphoGen, make sure the following tools and libraries are installed:

- CMake version 3.16 or newer
- A C++ compiler supporting C++17 (e.g., g++, clang++)
- libtiff development library
- POSIX threads (usually available by default on Unix-based systems)


## 1. Install libTIFF

BranchMorphoGen depends on libtiff for image generation. You can install it using a system package manager or specify a custom installation path.

### Option A: System-wide Installation

For Ubuntu/Debian:
    sudo apt-get install libtiff-dev

For macOS (using Homebrew):
    brew install libtiff

### Option B: Custom Installation

If libtiff is installed in a non-standard directory, set the TIFF_ROOT environment variable:

    export TIFF_ROOT=/path/to/tiff/installation

Make sure the following paths exist:
- $TIFF_ROOT/include/tiff.h
- $TIFF_ROOT/lib/libtiff.so or libtiff.dylib or libtiff.a

---

## 2. Clone the BranchMorphoGen Repository

    git clone https://github.com/yourusername/BranchMorphoGen.git
    cd BranchMorphoGen

---

## 3. Build the Project

Create a build directory and compile the source code using CMake:

    mkdir build
    cd build
    cmake ..         # or: cmake -DTIFF_ROOT=/custom/tiff ..
    make -j2  # Use all available cores

This will create the `BranchMorphoGen` executable in the `build/` directory.

---

## 4. Run the Simulation

Run the executable with your parameter input file:

    ./BranchMorphoGen ../parameters.in

Make sure `parameters.in` exists and is formatted correctly.

---

## 5. Clean Rebuild (Optional)

If you need to rebuild from scratch:

    rm -rf build
    mkdir build
    cd build
    cmake ..
    make -j$(nproc)

---

## 6. Troubleshooting

### Problem: TIFF library not found

If you get the error:

    TIFF library not found. Set TIFF_ROOT to your local libtiff installation.

→ Ensure `TIFF_ROOT` is correctly set to the libtiff installation path.

### Problem: Build fails

Check the file:

    build/CMakeFiles/CMakeError.log

for detailed error messages.

---

## License

This project is open-source. Please see the `LICENSE` file in the root directory for full licensing terms.

