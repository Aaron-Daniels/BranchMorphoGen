cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

# On macOS, prefer clang++ if user did not specify a compiler
if(APPLE AND NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER clang++ CACHE FILEPATH "C++ compiler" FORCE)
endif()

project(BranchMorphoGen CXX)

# On macOS, enforce Clang or AppleClang
if(APPLE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR
            "On macOS BranchMorphoGen must be built with Clang or AppleClang. "
            "Current compiler id: ${CMAKE_CXX_COMPILER_ID}"
        )
    endif()
endif()

# =======================
# C++ standard
# =======================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =======================
# Compiler warnings and optimization flags
# =======================

# Enable compile_commands.json for clangd and similar tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Global warnings
add_compile_options(-Wall -Wextra -Wno-unused-parameter)

# =========================================================
# 2. Enforce -O3 and other optimisers
# =========================================================

# If user did not set a build type, default to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Common high optimisation flags for Clang
#set(CLANG_OPT_FLAGS "-O3 -march=native -funroll-loops")
set(CLANG_OPT_FLAGS "-O3 -fno-math-errno -ffp-contract=fast")

# Apply to generic flags (all build types)
string(APPEND CMAKE_CXX_FLAGS " ${CLANG_OPT_FLAGS}")

# Make sure Release and RelWithDebInfo also have NDEBUG
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -DNDEBUG")
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -DNDEBUG")

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")

# =======================
# Project sources and includes
# =======================

set(PROJECT_INCLUDE_DIR        "${PROJECT_SOURCE_DIR}/include")
set(PROJECT_INCLUDE_BRANCH_DIR "${PROJECT_SOURCE_DIR}/include/Branch")

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

if(NOT SRC_FILES)
    message(FATAL_ERROR "No source files found in src/. Check your directory structure.")
endif()

message(STATUS "Source files:")
foreach(f ${SRC_FILES})
    message(STATUS "  ${f}")
endforeach()

# =======================
# Bundled Eigen (header only)
# =======================

set(EIGEN3_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/third_party/eigen")

if(NOT EXISTS "${EIGEN3_INCLUDE_DIR}/Eigen/Core")
    message(FATAL_ERROR
        "Bundled Eigen not found at ${EIGEN3_INCLUDE_DIR}. "
        "Please place the Eigen headers so that "
        "${EIGEN3_INCLUDE_DIR}/Eigen/Core exists."
    )
endif()

message(STATUS "Using bundled Eigen from: ${EIGEN3_INCLUDE_DIR}")

# =======================
# Pthreads
# =======================

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# =======================
# Executable target
# =======================

add_executable(BranchMorphoGen ${SRC_FILES})

target_include_directories(BranchMorphoGen
    PRIVATE
        ${PROJECT_INCLUDE_DIR}
        ${PROJECT_INCLUDE_BRANCH_DIR}
        ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(BranchMorphoGen PRIVATE Threads::Threads)

# =======================
# TIFF support (custom or system)
# =======================

if(NOT DEFINED TIFF_ROOT AND DEFINED ENV{TIFF_ROOT})
    set(TIFF_ROOT $ENV{TIFF_ROOT})
endif()

if(TIFF_ROOT)
    message(STATUS "Looking for libtiff in custom path: ${TIFF_ROOT}")
    set(CMAKE_PREFIX_PATH "${TIFF_ROOT};${CMAKE_PREFIX_PATH}")
    set(TIFF_INCLUDE_DIR "${TIFF_ROOT}/include")
    set(TIFF_LIBRARY_DIR "${TIFF_ROOT}/lib")
endif()

find_package(TIFF QUIET)

if(TIFF_FOUND)
    message(STATUS "Found TIFF library include path: ${TIFF_INCLUDE_DIR}")
    if(TIFF_INCLUDE_DIR)
        target_include_directories(BranchMorphoGen PRIVATE ${TIFF_INCLUDE_DIR})
    endif()
    if(TIFF_LIBRARY_DIR)
        target_link_directories(BranchMorphoGen PRIVATE ${TIFF_LIBRARY_DIR})
    endif()
    target_link_libraries(BranchMorphoGen PRIVATE ${TIFF_LIBRARIES})
else()
    message(FATAL_ERROR "TIFF library not found. Set TIFF_ROOT to your local libtiff installation.")
endif()

# =======================
# Install target
# =======================

install(TARGETS BranchMorphoGen DESTINATION bin)

install(FILES "${PROJECT_SOURCE_DIR}/parameters.in"
        DESTINATION share/BranchMorphoGen)
# =======================


# End of CMakeLists.txt