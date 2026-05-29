cmake_minimum_required(VERSION 3.16)
project(argos_msgs)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/MissionStatus.msg"
  "msg/FlightState.msg"
  "msg/FaultEvent.msg"
  "msg/SystemHealth.msg"
  "msg/NodeHeartbeat.msg"
  "msg/WaypointList.msg"
  "srv/ArmDisarm.srv"
  "srv/LoadMission.srv"
  "srv/SetRecoveryPolicy.srv"
  "srv/RequestAuthority.srv"
  "action/ExecuteMission.action"
  "action/NavigateTo.action"
  DEPENDENCIES
    std_msgs
    geometry_msgs
    builtin_interfaces
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
