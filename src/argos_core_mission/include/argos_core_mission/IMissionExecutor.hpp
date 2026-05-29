#pragma once

#include <string>
#include <vector>

namespace argos {

enum class MissionStatus : uint8_t {
    IDLE     = 0,
    LOADING  = 1,
    RUNNING  = 2,
    PAUSED   = 3,
    COMPLETE = 4,
    ABORTED  = 5,
    FAILED   = 6,
};

struct MissionWaypoint {
    double x{0.0};
    double y{0.0};
    double z{0.0};
    double acceptance_radius_m{1.0};
    double speed_mps{5.0};
};

class IMissionExecutor {
public:
    virtual ~IMissionExecutor() = default;

    virtual bool loadMission(const std::vector<MissionWaypoint>& waypoints,
                             float acceptance_radius_m,
                             float cruise_speed_mps) = 0;
    virtual bool startMission() = 0;
    virtual void pauseMission() = 0;
    virtual void abortMission() = 0;

    virtual MissionStatus getStatus() const = 0;
    virtual uint32_t      getCurrentWaypointIndex() const = 0;
    virtual uint32_t      getTotalWaypoints() const = 0;
    virtual float         getMissionProgressPct() const = 0;
};

}  // namespace argos
