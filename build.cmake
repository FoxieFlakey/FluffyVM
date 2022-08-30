# Build config

set(BUILD_PROJECT_NAME "FluffyVM")

# We're making library
set(BUILD_IS_LIBRARY YES)

# If we want make libary and
# executable project
set(BUILD_INSTALL_EXECUTABLE YES)

# Sources which common between exe and library
set(BUILD_SOURCES
  src/format/bytecode.pb-c.c
  src/fiber.c
  src/vm.c
  src/coroutine.c
  src/value.c
  src/fiber_impl/posix_thread.c
  src/fiber_impl/ucontext.c
)

# Note that exe does not represent Windows' 
# exe its just short hand of executable 
#
# Note:
# Still creates executable even building library. 
# This would contain test codes if project is 
# library. The executable directly links to the 
# library objects instead through shared library
set(BUILD_EXE_SOURCES
  src/specials.c
  src/premain.c
  src/main.c
)

# Public header to be exported
# If this a library
set(BUILD_PUBLIC_HEADERS
  include/FluffyVM/dummy.h
)

# AddPkgConfigLib is in ./buildsystem/CMakeLists.txt
macro(AddDependencies)
  # Example
  # AddPkgConfigLib(FluffyGC FluffyGC>=1.0.0)
  AddPkgConfigLib(FluffyGC FluffyGC>=1.0.0)
endmacro()


