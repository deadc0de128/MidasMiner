# CMakeList.txt : CMake project for stella_disasm, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0076 NEW)

project("MidasMiner")

if (MSVC)
    set(CMAKE_SUPPRESS_REGENERATION true) # do not create ALL_BUILD and ZERO_CHECK projects
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wno-switch -Wno-deprecated-declarations)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Setting Clang compiler...")
    add_compile_options(-stdlib=libc++)
    add_compile_options(-fcolor-diagnostics)
    add_compile_options(-D_LIBCPP_ENABLE_CXX17_REMOVED_FEATURES)
    add_link_options(-stdlib=libc++)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-gdwarf-4 -O0)
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Setting GNU compiler...")
    add_compile_options(-fdiagnostics-color=always)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-ggdb -O0)
    endif()
endif()

find_package(SDL2 REQUIRED)

# Add source to this project's executable.
include_directories(${SDL2_INCLUDE_DIRS})
add_executable(MidasMiner Animations.cpp Grid.cpp MidasMiner.cpp Objects.cpp)
target_link_libraries(MidasMiner ${SDL2_LIBRARIES} SDL2_image)


