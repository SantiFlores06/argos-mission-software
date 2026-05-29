#include "argos_core_fdir/RecoveryStrategies.hpp"

namespace argos {

// ── ReturnToHomeStrategy ──────────────────────────────────────────────────────

RecoveryResult ReturnToHomeStrategy::execute(const RecoveryContext& /*context*/)
{
    if (aborted_) return RecoveryResult::ABORTED;
    // Real implementation delegates to NavigationManager via ROS2 action.
    // In the pure-C++ core, this returns IN_PROGRESS; the node adapter drives it.
    return RecoveryResult::IN_PROGRESS;
}

bool ReturnToHomeStrategy::canHandle(const FaultEvent& fault) const
{
    // RTH handles GCS link loss and navigation faults, but NOT battery critical
    // (battery critical requires immediate landing, not a trip home).
    return fault.subsystem == Subsystem::GCS_LINK ||
           fault.subsystem == Subsystem::NAVIGATION ||
           fault.subsystem == Subsystem::EKF;
}

void ReturnToHomeStrategy::abort()
{
    aborted_ = true;
}

// ── EmergencyLandStrategy ─────────────────────────────────────────────────────

RecoveryResult EmergencyLandStrategy::execute(const RecoveryContext& /*context*/)
{
    if (aborted_) return RecoveryResult::ABORTED;
    return RecoveryResult::IN_PROGRESS;
}

bool EmergencyLandStrategy::canHandle(const FaultEvent& fault) const
{
    // Battery critical — don't waste power flying home.
    return fault.subsystem == Subsystem::BATTERY &&
           fault.severity  >= FaultSeverity::ERROR;
}

void EmergencyLandStrategy::abort()
{
    aborted_ = true;
}

// ── HoldPositionStrategy ──────────────────────────────────────────────────────

RecoveryResult HoldPositionStrategy::execute(const RecoveryContext& /*context*/)
{
    if (aborted_) return RecoveryResult::ABORTED;
    // Hold is immediate — return SUCCESS once the vehicle stops moving.
    // The node adapter monitors velocity to confirm the hold.
    return RecoveryResult::IN_PROGRESS;
}

bool HoldPositionStrategy::canHandle(const FaultEvent& fault) const
{
    // GPS degradation — hold using dead-reckoning until GPS recovers.
    return fault.subsystem == Subsystem::GPS;
}

void HoldPositionStrategy::abort()
{
    aborted_ = true;
}

}  // namespace argos
