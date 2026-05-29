cmake_minimum_required(VERSION 3.16)
project(argos_system_watchdog_node)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(argos_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(argos_system_watchdog_node
  src/SystemWatchdogNode.cpp
)

ament_target_dependencies(argos_system_watchdog_node
  rclcpp
  rclcpp_lifecycle
  argos_msgs
  std_msgs
)

target_compile_options(argos_system_watchdog_node PRIVATE
  -Wall -Wextra -Wpedantic
)

install(TARGETS argos_system_watchdog_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
