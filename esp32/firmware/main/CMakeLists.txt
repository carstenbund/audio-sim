# ESP32 Modal Resonator Node
# CMakeLists.txt for ESP-IDF build system

# Determine which main file to use based on hub mode
if(CONFIG_MODAL_HUB_MODE)
    set(MAIN_SRC "hub_main.c")
    message(STATUS "Building in HUB MODE")
else()
    set(MAIN_SRC "main.c")
    message(STATUS "Building in NODE MODE")
endif()

idf_component_register(
    SRCS
        "${MAIN_SRC}"
        "core/modal_node.c"
        "audio/audio_synth.c"
        "audio/audio_i2s.c"
        "network/protocol.c"
        "network/esp_now_manager.c"
        "config/session_config.c"
        "config/hub_controller.c"

    INCLUDE_DIRS
        "."
        "core"
        "audio"
        "network"
        "config"
        "utils"

    REQUIRES
        nvs_flash
        esp_wifi
        driver
        esp_timer
)

# Compiler flags
target_compile_options(${COMPONENT_LIB} PRIVATE
    -O2                     # Optimize for performance
    -ffast-math             # Fast floating-point math
    -fno-signed-zeros       # Allow optimizations with zeros
    -Wall                   # Enable warnings
    -Wextra                 # Extra warnings
    -Werror                 # Treat warnings as errors
)

# Link math library
target_link_libraries(${COMPONENT_LIB} PRIVATE m)
