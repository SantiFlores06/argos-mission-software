#pragma once

#include <vector>

namespace argos {

struct Waypoint {
    double x{0.0};
    double y{0.0};
    double z{0.0};
    double acceptance_radius_m{1.0};
    double speed_mps{5.0};
};

class INavigationManager {
public:
    virtual ~INavigationManager() = default;

    virtual void    loadWaypoints(const std::vector<Waypoint>& waypoints) = 0;
    virtual bool    advanceToNextWaypoint() = 0;
    virtual Waypoint getCurrentWaypoint() const = 0;
    virtual bool    isAtCurrentWaypoint() const = 0;
    virtual void    abort() = 0;
};

}  // namespace argos
