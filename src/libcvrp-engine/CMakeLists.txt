add_library(libcvrp-engine)

target_include_directories(libcvrp-engine 
  PUBLIC 
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_sources(libcvrp-engine
  PRIVATE
  libcvrp/engine/io.cpp
  libcvrp/engine/geometry.cpp
  libcvrp/engine/splitter.cpp
  libcvrp/engine/local-search.cpp
)

target_link_libraries(libcvrp-engine
  PUBLIC
  libcvrp-core
)

find_package(OpenMP REQUIRED)
target_link_libraries(libcvrp-engine
  PUBLIC
  OpenMP::OpenMP_CXX
)

if(BUILD_TESTING)
  # Procura todos os arquivos de teste na pasta
  file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "libcvrp/engine/*.test.cpp")

  foreach(TEST_SRC ${TEST_SOURCES})
    
    get_filename_component(BASE_NAME ${TEST_SRC} NAME)
    string(REPLACE "." "_" TEST_TARGET_NAME ${BASE_NAME})

    set(TEST_TARGET_NAME "cvrp-${TEST_TARGET_NAME}")
    
    add_executable(${TEST_TARGET_NAME} 
      ${TEST_SRC}
    )

    target_include_directories(${TEST_TARGET_NAME} 
      PRIVATE 
      "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_link_libraries(${TEST_TARGET_NAME} 
      PRIVATE 
      libcvrp-engine
    )

    # Registra o teste no CTest
    add_test(NAME ${TEST_TARGET_NAME} COMMAND ${TEST_TARGET_NAME})
      
  endforeach()
endif()