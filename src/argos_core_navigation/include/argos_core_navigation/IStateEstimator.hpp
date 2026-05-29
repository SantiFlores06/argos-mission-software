#pragma once

namespace argos {

struct Position3D {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

struct Velocity3D {
    double vx{0.0};
    double vy{0.0};
    double vz{0.0};
};

struct Quaternion {
    double w{1.0};
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

class IStateEstimator {
public:
    virtual ~IStateEstimator() = default;

    virtual Position3D getPosition() const = 0;
    virtual Velocity3D getVelocity() const = 0;
    virtual Quaternion getAttitude() const = 0;
    virtual double     getCovarianceNorm() const = 0;
    virtual bool       isDiverged() const = 0;
    virtual bool       isDeadReckoningActive() const = 0;
};

}  // namespace argos
