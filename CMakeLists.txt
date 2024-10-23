cmake_minimum_required(VERSION 3.22)

project(vrp_pso)
add_subdirectory(matplotplusplus)

add_executable(vrp_pso
    src/Particle.cpp
    src/Velocity.cpp
    src/Cidade.cpp
    src/Scanner.cpp
    src/PSO.cpp
    src/Grafico.cpp
    src/main.cpp
)
target_link_libraries(vrp_pso PUBLIC matplot)
target_include_directories(vrp_pso PRIVATE include)