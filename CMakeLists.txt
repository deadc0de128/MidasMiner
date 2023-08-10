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
find_package(SDL2_image REQUIRED)

add_executable(MidasMiner "")
target_include_directories(MidasMiner PRIVATE SDL2::SDL2 SDL2_image::SDL2_image)
target_link_libraries(MidasMiner SDL2::SDL2 SDL2::SDL2main SDL2_image::SDL2_image)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(MidasMiner PRIVATE _LIBCPP_ENABLE_CXX17_REMOVED_FEATURES)
endif()

add_subdirectory(src)

if (MSVC)
    get_target_property(SDL2_DLL SDL2::SDL2 IMPORTED_LOCATION)
    get_target_property(SDL2_IMAGE_DLL SDL2_image::SDL2_image IMPORTED_LOCATION)

    add_custom_command(
        TARGET MidasMiner POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${SDL2_DLL}
                ${CMAKE_CURRENT_BINARY_DIR})

    add_custom_command(
        TARGET MidasMiner POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${SDL2_IMAGE_DLL}
                ${CMAKE_CURRENT_BINARY_DIR})
endif()
