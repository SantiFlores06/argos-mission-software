#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace argos {

enum class Subsystem : uint8_t {
    GPS        = 1,
    IMU        = 2,
    BATTERY    = 3,
    GCS_LINK   = 4,
    EKF        = 5,
    NAVIGATION = 6,
};

enum class FaultSeverity : uint8_t {
    WARNING  = 1,
    ERROR    = 2,
    CRITICAL = 3,
};

struct FaultEvent {
    uint32_t     fault_id;
    Subsystem    subsystem;
    FaultSeverity severity;
    std::string  description;
    double       metric_value;
    double       threshold_value;
};

class IFaultMonitor {
public:
    virtual ~IFaultMonitor() = default;

    virtual void registerHealthSource(const std::string& topic) = 0;
    virtual std::vector<FaultEvent> getActiveFaults() const = 0;
    virtual void clearFault(uint32_t fault_id) = 0;
};

}  // namespace argos
