#pragma once

#include "IRecoveryStrategy.hpp"

namespace argos {

class ReturnToHomeStrategy : public IRecoveryStrategy {
public:
    std::string    getName() const override { return "ReturnToHome"; }
    RecoveryResult execute(const RecoveryContext& context) override;
    bool           canHandle(const FaultEvent& fault) const override;
    void           abort() override;

private:
    bool aborted_{false};
};

class EmergencyLandStrategy : public IRecoveryStrategy {
public:
    std::string    getName() const override { return "EmergencyLand"; }
    RecoveryResult execute(const RecoveryContext& context) override;
    bool           canHandle(const FaultEvent& fault) const override;
    void           abort() override;

private:
    bool aborted_{false};
};

class HoldPositionStrategy : public IRecoveryStrategy {
public:
    std::string    getName() const override { return "HoldPosition"; }
    RecoveryResult execute(const RecoveryContext& context) override;
    bool           canHandle(const FaultEvent& fault) const override;
    void           abort() override;

private:
    bool aborted_{false};
};

}  // namespace argos
