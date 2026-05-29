#pragma once

#include "IRecoveryStrategy.hpp"
#include <memory>
#include <mutex>
#include <vector>

namespace argos {

class RecoveryManager {
public:
    void registerStrategy(std::unique_ptr<IRecoveryStrategy> strategy);
    void handleFault(const FaultEvent& fault);
    void abortRecovery();

    bool isRecovering() const;
    const IRecoveryStrategy* activeStrategy() const;

private:
    mutable std::mutex                          mutex_;
    std::vector<std::unique_ptr<IRecoveryStrategy>> strategies_;
    IRecoveryStrategy*                          active_strategy_{nullptr};

    IRecoveryStrategy* selectStrategy(const FaultEvent& fault);
};

}  // namespace argos
