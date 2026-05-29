cmake_minimum_required(VERSION 3.16)
project(argos_core_authority VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(argos_core_authority_lib SHARED
  src/AuthorityManagerImpl.cpp
)

target_include_directories(argos_core_authority_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(argos_core_authority_lib PRIVATE
  -Wall -Wextra -Wpedantic -Werror
)

if(BUILD_TESTING)
  find_package(GTest REQUIRED)
  enable_testing()

  add_executable(test_authority_manager
    test/test_authority_manager.cpp
  )
  target_link_libraries(test_authority_manager
    argos_core_authority_lib
    GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(test_authority_manager)
endif()

install(TARGETS argos_core_authority_lib
  EXPORT argos_core_authority_libTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include
)
