cmake_minimum_required(VERSION 3.16)
project(reflappy CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

file(GLOB SRC CONFIGURE_DEPENDS src/*.cpp)
add_executable(reflappy ${SRC})
target_link_libraries(reflappy PRIVATE SDL2::SDL2 SDL2_image::SDL2_image)

# SDL2_mixer ships a CMake config on current distros. Fall back to pkg-config elsewhere.
find_package(SDL2_mixer QUIET)
if(TARGET SDL2_mixer::SDL2_mixer)
  target_link_libraries(reflappy PRIVATE SDL2_mixer::SDL2_mixer)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)
  target_link_libraries(reflappy PRIVATE PkgConfig::SDL2_MIXER)
endif()
