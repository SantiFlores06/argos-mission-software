#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <argos_msgs/msg/node_heartbeat.hpp>
#include <argos_msgs/msg/flight_state.hpp>
#include <std_msgs/msg/string.hpp>

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <string>

using namespace std::chrono_literals;
using rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface;

// The watchdog subscribes to /argos/watchdog/heartbeat from each critical node.
// If any node misses its heartbeat by more than (timeout_multiplier × declared_period),
// the watchdog publishes a FAULTED flight state to trigger the FDIR pipeline.
//
// Design choice — single watchdog vs. per-node watchdogs:
//   A single centralized watchdog is simpler and prevents cascading failures where
//   a dead node takes its own watchdog with it. The trade-off is a single point of
//   failure in the watchdog itself, mitigated by keeping it as a thin observer with
//   no command authority.

class SystemWatchdogNode : public rclcpp_lifecycle::LifecycleNode {
public:
    SystemWatchdogNode()
        : LifecycleNode("argos_system_watchdog")
    {
        declare_parameter("timeout_multiplier", 2.0);
        declare_parameter("check_period_ms", 100);
        declare_parameter("critical_nodes", std::vector<std::string>{
            "argos_authority_manager",
            "argos_fault_monitor",
            "argos_mission_executor",
            "argos_state_estimator",
            "argos_navigation_manager",
        });
    }

    LifecycleNodeInterface::CallbackReturn on_configure(const rclcpp_lifecycle::State&) override
    {
        timeout_multiplier_ = get_parameter("timeout_multiplier").as_double();
        int check_ms        = get_parameter("check_period_ms").as_int();
        critical_nodes_     = get_parameter("critical_nodes").as_string_array();

        heartbeat_sub_ = create_subscription<argos_msgs::msg::NodeHeartbeat>(
            "/argos/watchdog/heartbeat",
            rclcpp::QoS(10).best_effort(),
            [this](const argos_msgs::msg::NodeHeartbeat::SharedPtr msg) {
                onHeartbeat(msg);
            });

        fault_pub_ = create_publisher<std_msgs::msg::String>(
            "/argos/watchdog/node_timeout",
            rclcpp::QoS(10).reliable());

        check_timer_ = create_wall_timer(
            std::chrono::milliseconds(check_ms),
            [this]() { checkHeartbeats(); });

        RCLCPP_INFO(get_logger(), "SystemWatchdog configured — monitoring %zu nodes",
                    critical_nodes_.size());
        return LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    LifecycleNodeInterface::CallbackReturn on_activate(const rclcpp_lifecycle::State&) override
    {
        fault_pub_->on_activate();
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& name : critical_nodes_) {
            last_heartbeat_[name] = now;  // grace period from activation
            declared_period_ms_[name] = 100.0;  // default until first heartbeat
        }
        RCLCPP_INFO(get_logger(), "SystemWatchdog active");
        return LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    LifecycleNodeInterface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State&) override
    {
        fault_pub_->on_deactivate();
        return LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    LifecycleNodeInterface::CallbackReturn on_cleanup(const rclcpp_lifecycle::State&) override
    {
        heartbeat_sub_.reset();
        check_timer_.reset();
        fault_pub_.reset();
        return LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

private:
    void onHeartbeat(const argos_msgs::msg::NodeHeartbeat::SharedPtr msg)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        last_heartbeat_[msg->node_name] = std::chrono::steady_clock::now();

        double period_ms = msg->period.sec * 1000.0 + msg->period.nanosec / 1e6;
        if (period_ms > 0.0) {
            declared_period_ms_[msg->node_name] = period_ms;
        }
    }

    void checkHeartbeats()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();

        for (const auto& name : critical_nodes_) {
            auto it = last_heartbeat_.find(name);
            if (it == last_heartbeat_.end()) continue;

            double period_ms = declared_period_ms_.count(name)
                                   ? declared_period_ms_.at(name)
                                   : 100.0;
            double timeout_ms = timeout_multiplier_ * period_ms;
            double elapsed_ms = std::chrono::duration<double, std::milli>(
                                     now - it->second).count();

            if (elapsed_ms > timeout_ms) {
                RCLCPP_ERROR(get_logger(),
                             "TIMEOUT: node '%s' missed heartbeat (%.0f ms > %.0f ms timeout)",
                             name.c_str(), elapsed_ms, timeout_ms);
                publishTimeout(name);
            }
        }
    }

    void publishTimeout(const std::string& node_name)
    {
        if (!fault_pub_->is_activated()) return;
        std_msgs::msg::String msg;
        msg.data = node_name;
        fault_pub_->publish(msg);
    }

    rclcpp::Subscription<argos_msgs::msg::NodeHeartbeat>::SharedPtr heartbeat_sub_;
    rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::String>::SharedPtr fault_pub_;
    rclcpp::TimerBase::SharedPtr check_timer_;

    mutable std::mutex mutex_;
    std::map<std::string, std::chrono::steady_clock::time_point> last_heartbeat_;
    std::map<std::string, double> declared_period_ms_;

    double timeout_multiplier_{2.0};
    std::vector<std::string> critical_nodes_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<SystemWatchdogNode>();
    rclcpp::executors::SingleThreadedExecutor exec;
    exec.add_node(node->get_node_base_interface());
    exec.spin();
    rclcpp::shutdown();
    return 0;
}
