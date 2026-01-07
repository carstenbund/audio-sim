# Modal Attractors AU Plugin - CMake Build Configuration
# Based on AU Plugin Porting Proposal
# Phase 1: Core DSP Port - Standalone test application

cmake_minimum_required(VERSION 3.20)
project(ModalAttractorsAU VERSION 1.0.0 LANGUAGES C CXX)

# ============================================================================
# Build Options
# ============================================================================

option(BUILD_STANDALONE_TEST "Build standalone test application" ON)
option(BUILD_AU_PLUGIN "Build Audio Unit plugin (requires macOS + Xcode)" OFF)
option(ENABLE_SIMD "Enable SIMD optimizations (Accelerate framework)" OFF)

# ============================================================================
# Compiler Settings
# ============================================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-ffast-math)  # Enable fast math for audio DSP
endif()

# Architecture support (Universal Binary for macOS)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
endif()

# ============================================================================
# Source Files
# ============================================================================

# ESP32 ported DSP core (C)
set(ESP32_PORT_SOURCES
    src/esp32_port/modal_node.c
    src/esp32_port/audio_synth.c
)

set(ESP32_PORT_HEADERS
    src/esp32_port/modal_node.h
    src/esp32_port/audio_synth.h
)

# C++ DSP wrapper classes
set(DSP_CORE_SOURCES
    src/dsp_core/ModalVoice.cpp
    src/dsp_core/VoiceAllocator.cpp
    src/dsp_core/TopologyEngine.cpp
)

set(DSP_CORE_HEADERS
    src/dsp_core/ModalVoice.h
    src/dsp_core/VoiceAllocator.h
    src/dsp_core/TopologyEngine.h
)

# AU wrapper (C++ interface, actual AU code in Objective-C++)
set(AU_WRAPPER_SOURCES
    src/au_wrapper/ModalAttractorsEngine.cpp
)

set(AU_WRAPPER_HEADERS
    src/au_wrapper/ModalAttractorsAU.h
    src/au_wrapper/ModalParameters.h
)

# ============================================================================
# Library: Modal DSP Core
# ============================================================================

add_library(modal_dsp_core STATIC
    ${ESP32_PORT_SOURCES}
    ${ESP32_PORT_HEADERS}
    ${DSP_CORE_SOURCES}
    ${DSP_CORE_HEADERS}
    ${AU_WRAPPER_SOURCES}
    ${AU_WRAPPER_HEADERS}
)

target_include_directories(modal_dsp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/esp32_port
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp_core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/au_wrapper
)

# Link math library
target_link_libraries(modal_dsp_core PUBLIC m)

# macOS Accelerate framework (SIMD optimizations)
if(APPLE AND ENABLE_SIMD)
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    target_link_libraries(modal_dsp_core PUBLIC ${ACCELERATE_FRAMEWORK})
    target_compile_definitions(modal_dsp_core PUBLIC USE_ACCELERATE)
endif()

# ============================================================================
# Executable: Standalone Test (Phase 1 deliverable)
# ============================================================================

if(BUILD_STANDALONE_TEST)
    add_executable(test_modal_voice
        Tests/test_modal_voice.cpp
    )

    target_link_libraries(test_modal_voice PRIVATE modal_dsp_core)

    # Install test binary
    install(TARGETS test_modal_voice
        RUNTIME DESTINATION bin
    )
endif()

# ============================================================================
# Audio Unit Plugin (Phase 2 - requires Xcode and AU SDK)
# ============================================================================

if(BUILD_AU_PLUGIN AND APPLE)
    message(STATUS "Audio Unit plugin build not yet implemented")
    message(STATUS "Requires Xcode project with AudioUnit.framework")
    # TODO: Add AU plugin bundle target
    # This would typically be done via Xcode project, not CMake
endif()

# ============================================================================
# Installation
# ============================================================================

# Install headers (for external use)
install(DIRECTORY src/esp32_port/
    DESTINATION include/modal_attractors/esp32_port
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY src/dsp_core/
    DESTINATION include/modal_attractors/dsp_core
    FILES_MATCHING PATTERN "*.h"
)

# Install library
install(TARGETS modal_dsp_core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# ============================================================================
# Documentation
# ============================================================================

# Installation paths
set(DOC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs)

# Print build summary
message(STATUS "==============================================")
message(STATUS "Modal Attractors AU Plugin - Build Configuration")
message(STATUS "==============================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if(APPLE)
    message(STATUS "Target architectures: ${CMAKE_OSX_ARCHITECTURES}")
    message(STATUS "macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()
message(STATUS "SIMD optimizations: ${ENABLE_SIMD}")
message(STATUS "Build standalone test: ${BUILD_STANDALONE_TEST}")
message(STATUS "Build AU plugin: ${BUILD_AU_PLUGIN}")
message(STATUS "==============================================")
