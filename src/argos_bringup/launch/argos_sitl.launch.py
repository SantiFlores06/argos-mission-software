from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node, LifecycleNode
from launch_ros.substitutions import FindPackageShare
from launch_ros.event_handlers import OnStateTransition
from launch.actions import EmitEvent, RegisterEventHandler
import launch_ros.events.lifecycle as lifecycle_events


def generate_launch_description():
    pkg_bringup = FindPackageShare("argos_bringup")

    watchdog_cfg  = PathJoinSubstitution([pkg_bringup, "config", "watchdog_timeouts.yaml"])
    ekf_cfg       = PathJoinSubstitution([pkg_bringup, "config", "ekf_params.yaml"])
    recovery_cfg  = PathJoinSubstitution([pkg_bringup, "config", "recovery_policies.yaml"])

    # ── Infrastructure layer ──────────────────────────────────────────────────
    system_watchdog = LifecycleNode(
        package="argos_system_watchdog_node",
        executable="argos_system_watchdog_node",
        name="argos_system_watchdog",
        parameters=[watchdog_cfg],
        output="screen",
    )

    authority_manager = LifecycleNode(
        package="argos_authority_manager_node",
        executable="argos_authority_manager_node",
        name="argos_authority_manager",
        output="screen",
    )

    fault_monitor = LifecycleNode(
        package="argos_fault_monitor_node",
        executable="argos_fault_monitor_node",
        name="argos_fault_monitor",
        parameters=[recovery_cfg],
        output="screen",
    )

    recovery_manager = LifecycleNode(
        package="argos_recovery_manager_node",
        executable="argos_recovery_manager_node",
        name="argos_recovery_manager",
        parameters=[recovery_cfg],
        output="screen",
    )

    health_reporter = LifecycleNode(
        package="argos_health_reporter_node",
        executable="argos_health_reporter_node",
        name="argos_health_reporter",
        output="screen",
    )

    # ── Navigation layer ──────────────────────────────────────────────────────
    state_estimator = LifecycleNode(
        package="argos_state_estimator_node",
        executable="argos_state_estimator_node",
        name="argos_state_estimator",
        parameters=[ekf_cfg],
        output="screen",
    )

    navigation_manager = LifecycleNode(
        package="argos_navigation_manager_node",
        executable="argos_navigation_manager_node",
        name="argos_navigation_manager",
        output="screen",
    )

    # ── Mission layer ─────────────────────────────────────────────────────────
    mission_executor = LifecycleNode(
        package="argos_mission_executor_node",
        executable="argos_mission_executor_node",
        name="argos_mission_executor",
        output="screen",
    )

    bt_engine = LifecycleNode(
        package="argos_behavior_tree_engine_node",
        executable="argos_behavior_tree_engine_node",
        name="argos_behavior_tree_engine",
        output="screen",
    )

    # ── Communication layer ───────────────────────────────────────────────────
    gcs_bridge = LifecycleNode(
        package="argos_gcs_bridge_node",
        executable="argos_gcs_bridge_node",
        name="argos_gcs_bridge",
        output="screen",
    )

    return LaunchDescription([
        system_watchdog,
        authority_manager,
        fault_monitor,
        recovery_manager,
        health_reporter,
        state_estimator,
        navigation_manager,
        mission_executor,
        bt_engine,
        gcs_bridge,
    ])
