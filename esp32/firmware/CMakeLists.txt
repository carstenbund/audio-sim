# ESP32 Modal Resonator Network - Root CMakeLists.txt
# Compatible with ESP-IDF v5.0+

cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project definition
project(modal_resonator_node)

# Project metadata
set(PROJECT_VERSION "1.0.0")
set(PROJECT_DESCRIPTION "Distributed Modal Resonator Network Node")
