#include <gtest/gtest.h>
#include "argos_core_navigation/WaypointNavigator.hpp"
#include "argos_core_navigation/IStateEstimator.hpp"

using namespace argos;

// Minimal stub estimator for testing WaypointNavigator without real EKF.
class StubEstimator : public IStateEstimator {
public:
    Position3D pos;

    Position3D getPosition() const override { return pos; }
    Velocity3D getVelocity() const override { return {}; }
    Quaternion getAttitude() const override { return {}; }
    double     getCovarianceNorm() const override { return 0.1; }
    bool       isDiverged() const override { return false; }
    bool       isDeadReckoningActive() const override { return false; }
};

class WaypointNavigatorTest : public ::testing::Test {
protected:
    StubEstimator estimator;
    WaypointNavigator navigator{estimator};

    void SetUp() override {
        std::vector<Waypoint> wps = {
            {0.0, 0.0, -10.0, 1.0, 5.0},
            {10.0, 0.0, -10.0, 1.0, 5.0},
            {10.0, 10.0, -10.0, 1.0, 5.0},
        };
        navigator.loadWaypoints(wps);
    }
};

TEST_F(WaypointNavigatorTest, FirstWaypointIsCorrect)
{
    auto wp = navigator.getCurrentWaypoint();
    EXPECT_DOUBLE_EQ(wp.x, 0.0);
    EXPECT_DOUBLE_EQ(wp.y, 0.0);
}

TEST_F(WaypointNavigatorTest, NotAtWaypointWhenFar)
{
    estimator.pos = {50.0, 50.0, -10.0};
    EXPECT_FALSE(navigator.isAtCurrentWaypoint());
}

TEST_F(WaypointNavigatorTest, AtWaypointWhenWithinRadius)
{
    estimator.pos = {0.5, 0.0, -10.0};  // within 1.0m radius
    EXPECT_TRUE(navigator.isAtCurrentWaypoint());
}

TEST_F(WaypointNavigatorTest, AdvanceMovesToNextWaypoint)
{
    EXPECT_TRUE(navigator.advanceToNextWaypoint());
    auto wp = navigator.getCurrentWaypoint();
    EXPECT_DOUBLE_EQ(wp.x, 10.0);
}

TEST_F(WaypointNavigatorTest, AdvanceBeyondLastReturnsFalse)
{
    navigator.advanceToNextWaypoint();
    navigator.advanceToNextWaypoint();
    EXPECT_FALSE(navigator.advanceToNextWaypoint());
}

TEST_F(WaypointNavigatorTest, AbortPreventsAdvance)
{
    navigator.abort();
    EXPECT_FALSE(navigator.advanceToNextWaypoint());
    EXPECT_TRUE(navigator.isFinished());
}
