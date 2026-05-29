cmake_minimum_required(VERSION 3.16)
project(argos_core_fdir VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(argos_core_fdir_lib SHARED
  src/FaultMonitorImpl.cpp
  src/RecoveryStrategies.cpp
  src/RecoveryManager.cpp
)

target_include_directories(argos_core_fdir_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(argos_core_fdir_lib PRIVATE
  -Wall -Wextra -Wpedantic -Werror
)

if(BUILD_TESTING)
  find_package(GTest REQUIRED)
  enable_testing()

  add_executable(test_fault_monitor
    test/test_fault_monitor.cpp
  )
  target_link_libraries(test_fault_monitor
    argos_core_fdir_lib
    GTest::gtest_main
  )

  add_executable(test_recovery_strategies
    test/test_recovery_strategies.cpp
  )
  target_link_libraries(test_recovery_strategies
    argos_core_fdir_lib
    GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(test_fault_monitor)
  gtest_discover_tests(test_recovery_strategies)
endif()

install(TARGETS argos_core_fdir_lib
  EXPORT argos_core_fdir_libTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include
)
