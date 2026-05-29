#include "argos_core_navigation/EKFStateEstimator.hpp"
#include <cmath>

namespace argos {

EKFStateEstimator::EKFStateEstimator(double divergence_threshold,
                                     double dead_reckoning_timeout_s)
    : divergence_threshold_(divergence_threshold),
      dead_reckoning_timeout_s_(dead_reckoning_timeout_s)
{
}

Position3D EKFStateEstimator::getPosition() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return position_;
}

Velocity3D EKFStateEstimator::getVelocity() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return velocity_;
}

Quaternion EKFStateEstimator::getAttitude() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return attitude_;
}

double EKFStateEstimator::getCovarianceNorm() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return computeCovarianceNorm();
}

bool EKFStateEstimator::isDiverged() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return computeCovarianceNorm() > divergence_threshold_;
}

bool EKFStateEstimator::isDeadReckoningActive() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return dead_reckoning_active_;
}

void EKFStateEstimator::feedGPS(const GpsMeasurement& gps)
{
    std::lock_guard<std::mutex> lock(mutex_);
    last_gps_time_      = Clock::now();
    gps_ever_received_  = true;
    dead_reckoning_active_ = false;

    updateGPS(gps);
}

void EKFStateEstimator::feedIMU(const ImuMeasurement& imu)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (gps_ever_received_) {
        auto elapsed = std::chrono::duration<double>(Clock::now() - last_gps_time_).count();
        dead_reckoning_active_ = (elapsed > dead_reckoning_timeout_s_);
    }

    propagateIMU(imu);
}

void EKFStateEstimator::propagateIMU(const ImuMeasurement& imu)
{
    // Simplified constant-acceleration integration — placeholder for robot_localization EKF.
    // In production the ROS2 node wraps robot_localization::EKF which implements
    // a full 15-state UKF. This stub allows unit-testing the interface contract.
    const double dt = 0.02;  // assume 50 Hz IMU

    velocity_.vx += imu.ax * dt;
    velocity_.vy += imu.ay * dt;
    velocity_.vz += (imu.az - 9.81) * dt;

    position_.x += velocity_.vx * dt;
    position_.y += velocity_.vy * dt;
    position_.z += velocity_.vz * dt;

    // Covariance grows without GPS corrections
    if (dead_reckoning_active_) {
        for (auto& c : covariance_diag_) {
            c *= 1.001;
        }
    }
}

void EKFStateEstimator::updateGPS(const GpsMeasurement& gps)
{
    // Simplified GPS update — resets position covariance based on HDOP.
    // Production: robot_localization handles the Kalman gain calculation.
    double pos_variance = gps.hdop * gps.hdop * 4.0;  // ~2σ accuracy in meters

    position_.x = gps.lat;   // In production these are local NED coordinates
    position_.y = gps.lon;
    position_.z = gps.alt;

    covariance_diag_[0] = pos_variance;
    covariance_diag_[1] = pos_variance;
    covariance_diag_[2] = pos_variance * 2.0;
}

double EKFStateEstimator::computeCovarianceNorm() const
{
    double norm = 0.0;
    for (double c : covariance_diag_) {
        norm += c * c;
    }
    return std::sqrt(norm);
}

}  // namespace argos
