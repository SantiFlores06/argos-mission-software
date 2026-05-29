#pragma once

#include "IFaultMonitor.hpp"
#include <map>
#include <mutex>
#include <functional>

namespace argos {

struct HealthThreshold {
    double warning_threshold{0.0};
    double error_threshold{0.0};
    bool   higher_is_worse{true};
};

class FaultMonitorImpl : public IFaultMonitor {
public:
    using FaultCallback = std::function<void(const FaultEvent&)>;

    void setFaultCallback(FaultCallback cb);
    void setThreshold(const std::string& source, Subsystem subsystem,
                      double warning, double error, bool higher_is_worse = true);

    void registerHealthSource(const std::string& topic) override;
    std::vector<FaultEvent> getActiveFaults() const override;
    void clearFault(uint32_t fault_id) override;

    void evaluateHealth(const std::string& source, double value);

private:
    mutable std::mutex                      mutex_;
    std::map<uint32_t, FaultEvent>          active_faults_;
    std::map<std::string, HealthThreshold>  thresholds_;
    std::map<std::string, Subsystem>        source_to_subsystem_;
    FaultCallback                           fault_callback_;
    uint32_t                                next_fault_id_{1};

    uint32_t generateFaultId();
    void     publishFault(const FaultEvent& event);
};

}  // namespace argos
