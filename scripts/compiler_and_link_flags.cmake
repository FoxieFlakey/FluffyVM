include_guard()

include(scripts/process_dot_config.cmake)

add_library(ProjectCommonFlags INTERFACE)

set_property(TARGET ProjectCommonFlags
  PROPERTY POSITION_INDEPENDENT_CODE YES
)

target_compile_options(ProjectCommonFlags INTERFACE "-g")
target_compile_options(ProjectCommonFlags INTERFACE "-fPIC")
target_compile_options(ProjectCommonFlags INTERFACE "-O0")
target_compile_options(ProjectCommonFlags INTERFACE "-Wall")
target_compile_options(ProjectCommonFlags INTERFACE "-fblocks")
target_compile_options(ProjectCommonFlags INTERFACE "-fno-sanitize-recover=all")
target_compile_options(ProjectCommonFlags INTERFACE "-fsanitize-recover=unsigned-integer-overflow")
target_compile_options(ProjectCommonFlags INTERFACE "-fno-omit-frame-pointer")
target_compile_options(ProjectCommonFlags INTERFACE "-fno-common")
target_compile_options(ProjectCommonFlags INTERFACE "-fno-optimize-sibling-calls")
target_compile_options(ProjectCommonFlags INTERFACE "-fvisibility=hidden")
target_link_options(ProjectCommonFlags INTERFACE "-fPIC")

if (DEFINED CONFIG_UBSAN)
  target_compile_options(ProjectCommonFlags INTERFACE "-fsanitize=undefined")
  #target_compile_options(ProjectCommonFlags INTERFACE "-fsanitize-address-use-after-return=always")
  target_compile_options(ProjectCommonFlags INTERFACE "-fsanitize=float-divide-by-zero")
  target_compile_options(ProjectCommonFlags INTERFACE "-fsanitize=implicit-conversion")
  target_compile_options(ProjectCommonFlags INTERFACE "-fsanitize=unsigned-integer-overflow")

  target_link_options(ProjectCommonFlags INTERFACE "-fsanitize=undefined")
  target_link_options(ProjectCommonFlags INTERFACE "-static-libsan")
endif()

target_compile_definitions(ProjectCommonFlags INTERFACE "PROCESSED_BY_CMAKE")

if (DEFINED CONFIG_ASAN)
  target_compile_options(ProjectCommonFlags INTERFACE "-fsanitize-address-use-after-scope")
  target_compile_options(ProjectCommonFlags INTERFACE "-fsanitize=address")
  target_link_options(ProjectCommonFlags INTERFACE "-fsanitize=address")
endif()

if (DEFINED CONFIG_TSAN)
  target_compile_options(ProjectCommonFlags INTERFACE "-fsanitize=thread")
  target_link_options(ProjectCommonFlags INTERFACE "-fsanitize=thread")
endif()

if (DEFINED CONFIG_XRAY)
  target_compile_options(ProjectCommonFlags INTERFACE "-fxray-instrument")
  target_compile_options(ProjectCommonFlags INTERFACE "-fxray-instruction-threshold=1")
  target_link_options(ProjectCommonFlags INTERFACE "-fxray-instrument")
  target_link_options(ProjectCommonFlags INTERFACE "-fxray-instruction-threshold=1")
endif()

if (DEFINED CONFIG_MSAN)
  target_compile_options(ProjectCommonFlags INTERFACE "-fsanitize-memory-track-origins")
  target_compile_options(ProjectCommonFlags INTERFACE "-fsanitize=memory")
  target_link_options(ProjectCommonFlags INTERFACE "-fsanitize=memory")
endif()

target_link_libraries(ProjectCommonFlags INTERFACE BlocksRuntime)
target_link_libraries(ProjectCommonFlags INTERFACE pthread)
target_link_libraries(ProjectCommonFlags INTERFACE protobuf-c)

target_include_directories(ProjectCommonFlags INTERFACE "${PROJECT_BINARY_DIR}/src")
target_include_directories(ProjectCommonFlags INTERFACE "${PROJECT_SOURCE_DIR}/src")
target_include_directories(ProjectCommonFlags INTERFACE "${PROJECT_SOURCE_DIR}/src/collection")
target_include_directories(ProjectCommonFlags INTERFACE "${PROJECT_SOURCE_DIR}/include")

