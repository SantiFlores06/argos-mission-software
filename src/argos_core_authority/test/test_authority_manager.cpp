#include <gtest/gtest.h>
#include "argos_core_authority/AuthorityManagerImpl.hpp"

using namespace argos;

class AuthorityManagerTest : public ::testing::Test {
protected:
    AuthorityManagerImpl mgr;
};

TEST_F(AuthorityManagerTest, InitialAuthorityIsNone)
{
    EXPECT_EQ(mgr.getCurrentAuthority(), CommandSource::NONE);
}

TEST_F(AuthorityManagerTest, FirstRequestGranted)
{
    EXPECT_TRUE(mgr.requestAuthority(CommandSource::FLIGHT_LIFECYCLE_FSM, 100));
    EXPECT_EQ(mgr.getCurrentAuthority(), CommandSource::FLIGHT_LIFECYCLE_FSM);
}

TEST_F(AuthorityManagerTest, LowerPriorityDenied)
{
    mgr.requestAuthority(CommandSource::FLIGHT_LIFECYCLE_FSM, 100);
    EXPECT_FALSE(mgr.requestAuthority(CommandSource::BEHAVIOR_TREE, 50));
    EXPECT_EQ(mgr.getCurrentAuthority(), CommandSource::FLIGHT_LIFECYCLE_FSM);
}

TEST_F(AuthorityManagerTest, HigherPriorityPreempts)
{
    mgr.requestAuthority(CommandSource::BEHAVIOR_TREE, 50);
    EXPECT_TRUE(mgr.requestAuthority(CommandSource::RECOVERY_MANAGER, 200));
    EXPECT_EQ(mgr.getCurrentAuthority(), CommandSource::RECOVERY_MANAGER);
}

TEST_F(AuthorityManagerTest, GCSOverrideAlwaysWins)
{
    mgr.requestAuthority(CommandSource::RECOVERY_MANAGER, 200);
    EXPECT_TRUE(mgr.requestAuthority(CommandSource::GCS_OVERRIDE, 255));
    EXPECT_TRUE(mgr.isAuthorityGranted(CommandSource::GCS_OVERRIDE));
}

TEST_F(AuthorityManagerTest, ReleaseRestoresNone)
{
    mgr.requestAuthority(CommandSource::FLIGHT_LIFECYCLE_FSM, 100);
    mgr.releaseAuthority(CommandSource::FLIGHT_LIFECYCLE_FSM);
    EXPECT_EQ(mgr.getCurrentAuthority(), CommandSource::NONE);
}

TEST_F(AuthorityManagerTest, ReleasingNonHolderHasNoEffect)
{
    mgr.requestAuthority(CommandSource::FLIGHT_LIFECYCLE_FSM, 100);
    mgr.releaseAuthority(CommandSource::BEHAVIOR_TREE);
    EXPECT_EQ(mgr.getCurrentAuthority(), CommandSource::FLIGHT_LIFECYCLE_FSM);
}

TEST_F(AuthorityManagerTest, IsAuthorityGrantedReturnsFalseForNonHolder)
{
    mgr.requestAuthority(CommandSource::FLIGHT_LIFECYCLE_FSM, 100);
    EXPECT_FALSE(mgr.isAuthorityGranted(CommandSource::BEHAVIOR_TREE));
    EXPECT_TRUE(mgr.isAuthorityGranted(CommandSource::FLIGHT_LIFECYCLE_FSM));
}
