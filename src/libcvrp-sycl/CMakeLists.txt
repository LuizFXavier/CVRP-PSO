add_library(libcvrp-sycl)

target_include_directories(libcvrp-sycl
  PUBLIC 
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_sources(libcvrp-sycl
  PRIVATE
  libcvrp/sycl/local-search.cpp
)

target_link_libraries(libcvrp-sycl
  PUBLIC
  libcvrp-core
)

find_package(AdaptiveCpp CONFIG REQUIRED)

add_sycl_to_target(TARGET libcvrp-sycl)

if(BUILD_TESTING)
  # Procura todos os arquivos de teste na pasta
  file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "libcvrp/sycl/*.test.cpp")

  foreach(TEST_SRC ${TEST_SOURCES})
    
    get_filename_component(BASE_NAME ${TEST_SRC} NAME)
    string(REPLACE "." "_" TEST_TARGET_NAME ${BASE_NAME})

    set(TEST_TARGET_NAME "sycl-${TEST_TARGET_NAME}")
    
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