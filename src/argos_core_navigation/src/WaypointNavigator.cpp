#include "argos_core_navigation/WaypointNavigator.hpp"
#include <cmath>
#include <stdexcept>

namespace argos {

WaypointNavigator::WaypointNavigator(const IStateEstimator& estimator)
    : estimator_(estimator)
{
}

void WaypointNavigator::loadWaypoints(const std::vector<Waypoint>& waypoints)
{
    std::lock_guard<std::mutex> lock(mutex_);
    waypoints_     = waypoints;
    current_index_ = 0;
    aborted_       = false;
}

bool WaypointNavigator::advanceToNextWaypoint()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (aborted_ || waypoints_.empty()) return false;

    if (current_index_ + 1 < waypoints_.size()) {
        ++current_index_;
        return true;
    }
    return false;
}

Waypoint WaypointNavigator::getCurrentWaypoint() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (waypoints_.empty()) {
        throw std::runtime_error("No waypoints loaded");
    }
    return waypoints_[current_index_];
}

bool WaypointNavigator::isAtCurrentWaypoint() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (waypoints_.empty() || aborted_) return false;
    return distanceToWaypoint(waypoints_[current_index_]) <
           waypoints_[current_index_].acceptance_radius_m;
}

void WaypointNavigator::abort()
{
    std::lock_guard<std::mutex> lock(mutex_);
    aborted_ = true;
}

bool WaypointNavigator::isFinished() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (waypoints_.empty() || aborted_) return true;
    return current_index_ >= waypoints_.size();
}

double WaypointNavigator::distanceToWaypoint(const Waypoint& wp) const
{
    Position3D pos = estimator_.getPosition();
    double dx = pos.x - wp.x;
    double dy = pos.y - wp.y;
    double dz = pos.z - wp.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

}  // namespace argos
