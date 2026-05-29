#pragma once

#include "IFaultMonitor.hpp"
#include <string>

namespace argos {

enum class RecoveryResult {
    SUCCESS,
    FAILURE,
    IN_PROGRESS,
    ABORTED,
};

struct RecoveryContext {
    FaultEvent active_fault;
    double     current_altitude_m{0.0};
    double     home_lat{0.0};
    double     home_lon{0.0};
    double     home_alt{0.0};
};

class IRecoveryStrategy {
public:
    virtual ~IRecoveryStrategy() = default;

    virtual std::string    getName() const = 0;
    virtual RecoveryResult execute(const RecoveryContext& context) = 0;
    virtual bool           canHandle(const FaultEvent& fault) const = 0;
    virtual void           abort() = 0;
};

}  // namespace argos
