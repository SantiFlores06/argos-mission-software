#include "argos_core_fdir/RecoveryManager.hpp"

namespace argos {

void RecoveryManager::registerStrategy(std::unique_ptr<IRecoveryStrategy> strategy)
{
    std::lock_guard<std::mutex> lock(mutex_);
    strategies_.push_back(std::move(strategy));
}

void RecoveryManager::handleFault(const FaultEvent& fault)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (active_strategy_) {
        // A recovery is already running; do not interrupt unless the new fault
        // is CRITICAL and the current strategy cannot handle it.
        if (fault.severity < FaultSeverity::CRITICAL) return;
        active_strategy_->abort();
    }

    active_strategy_ = selectStrategy(fault);
    if (active_strategy_) {
        RecoveryContext ctx;
        ctx.active_fault = fault;
        active_strategy_->execute(ctx);
    }
}

void RecoveryManager::abortRecovery()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_strategy_) {
        active_strategy_->abort();
        active_strategy_ = nullptr;
    }
}

bool RecoveryManager::isRecovering() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return active_strategy_ != nullptr;
}

const IRecoveryStrategy* RecoveryManager::activeStrategy() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return active_strategy_;
}

IRecoveryStrategy* RecoveryManager::selectStrategy(const FaultEvent& fault)
{
    for (auto& strategy : strategies_) {
        if (strategy->canHandle(fault)) {
            return strategy.get();
        }
    }
    return nullptr;
}

}  // namespace argos
