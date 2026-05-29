#pragma once

#include "INavigationManager.hpp"
#include "IStateEstimator.hpp"
#include <mutex>

namespace argos {

class WaypointNavigator : public INavigationManager {
public:
    explicit WaypointNavigator(const IStateEstimator& estimator);

    void     loadWaypoints(const std::vector<Waypoint>& waypoints) override;
    bool     advanceToNextWaypoint() override;
    Waypoint getCurrentWaypoint() const override;
    bool     isAtCurrentWaypoint() const override;
    void     abort() override;

    bool isFinished() const;

private:
    mutable std::mutex       mutex_;
    const IStateEstimator&   estimator_;
    std::vector<Waypoint>    waypoints_;
    size_t                   current_index_{0};
    bool                     aborted_{false};

    double distanceToWaypoint(const Waypoint& wp) const;
};

}  // namespace argos
