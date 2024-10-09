cmake_minimum_required(VERSION 3.22)

project(vrp_pso)

add_executable(vrp_pso
    src/Particle.cpp
    src/Velocity.cpp
    src/Cidade.cpp
    src/PSO.cpp
    src/main.cpp
)

target_include_directories(vrp_pso PRIVATE include)