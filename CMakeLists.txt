cmake_minimum_required(VERSION 3.22)

project(vrp_pso)

add_executable(vrp_pso
    src/util.cpp
    src/Particle.cpp
    src/Velocity.cpp
    src/Scanner.cpp
    src/LocalSearch.cpp
    src/PSO.cpp
    src/main.cpp
)

target_include_directories(vrp_pso PRIVATE include)