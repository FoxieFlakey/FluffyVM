cmake_minimum_required(VERSION 3.18)

# Source of truth for version is 
# NOT HERE its in Kconfig file
project(FluffyVM VERSION 1.0.0
                 LANGUAGES C)

if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "Non clang compiler unsupported")
endif()

# C standard checks
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

##################################
## Dont touch anything but this ##
##################################
set(PROJECT_SOURCES
  src/format/bytecode.pb-c.c
  src/vm.c
)

set(PROJECT_PUBLIC_HEADERS
  include/FluffyVM/dummy.h
)

set(PROJECT_TEST_SOURCES
  src/specials.c
  src/premain.c
  src/main.c
)
##################################
  
# Suppression files (abusing configure)
configure_file(
  ${PROJECT_SOURCE_DIR}/suppressions/UBSan.supp 
  ${PROJECT_BINARY_DIR}/suppressions/UBSan.supp
)

##################################
include(scripts/process_dot_config.cmake)
include(scripts/compiler_and_link_flags.cmake)

configure_file(src/config.h src/processed_config.h)
configure_file(${PROJECT_SOURCE_DIR}/.config ${PROJECT_BINARY_DIR}/.config)

add_library(${PROJECT_NAME}_Objects OBJECT ${PROJECT_SOURCES})
add_library(${PROJECT_NAME} SHARED)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
  VERSION ${CONFIG_VERSION_MAJOR}.${CONFIG_VERSION_MINOR}.${CONFIG_VERSION_PATCH}${VERSION_LOCAL_VERSION}
  SOVERSION ${CONFIG_VERSION_MAJOR}
)

find_package(PkgConfig REQUIRED)
function (AddPkgConfigLib name spec)
  pkg_check_modules(${name} REQUIRED ${spec})
  target_link_libraries(ProjectCommonFlags INTERFACE ${${name}_LIBRARIES})
  target_include_directories(ProjectCommonFlags INTERFACE ${${name}_INCLUDE_DIRS})
  target_compile_options(ProjectCommonFlags INTERFACE ${${name}_CFLAGS_OTHER})
endfunction()

## Deps here ##
target_link_libraries(${PROJECT_NAME}_Objects ProjectCommonFlags)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_Objects)

AddPkgConfigLib(FluffyGC FluffyGC>=1.0.0)
###############

add_executable(TestExecutable ${PROJECT_TEST_SOURCES})
target_link_libraries(TestExecutable ProjectCommonFlags)
target_link_libraries(TestExecutable ${PROJECT_NAME}_Objects)

# Installations
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} DESTINATION lib/${PROJECT_NAME})

# Install library headers
install(
  FILES ${PROJECT_PUBLIC_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ExampleProject
)

# Install library
install(
  TARGETS ${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/
  PERMISSIONS WORLD_EXECUTE GROUP_EXECUTE OWNER_EXECUTE
  PERMISSIONS
    WORLD_READ    GROUP_READ    OWNER_READ      
                                OWNER_WRITE
    WORLD_EXECUTE GROUP_EXECUTE OWNER_EXECUTE
)

# Install pkgconfig file
set(target ${PROJECT_NAME})
configure_file(package.pc.in
  ${CMAKE_BINARY_DIR}/ExampleProject.pc @ONLY)
install(
  FILES ${CMAKE_BINARY_DIR}/ExampleProject.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig/
)



















