#include <gtest/gtest.h>
#include "argos_core_mission/MissionExecutorImpl.hpp"

using namespace argos;

class MissionExecutorTest : public ::testing::Test {
protected:
    MissionExecutorImpl executor;

    std::vector<MissionWaypoint> threeWaypoints() {
        return {
            {0.0, 0.0, -10.0, 1.0, 5.0},
            {10.0, 0.0, -10.0, 1.0, 5.0},
            {10.0, 10.0, -10.0, 1.0, 5.0},
        };
    }
};

TEST_F(MissionExecutorTest, InitialStatusIsIdle)
{
    EXPECT_EQ(executor.getStatus(), MissionStatus::IDLE);
}

TEST_F(MissionExecutorTest, LoadTransitionsToLoading)
{
    EXPECT_TRUE(executor.loadMission(threeWaypoints(), 1.0f, 5.0f));
    EXPECT_EQ(executor.getStatus(), MissionStatus::LOADING);
    EXPECT_EQ(executor.getTotalWaypoints(), 3u);
}

TEST_F(MissionExecutorTest, StartAfterLoadSucceeds)
{
    executor.loadMission(threeWaypoints(), 1.0f, 5.0f);
    EXPECT_TRUE(executor.startMission());
    EXPECT_EQ(executor.getStatus(), MissionStatus::RUNNING);
}

TEST_F(MissionExecutorTest, StartWithoutLoadFails)
{
    EXPECT_FALSE(executor.startMission());
}

TEST_F(MissionExecutorTest, PauseWhileRunning)
{
    executor.loadMission(threeWaypoints(), 1.0f, 5.0f);
    executor.startMission();
    executor.pauseMission();
    EXPECT_EQ(executor.getStatus(), MissionStatus::PAUSED);
}

TEST_F(MissionExecutorTest, AbortSetsAbortedStatus)
{
    executor.loadMission(threeWaypoints(), 1.0f, 5.0f);
    executor.startMission();
    executor.abortMission();
    EXPECT_EQ(executor.getStatus(), MissionStatus::ABORTED);
}

TEST_F(MissionExecutorTest, ProgressUpdatesOnWaypointReached)
{
    executor.loadMission(threeWaypoints(), 1.0f, 5.0f);
    executor.startMission();
    EXPECT_FLOAT_EQ(executor.getMissionProgressPct(), 0.0f);
    executor.markWaypointReached();
    EXPECT_GT(executor.getMissionProgressPct(), 0.0f);
}

TEST_F(MissionExecutorTest, AllWaypointsReachedSetsComplete)
{
    executor.loadMission(threeWaypoints(), 1.0f, 5.0f);
    executor.startMission();
    executor.markWaypointReached();
    executor.markWaypointReached();
    executor.markWaypointReached();
    EXPECT_EQ(executor.getStatus(), MissionStatus::COMPLETE);
}
