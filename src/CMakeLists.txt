cmake_minimum_required(VERSION 4.0)

project(cvrp-pso VERSION 1.9.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(libcvrp-core)
add_subdirectory(libcvrp-engine)

add_subdirectory(libpso-core)
add_subdirectory(libpso-engine)
add_subdirectory(app)