#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <argos_msgs/msg/node_heartbeat.hpp>
#include <argos_msgs/srv/request_authority.hpp>
#include <std_msgs/msg/string.hpp>

#include "argos_core_authority/AuthorityManagerImpl.hpp"

#include <chrono>
#include <memory>
#include <string>

using namespace std::chrono_literals;
using rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface;

static const std::map<uint8_t, argos::CommandSource> kSourceMap = {
    {0, argos::CommandSource::NONE},
    {1, argos::CommandSource::FLIGHT_LIFECYCLE_FSM},
    {2, argos::CommandSource::BEHAVIOR_TREE},
    {3, argos::CommandSource::RECOVERY_MANAGER},
    {4, argos::CommandSource::GCS_OVERRIDE},
};

static const std::map<argos::CommandSource, std::string> kSourceName = {
    {argos::CommandSource::NONE,                 "NONE"},
    {argos::CommandSource::FLIGHT_LIFECYCLE_FSM, "FLIGHT_LIFECYCLE_FSM"},
    {argos::CommandSource::BEHAVIOR_TREE,        "BEHAVIOR_TREE"},
    {argos::CommandSource::RECOVERY_MANAGER,     "RECOVERY_MANAGER"},
    {argos::CommandSource::GCS_OVERRIDE,         "GCS_OVERRIDE"},
};

class AuthorityManagerNode : public rclcpp_lifecycle::LifecycleNode {
public:
    AuthorityManagerNode()
        : LifecycleNode("argos_authority_manager"),
          authority_manager_(std::make_unique<argos::AuthorityManagerImpl>())
    {
        declare_parameter("heartbeat_period_ms", 100);
    }

    LifecycleNodeInterface::CallbackReturn on_configure(const rclcpp_lifecycle::State&) override
    {
        int hb_ms = get_parameter("heartbeat_period_ms").as_int();

        authority_service_ = create_service<argos_msgs::srv::RequestAuthority>(
            "/argos/authority/request",
            [this](const std::shared_ptr<argos_msgs::srv::RequestAuthority::Request> req,
                   std::shared_ptr<argos_msgs::srv::RequestAuthority::Response> res) {
                handleAuthorityRequest(req, res);
            });

        current_authority_pub_ = create_publisher<std_msgs::msg::String>(
            "/argos/authority/current",
            rclcpp::QoS(10).reliable().transient_local());

        heartbeat_pub_ = create_publisher<argos_msgs::msg::NodeHeartbeat>(
            "/argos/watchdog/heartbeat",
            rclcpp::QoS(10).best_effort());

        heartbeat_timer_ = create_wall_timer(
            std::chrono::milliseconds(hb_ms),
            [this, hb_ms]() { publishHeartbeat(hb_ms); });

        RCLCPP_INFO(get_logger(), "AuthorityManagerNode configured");
        return LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    LifecycleNodeInterface::CallbackReturn on_activate(const rclcpp_lifecycle::State&) override
    {
        current_authority_pub_->on_activate();
        heartbeat_pub_->on_activate();
        publishCurrentAuthority();
        RCLCPP_INFO(get_logger(), "AuthorityManagerNode active");
        return LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    LifecycleNodeInterface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State&) override
    {
        current_authority_pub_->on_deactivate();
        heartbeat_pub_->on_deactivate();
        return LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    LifecycleNodeInterface::CallbackReturn on_cleanup(const rclcpp_lifecycle::State&) override
    {
        authority_service_.reset();
        heartbeat_timer_.reset();
        current_authority_pub_.reset();
        heartbeat_pub_.reset();
        authority_manager_ = std::make_unique<argos::AuthorityManagerImpl>();
        return LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

private:
    void handleAuthorityRequest(
        const std::shared_ptr<argos_msgs::srv::RequestAuthority::Request> req,
        std::shared_ptr<argos_msgs::srv::RequestAuthority::Response> res)
    {
        auto it = kSourceMap.find(req->source);
        if (it == kSourceMap.end()) {
            res->granted = false;
            res->current_holder = "INVALID_SOURCE";
            return;
        }

        bool granted = authority_manager_->requestAuthority(it->second, req->priority);
        res->granted = granted;

        auto current = authority_manager_->getCurrentAuthority();
        auto name_it = kSourceName.find(current);
        res->current_holder = (name_it != kSourceName.end()) ? name_it->second : "UNKNOWN";

        if (granted) {
            RCLCPP_INFO(get_logger(), "Authority granted to: %s", res->current_holder.c_str());
            publishCurrentAuthority();
        } else {
            RCLCPP_WARN(get_logger(), "Authority denied to source %d — holder: %s",
                        req->source, res->current_holder.c_str());
        }
    }

    void publishCurrentAuthority()
    {
        if (!current_authority_pub_ || !current_authority_pub_->is_activated()) return;
        auto current = authority_manager_->getCurrentAuthority();
        auto it      = kSourceName.find(current);
        std_msgs::msg::String msg;
        msg.data = (it != kSourceName.end()) ? it->second : "UNKNOWN";
        current_authority_pub_->publish(msg);
    }

    void publishHeartbeat(int period_ms)
    {
        if (!heartbeat_pub_->is_activated()) return;
        argos_msgs::msg::NodeHeartbeat hb;
        hb.header.stamp      = now();
        hb.node_name         = get_name();
        hb.sequence          = ++heartbeat_seq_;
        hb.period.sec        = period_ms / 1000;
        hb.period.nanosec    = (period_ms % 1000) * 1'000'000;
        heartbeat_pub_->publish(hb);
    }

    std::unique_ptr<argos::AuthorityManagerImpl> authority_manager_;

    rclcpp::Service<argos_msgs::srv::RequestAuthority>::SharedPtr authority_service_;
    rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::String>::SharedPtr current_authority_pub_;
    rclcpp_lifecycle::LifecyclePublisher<argos_msgs::msg::NodeHeartbeat>::SharedPtr heartbeat_pub_;
    rclcpp::TimerBase::SharedPtr heartbeat_timer_;

    uint64_t heartbeat_seq_{0};
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<AuthorityManagerNode>();
    rclcpp::executors::SingleThreadedExecutor exec;
    exec.add_node(node->get_node_base_interface());
    exec.spin();
    rclcpp::shutdown();
    return 0;
}
