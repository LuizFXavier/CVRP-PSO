cmake_minimum_required(VERSION 4.0)

project(cvrp-pso VERSION 0.0.1 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(libcvrp-core)
add_subdirectory(libcvrp-engine)

add_subdirectory(libpso-core)
add_subdirectory(app)