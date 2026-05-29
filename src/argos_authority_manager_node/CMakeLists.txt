cmake_minimum_required(VERSION 3.16)
project(argos_authority_manager_node)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(argos_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(argos_core_authority REQUIRED)

add_executable(argos_authority_manager_node
  src/AuthorityManagerNode.cpp
)

ament_target_dependencies(argos_authority_manager_node
  rclcpp
  rclcpp_lifecycle
  argos_msgs
  std_msgs
)

target_link_libraries(argos_authority_manager_node
  argos_core_authority::argos_core_authority_lib
)

target_compile_options(argos_authority_manager_node PRIVATE
  -Wall -Wextra -Wpedantic
)

install(TARGETS argos_authority_manager_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
