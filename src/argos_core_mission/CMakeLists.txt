cmake_minimum_required(VERSION 3.16)
project(argos_core_mission VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(argos_core_mission_lib SHARED
  src/MissionExecutorImpl.cpp
)

target_include_directories(argos_core_mission_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(argos_core_mission_lib PRIVATE
  -Wall -Wextra -Wpedantic -Werror
)

if(BUILD_TESTING)
  find_package(GTest REQUIRED)
  enable_testing()

  add_executable(test_mission_executor
    test/test_mission_executor.cpp
  )
  target_link_libraries(test_mission_executor
    argos_core_mission_lib
    GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(test_mission_executor)
endif()

install(TARGETS argos_core_mission_lib
  EXPORT argos_core_mission_libTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include
)
