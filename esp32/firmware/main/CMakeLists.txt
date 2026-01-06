# ESP32 Modal Resonator Node
# CMakeLists.txt for ESP-IDF build system

idf_component_register(
    SRCS
        "main.c"
        "core/modal_node.c"
        "audio/audio_synth.c"
        "audio/audio_i2s.c"
        "network/protocol.c"
        "network/esp_now_manager.c"
        "config/session_config.c"
        "config/topology.c"
        "config/presets.c"
        "utils/complex_math.c"
        "utils/crc32.c"

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
