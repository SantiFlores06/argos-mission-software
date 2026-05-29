#pragma once

#include "IMissionExecutor.hpp"
#include <mutex>

namespace argos {

class MissionExecutorImpl : public IMissionExecutor {
public:
    bool loadMission(const std::vector<MissionWaypoint>& waypoints,
                     float acceptance_radius_m,
                     float cruise_speed_mps) override;
    bool startMission() override;
    void pauseMission() override;
    void abortMission() override;

    MissionStatus getStatus() const override;
    uint32_t      getCurrentWaypointIndex() const override;
    uint32_t      getTotalWaypoints() const override;
    float         getMissionProgressPct() const override;

    void markWaypointReached();

private:
    mutable std::mutex          mutex_;
    MissionStatus               status_{MissionStatus::IDLE};
    std::vector<MissionWaypoint> waypoints_;
    uint32_t                    current_index_{0};
    float                       acceptance_radius_m_{1.0f};
    float                       cruise_speed_mps_{5.0f};
};

}  // namespace argos
