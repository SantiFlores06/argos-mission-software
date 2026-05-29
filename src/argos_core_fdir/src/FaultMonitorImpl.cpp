#include "argos_core_fdir/FaultMonitorImpl.hpp"

namespace argos {

void FaultMonitorImpl::setFaultCallback(FaultCallback cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    fault_callback_ = std::move(cb);
}

void FaultMonitorImpl::setThreshold(const std::string& source, Subsystem subsystem,
                                     double warning, double error, bool higher_is_worse)
{
    std::lock_guard<std::mutex> lock(mutex_);
    thresholds_[source]          = {warning, error, higher_is_worse};
    source_to_subsystem_[source] = subsystem;
}

void FaultMonitorImpl::registerHealthSource(const std::string& topic)
{
    // Registration is handled via setThreshold in practice.
    // This satisfies the IFaultMonitor interface contract for topic-name bookkeeping.
    std::lock_guard<std::mutex> lock(mutex_);
    if (thresholds_.find(topic) == thresholds_.end()) {
        thresholds_[topic] = {};
    }
}

std::vector<FaultEvent> FaultMonitorImpl::getActiveFaults() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<FaultEvent> result;
    result.reserve(active_faults_.size());
    for (const auto& [id, event] : active_faults_) {
        result.push_back(event);
    }
    return result;
}

void FaultMonitorImpl::clearFault(uint32_t fault_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    active_faults_.erase(fault_id);
}

void FaultMonitorImpl::evaluateHealth(const std::string& source, double value)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = thresholds_.find(source);
    if (it == thresholds_.end()) return;

    const HealthThreshold& thresh = it->second;
    Subsystem subsystem = source_to_subsystem_.count(source)
                              ? source_to_subsystem_.at(source)
                              : Subsystem::GPS;

    FaultSeverity severity{};
    bool fault_triggered = false;

    if (thresh.higher_is_worse) {
        if (value >= thresh.error_threshold) {
            severity = FaultSeverity::ERROR;
            fault_triggered = true;
        } else if (value >= thresh.warning_threshold) {
            severity = FaultSeverity::WARNING;
            fault_triggered = true;
        }
    } else {
        if (value <= thresh.error_threshold) {
            severity = FaultSeverity::ERROR;
            fault_triggered = true;
        } else if (value <= thresh.warning_threshold) {
            severity = FaultSeverity::WARNING;
            fault_triggered = true;
        }
    }

    if (fault_triggered) {
        FaultEvent event;
        event.fault_id       = generateFaultId();
        event.subsystem      = subsystem;
        event.severity       = severity;
        event.description    = "Health threshold exceeded: " + source;
        event.metric_value   = value;
        event.threshold_value = (severity == FaultSeverity::ERROR)
                                    ? thresh.error_threshold
                                    : thresh.warning_threshold;
        active_faults_[event.fault_id] = event;
        publishFault(event);
    }
}

uint32_t FaultMonitorImpl::generateFaultId()
{
    return next_fault_id_++;
}

void FaultMonitorImpl::publishFault(const FaultEvent& event)
{
    if (fault_callback_) {
        fault_callback_(event);
    }
}

}  // namespace argos
