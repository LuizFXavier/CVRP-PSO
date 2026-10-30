cmake_minimum_required(VERSION 4.0)

project(cvrp-pso VERSION 2.0.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(libcvrp-core)
add_subdirectory(libcvrp-engine)

option(ENABLE_SYCL "Habilitar compilacao da versao paralela com SYCL (AdaptiveCpp)" OFF)

if(ENABLE_SYCL)
  add_subdirectory(libcvrp-sycl)
endif()

add_subdirectory(libpso-core)
add_subdirectory(libpso-engine)
add_subdirectory(app)
