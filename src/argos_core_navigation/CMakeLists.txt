cmake_minimum_required(VERSION 3.16)
project(argos_core_navigation VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(argos_core_navigation_lib SHARED
  src/EKFStateEstimator.cpp
  src/WaypointNavigator.cpp
)

target_include_directories(argos_core_navigation_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(argos_core_navigation_lib PRIVATE
  -Wall -Wextra -Wpedantic -Werror
)

if(BUILD_TESTING)
  find_package(GTest REQUIRED)
  enable_testing()

  add_executable(test_waypoint_navigator
    test/test_waypoint_navigator.cpp
  )
  target_link_libraries(test_waypoint_navigator
    argos_core_navigation_lib
    GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(test_waypoint_navigator)
endif()

install(TARGETS argos_core_navigation_lib
  EXPORT argos_core_navigation_libTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include
)
