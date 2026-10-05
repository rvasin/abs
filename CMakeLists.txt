cmake_minimum_required(VERSION 3.20)

project(abs
  DESCRIPTION "Abs - a minimalistic functional scripting language"
  LANGUAGES CXX)

# gnu++11, not c++11: run() calls popen, which MinGW only declares in the GNU
# dialect. See the comment at src/abs.cpp:1121. CMAKE_CXX_EXTENSIONS defaults to
# ON, so this is the gnu variant - keep it that way.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(ABS_STATIC "Link the runtime statically for distribution" OFF)

add_executable(abs
  src/abs.cpp
  src/absmain.cpp
  src/atoment.cpp
  src/utils.cpp)

target_include_directories(abs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# put the interpreter at build/abs rather than build/<config>/abs, so that
# tests/conftest.py can find it at a single predictable path
set_target_properties(abs PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

if(MSVC)
  target_compile_options(abs PRIVATE /W4)
else()
  target_compile_options(abs PRIVATE -Wall -Wextra)
  if(ABS_STATIC)
    if(MINGW)
      # full -static also works, but static-libgcc/stdc++ produce a smaller
      # binary and avoid dragging in the MSYS2 POSIX layer
      target_link_options(abs PRIVATE -static-libgcc -static-libstdc++ -static)
    else()
      target_link_options(abs PRIVATE -static)
    endif()
  endif()
endif()
