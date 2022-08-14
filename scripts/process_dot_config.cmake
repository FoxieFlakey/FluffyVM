include_guard()

set(PROJECT_CONFIG_FILE ${PROJECT_SOURCE_DIR}/.config CACHE STRING "" FORCE)

# Process .config files
# Create header
file(READ ${PROJECT_CONFIG_FILE} filedata)
string(REGEX REPLACE "#" "//" filedata "${filedata}")
string(REGEX REPLACE "CONFIG" "#define CONFIG" filedata "${filedata}")
string(REGEX REPLACE "(#define[^=]+)=y\n" "\\1 1\n" filedata "${filedata}")
string(REGEX REPLACE "(#define[^=]+)=n\n" "\\1 0\n" filedata "${filedata}")
string(REGEX REPLACE "(#define[^=]+)=([^\n]+)\n" "\\1 \\2\n" filedata "${filedata}")
file(WRITE ${PROJECT_BINARY_DIR}/src/kconfig.h "${filedata}")

# Create cmake config
file(READ ${PROJECT_CONFIG_FILE} filedata)
string(REGEX REPLACE "CONFIG" "set(CONFIG" filedata "${filedata}")
string(REGEX REPLACE "(set[^=]+)=y\n" "\\1, ON)\n" filedata "${filedata}")
string(REGEX REPLACE "(set[^=]+)=n\n" "\\1, OFF)\n" filedata "${filedata}")
string(REGEX REPLACE "(set[^=]+)=([^\n]+)\n" "\\1 \\2)\n" filedata "${filedata}")
string(REGEX REPLACE "(CONFIG_[A-Z0-9a-z_]+)," "\\1" filedata "${filedata}")
file(WRITE ${PROJECT_BINARY_DIR}/config.cmake "${filedata}")

# Load CMake config
include(${PROJECT_BINARY_DIR}/config.cmake)

