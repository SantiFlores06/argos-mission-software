#include "argos_core_mission/MissionExecutorImpl.hpp"

namespace argos {

bool MissionExecutorImpl::loadMission(const std::vector<MissionWaypoint>& waypoints,
                                       float acceptance_radius_m,
                                       float cruise_speed_mps)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == MissionStatus::RUNNING) return false;

    waypoints_           = waypoints;
    acceptance_radius_m_ = acceptance_radius_m;
    cruise_speed_mps_    = cruise_speed_mps;
    current_index_       = 0;
    status_              = MissionStatus::LOADING;
    return true;
}

bool MissionExecutorImpl::startMission()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (waypoints_.empty()) return false;
    if (status_ != MissionStatus::LOADING && status_ != MissionStatus::PAUSED) return false;

    status_ = MissionStatus::RUNNING;
    return true;
}

void MissionExecutorImpl::pauseMission()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == MissionStatus::RUNNING) {
        status_ = MissionStatus::PAUSED;
    }
}

void MissionExecutorImpl::abortMission()
{
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = MissionStatus::ABORTED;
}

MissionStatus MissionExecutorImpl::getStatus() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

uint32_t MissionExecutorImpl::getCurrentWaypointIndex() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return current_index_;
}

uint32_t MissionExecutorImpl::getTotalWaypoints() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<uint32_t>(waypoints_.size());
}

float MissionExecutorImpl::getMissionProgressPct() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (waypoints_.empty()) return 0.0f;
    return static_cast<float>(current_index_) / static_cast<float>(waypoints_.size()) * 100.0f;
}

void MissionExecutorImpl::markWaypointReached()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ != MissionStatus::RUNNING) return;

    ++current_index_;
    if (current_index_ >= waypoints_.size()) {
        status_ = MissionStatus::COMPLETE;
    }
}

}  // namespace argos
