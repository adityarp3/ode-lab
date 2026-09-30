cmake_minimum_required(VERSION 3.16)
project(odelab CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Lets editors (VSCode C/C++ extension, clangd) resolve includes correctly.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Core math library — no web/IO framework dependencies on purpose,
# so it's trivial to bind into Python (pybind11) or Java (JNI) later.
add_library(odelab
    src/state.cpp
    src/euler.cpp
    src/rk4.cpp
    src/solver.cpp
    src/phase_line.cpp
    src/expression.cpp
    src/direction_field.cpp
    src/io.cpp
)
target_include_directories(odelab PUBLIC include)

# CLI for local testing / generating data files without a server.
add_executable(odelab_cli apps/cli/main.cpp)
target_link_libraries(odelab_cli PRIVATE odelab)

# Showcase examples — one per tool, spanning 1D, 2D, and 3D systems.
foreach(example_name
    logistic_phase_line
    bistable_phase_line
    slope_field_demo
    van_der_pol
    predator_prey
    lorenz
)
    add_executable(${example_name} examples/${example_name}.cpp)
    target_link_libraries(${example_name} PRIVATE odelab)
endforeach()

enable_testing()
add_subdirectory(tests)
