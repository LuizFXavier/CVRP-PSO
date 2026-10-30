add_executable(cvrp-pso)

target_sources(cvrp-pso
  PRIVATE
  app/command-line.cpp 
  app/main.cpp
)

target_include_directories(cvrp-pso PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(cvrp-pso
  PUBLIC
  libcvrp-engine
  libpso-engine
)

IF (ENABLE_SYCL)

  add_executable(cvrp-sycl)

  target_sources(cvrp-sycl
    PRIVATE
    app/command-line.cpp 
    app/sycl.cpp
  )

  target_include_directories(cvrp-sycl PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

  target_link_libraries(cvrp-sycl
    PUBLIC
    libcvrp-engine
    libpso-engine
    libcvrp-sycl
  )

endif()

if(BUILD_TESTING)
    # Procura todos os ficheiros de teste na pasta
    file(GLOB TEST_SOURCES "app/*.test.cpp")

    foreach(TEST_SRC ${TEST_SOURCES})
        
        # Gera um nome seguro para o alvo (ex: command_line_test_cpp)
        get_filename_component(BASE_NAME ${TEST_SRC} NAME)
        string(REPLACE "." "_" TEST_TARGET_NAME ${BASE_NAME})
        set(TEST_TARGET_NAME "app-${TEST_TARGET_NAME}")
        
        add_executable(${TEST_TARGET_NAME} 
            ${TEST_SRC}
            app/command-line.cpp 
        )

        target_include_directories(${TEST_TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

        target_link_libraries(${TEST_TARGET_NAME} PRIVATE 
            libpso-core
        )

        # Registra o teste no CTest
        add_test(NAME ${TEST_TARGET_NAME} COMMAND ${TEST_TARGET_NAME})
        
    endforeach()
endif()
