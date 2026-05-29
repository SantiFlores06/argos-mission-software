#pragma once

#include "IStateEstimator.hpp"
#include <mutex>
#include <chrono>

namespace argos {

struct ImuMeasurement {
    double ax{0.0}, ay{0.0}, az{0.0};
    double gx{0.0}, gy{0.0}, gz{0.0};
};

struct GpsMeasurement {
    double lat{0.0}, lon{0.0}, alt{0.0};
    double hdop{99.0};
    int    satellites{0};
};

class EKFStateEstimator : public IStateEstimator {
public:
    explicit EKFStateEstimator(double divergence_threshold = 5.0,
                               double dead_reckoning_timeout_s = 3.0);

    Position3D getPosition() const override;
    Velocity3D getVelocity() const override;
    Quaternion getAttitude() const override;
    double     getCovarianceNorm() const override;
    bool       isDiverged() const override;
    bool       isDeadReckoningActive() const override;

    void feedGPS(const GpsMeasurement& gps);
    void feedIMU(const ImuMeasurement& imu);

private:
    mutable std::mutex mutex_;

    Position3D position_;
    Velocity3D velocity_;
    Quaternion attitude_;

    // 9-element diagonal covariance: [px, py, pz, vx, vy, vz, roll, pitch, yaw]
    double covariance_diag_[9]{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

    double divergence_threshold_;
    double dead_reckoning_timeout_s_;
    bool   dead_reckoning_active_{false};

    using Clock = std::chrono::steady_clock;
    Clock::time_point last_gps_time_;
    bool              gps_ever_received_{false};

    void propagateIMU(const ImuMeasurement& imu);
    void updateGPS(const GpsMeasurement& gps);
    double computeCovarianceNorm() const;
};

}  // namespace argos
