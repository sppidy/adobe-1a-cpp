cmake_minimum_required(VERSION 3.16)
project(PDFProcessor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags for maximum speed
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -DNDEBUG -ffast-math -funroll-loops")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(PkgConfig REQUIRED)

# Use system LibTorch (installed via pacman)
unset(CMAKE_PREFIX_PATH)  # Clear pip paths
find_package(Torch QUIET PATHS /usr /usr/local NO_DEFAULT_PATH)

# OpenCV for image processing
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs dnn)

# LibTorch for PyTorch model loading (.pt files) - CPU only
if(Torch_FOUND)
    message(STATUS "Found system LibTorch (CPU-only): ${TORCH_LIBRARIES}")
    set(USE_LIBTORCH ON)
    add_definitions(-DUSE_LIBTORCH)
    # Force CPU-only execution
    add_definitions(-DTORCH_CPU_ONLY)
else()
    message(STATUS "System LibTorch not found - YOLOv8 .pt support disabled")
    message(STATUS "Install with: sudo pacman -S python-pytorch")
    set(USE_LIBTORCH OFF)
endif()

# MuPDF for PDF processing - Force shared library usage
# First remove static libraries to force shared library linking
find_library(MUPDF_SHARED_LIB NAMES mupdf PATHS /usr/lib /usr/local/lib NO_DEFAULT_PATH)
find_path(MUPDF_INCLUDE_DIR mupdf/fitz.h PATHS /usr/include /usr/local/include)

if(MUPDF_SHARED_LIB AND MUPDF_INCLUDE_DIR)
    message(STATUS "Found MuPDF shared library: ${MUPDF_SHARED_LIB}")
    set(MUPDF_FOUND TRUE)
    set(MUPDF_LIBRARIES ${MUPDF_SHARED_LIB})
    set(MUPDF_INCLUDE_DIRS ${MUPDF_INCLUDE_DIR})
else()
    # Fallback to pkg-config if manual search fails
    pkg_check_modules(MUPDF mupdf)
    if(NOT MUPDF_FOUND)
        message(STATUS "MuPDF not found")
    endif()
endif()

# ONNX Runtime (fallback for non-LibTorch models)
find_library(ONNXRUNTIME_LIB onnxruntime PATHS /usr/lib /usr/local/lib)
find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h PATHS /usr/include/onnxruntime /usr/local/include/onnxruntime)

if(ONNXRUNTIME_LIB AND ONNXRUNTIME_INCLUDE_DIR)
    message(STATUS "ONNX Runtime found - AI layout detection enabled (backup)")
    add_definitions(-DUSE_ONNX_RUNTIME)
    include_directories(${ONNXRUNTIME_INCLUDE_DIR})
else()
    message(STATUS "ONNX Runtime not found - only LibTorch models supported")
endif()

# Tesseract for OCR
pkg_check_modules(TESSERACT REQUIRED tesseract)
pkg_check_modules(LEPTONICA REQUIRED lept)

# JSON library (nlohmann/json)
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
    # Download single header version if not found
    file(DOWNLOAD 
        "https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/json.hpp"
    )
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE src/)
endif()

# Threads
find_package(Threads REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/pdf_processor.cpp
    src/text_corrector.cpp
    src/heading_classifier.cpp
    src/yolo_inference.cpp
    src/utils.cpp
)

# Create executable
add_executable(pdf_processor ${SOURCES})

# Set RPATH for runtime library discovery
set_target_properties(pdf_processor PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Link libraries
target_link_libraries(pdf_processor
    ${OpenCV_LIBS}
    ${TESSERACT_LIBRARIES}
    ${LEPTONICA_LIBRARIES}
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Link LibTorch if found (for .pt model support)
if(USE_LIBTORCH)
    target_link_libraries(pdf_processor ${TORCH_LIBRARIES})
    set_property(TARGET pdf_processor PROPERTY CXX_STANDARD 17)
    message(STATUS "LibTorch linking: ${TORCH_LIBRARIES}")
endif()

# Link ONNX Runtime if found
if(ONNXRUNTIME_LIB)
    target_link_libraries(pdf_processor ${ONNXRUNTIME_LIB})
endif()

# Link MuPDF - Always use shared library
if(MUPDF_FOUND)
    target_include_directories(pdf_processor PRIVATE ${MUPDF_INCLUDE_DIRS})
    target_link_libraries(pdf_processor ${MUPDF_LIBRARIES})
    add_definitions(-DUSE_MUPDF)
    message(STATUS "Linking MuPDF: ${MUPDF_LIBRARIES}")
else()
    message(STATUS "MuPDF not found - PDF processing disabled")
endif()

# Configuration summary
message(STATUS "=== PDF Processor Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "LibTorch: ${USE_LIBTORCH}")
if(USE_LIBTORCH)
    message(STATUS "LibTorch libraries: ${TORCH_LIBRARIES}")
endif()
message(STATUS "ONNX Runtime: ${ONNXRUNTIME_LIB}")
message(STATUS "MuPDF library: ${MUPDF_LIBRARIES}")
message(STATUS "Tesseract: ${TESSERACT_LIBRARIES}")
message(STATUS "====================================")
