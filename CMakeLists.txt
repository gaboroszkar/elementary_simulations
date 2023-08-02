cmake_minimum_required(VERSION 3.22)

project(simulation_visualizations)

# Generate compile_commands.json.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(add_external_subdirectories)
    # External libraries are dynamically linked.
    set(BUILD_SHARED_LIBS ON)

    # elementary_visualizer.
    set(BUILD_TESTING OFF)
    set(ELEMENTARY_VISUALIZER_BUILD_EXAMPLES OFF)
    set(ELEMENTARY_VISUALIZER_FFMPEG_CONFIG "--enable-libvpx")
    set(ELEMENTARY_VISUALIZER_DIR ${PROJECT_SOURCE_DIR}/external/elementary_visualizer/)
    add_subdirectory(${ELEMENTARY_VISUALIZER_DIR})
endfunction()

add_external_subdirectories()

function(add_simulation NAME SOURCE_FILE)
    set(COMMON_SOURCES
        src/framework.cpp
        src/slider.cpp
    )

    add_executable(${NAME} ${COMMON_SOURCES} ${SOURCE_FILE})
    set_property(TARGET ${NAME} PROPERTY CXX_STANDARD 20)
    target_compile_options(${NAME} PRIVATE -Werror -Wall -Wextra)
    target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_link_libraries(${NAME} elementary_visualizer)

    # By default the library search path for the executable is set
    # by absolute path. This makes sure to set library search path
    # relative to the executable, and not with absolute directory path.
    set_target_properties(${NAME} PROPERTIES BUILD_RPATH_USE_ORIGIN TRUE)

    # Make formatting run before we build the project.
    add_dependencies(${NAME} simulation_clangformat)
endfunction()

# Add simulations.
add_simulation(0_simulation src/0_simulation.cpp)
add_simulation(1_periodic_wave src/1_periodic_wave.cpp)

# Add include directories;
# applies only to this subproject.
include_directories(
    ${PROJECT_SOURCE_DIR}/src
)

file(
    GLOB_RECURSE
    SOURCES_TO_FORMAT
    src/*.c
    src/*.cpp
    src/*.h
    src/*.hpp
)
add_custom_target(
    simulation_clangformat
    ALL
    COMMAND /usr/bin/clang-format
    -i
    ${SOURCES_TO_FORMAT}
)
