add_library(libpso-engine)

target_include_directories(libpso-engine 
  PUBLIC 
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_sources(libpso-engine
  PRIVATE
  libpso/engine/particle/initializer.cpp
  libpso/engine/particle/updater.cpp
  libpso/engine/pso-runner.cpp
)

target_link_libraries(libpso-engine
  PUBLIC
  libcvrp-core
  libcvrp-engine
  libpso-core
)

find_package(OpenMP REQUIRED)
target_link_libraries(libpso-engine
  PUBLIC
  OpenMP::OpenMP_CXX
)

if(BUILD_TESTING)
  # Procura todos os arquivos de teste na pasta
  file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "libpso/engine/*.test.cpp")

  foreach(TEST_SRC ${TEST_SOURCES})
      
    get_filename_component(BASE_NAME ${TEST_SRC} NAME)
    string(REPLACE "." "_" TEST_TARGET_NAME ${BASE_NAME}) 
    set(TEST_TARGET_NAME "pso-${TEST_TARGET_NAME}")
    
    add_executable(${TEST_TARGET_NAME} 
      ${TEST_SRC}
    )

    target_include_directories(${TEST_TARGET_NAME} 
      PRIVATE 
      "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_link_libraries(${TEST_TARGET_NAME} 
      PRIVATE 
      libpso-engine
    )

    # Registra o teste no CTest
    add_test(NAME ${TEST_TARGET_NAME} COMMAND ${TEST_TARGET_NAME})
      
  endforeach()
endif()