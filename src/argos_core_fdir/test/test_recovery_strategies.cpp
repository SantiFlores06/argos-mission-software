#include <gtest/gtest.h>
#include "argos_core_fdir/RecoveryStrategies.hpp"
#include "argos_core_fdir/RecoveryManager.hpp"
#include <memory>

using namespace argos;

TEST(ReturnToHomeStrategyTest, HandlesGCSLinkFault)
{
    ReturnToHomeStrategy strategy;
    FaultEvent fault;
    fault.subsystem = Subsystem::GCS_LINK;
    fault.severity  = FaultSeverity::ERROR;
    EXPECT_TRUE(strategy.canHandle(fault));
}

TEST(ReturnToHomeStrategyTest, DoesNotHandleBatteryCritical)
{
    ReturnToHomeStrategy strategy;
    FaultEvent fault;
    fault.subsystem = Subsystem::BATTERY;
    fault.severity  = FaultSeverity::ERROR;
    EXPECT_FALSE(strategy.canHandle(fault));
}

TEST(EmergencyLandStrategyTest, HandlesBatteryCritical)
{
    EmergencyLandStrategy strategy;
    FaultEvent fault;
    fault.subsystem = Subsystem::BATTERY;
    fault.severity  = FaultSeverity::ERROR;
    EXPECT_TRUE(strategy.canHandle(fault));
}

TEST(HoldPositionStrategyTest, HandlesGPSDegradation)
{
    HoldPositionStrategy strategy;
    FaultEvent fault;
    fault.subsystem = Subsystem::GPS;
    fault.severity  = FaultSeverity::WARNING;
    EXPECT_TRUE(strategy.canHandle(fault));
}

TEST(RecoveryManagerTest, SelectsCorrectStrategyForGPS)
{
    RecoveryManager mgr;
    mgr.registerStrategy(std::make_unique<ReturnToHomeStrategy>());
    mgr.registerStrategy(std::make_unique<EmergencyLandStrategy>());
    mgr.registerStrategy(std::make_unique<HoldPositionStrategy>());

    FaultEvent fault;
    fault.fault_id  = 1;
    fault.subsystem = Subsystem::GPS;
    fault.severity  = FaultSeverity::ERROR;

    mgr.handleFault(fault);
    ASSERT_NE(mgr.activeStrategy(), nullptr);
    EXPECT_EQ(mgr.activeStrategy()->getName(), "HoldPosition");
}

TEST(RecoveryManagerTest, AbortClearsActiveStrategy)
{
    RecoveryManager mgr;
    mgr.registerStrategy(std::make_unique<HoldPositionStrategy>());

    FaultEvent fault;
    fault.fault_id  = 1;
    fault.subsystem = Subsystem::GPS;
    fault.severity  = FaultSeverity::ERROR;
    mgr.handleFault(fault);

    mgr.abortRecovery();
    EXPECT_FALSE(mgr.isRecovering());
}
