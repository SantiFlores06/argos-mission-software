#include <gtest/gtest.h>
#include "argos_core_fdir/FaultMonitorImpl.hpp"

using namespace argos;

class FaultMonitorTest : public ::testing::Test {
protected:
    FaultMonitorImpl monitor;

    void SetUp() override {
        monitor.setThreshold("gps.sats", Subsystem::GPS, 8.0, 4.0, /*higher_is_worse=*/false);
        monitor.setThreshold("gps.hdop", Subsystem::GPS, 2.0, 5.0, /*higher_is_worse=*/true);
        monitor.setThreshold("battery.pct", Subsystem::BATTERY, 30.0, 15.0, /*higher_is_worse=*/false);
    }
};

TEST_F(FaultMonitorTest, NoFaultsInitially)
{
    EXPECT_TRUE(monitor.getActiveFaults().empty());
}

TEST_F(FaultMonitorTest, GoodHealthProducesNoFault)
{
    monitor.evaluateHealth("gps.sats", 12.0);
    EXPECT_TRUE(monitor.getActiveFaults().empty());
}

TEST_F(FaultMonitorTest, GPSDropoutTriggersFault)
{
    monitor.evaluateHealth("gps.sats", 3.0);  // below error threshold of 4
    auto faults = monitor.getActiveFaults();
    ASSERT_EQ(faults.size(), 1u);
    EXPECT_EQ(faults[0].severity, FaultSeverity::ERROR);
    EXPECT_EQ(faults[0].subsystem, Subsystem::GPS);
}

TEST_F(FaultMonitorTest, WarningThresholdTriggered)
{
    monitor.evaluateHealth("gps.sats", 6.0);  // between warning(8) and error(4)
    auto faults = monitor.getActiveFaults();
    ASSERT_EQ(faults.size(), 1u);
    EXPECT_EQ(faults[0].severity, FaultSeverity::WARNING);
}

TEST_F(FaultMonitorTest, BatteryCriticalTriggersFault)
{
    monitor.evaluateHealth("battery.pct", 10.0);  // below error threshold 15%
    auto faults = monitor.getActiveFaults();
    ASSERT_FALSE(faults.empty());
    EXPECT_EQ(faults[0].subsystem, Subsystem::BATTERY);
}

TEST_F(FaultMonitorTest, ClearFaultRemovesIt)
{
    monitor.evaluateHealth("gps.sats", 2.0);
    auto faults = monitor.getActiveFaults();
    ASSERT_FALSE(faults.empty());
    monitor.clearFault(faults[0].fault_id);
    EXPECT_TRUE(monitor.getActiveFaults().empty());
}

TEST_F(FaultMonitorTest, CallbackInvokedOnFault)
{
    bool callback_called = false;
    monitor.setFaultCallback([&](const FaultEvent&) { callback_called = true; });
    monitor.evaluateHealth("gps.sats", 2.0);
    EXPECT_TRUE(callback_called);
}
