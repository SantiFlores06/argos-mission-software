#pragma once

#include <cstdint>

namespace argos {

enum class CommandSource : uint8_t {
    NONE                 = 0,
    FLIGHT_LIFECYCLE_FSM = 1,
    BEHAVIOR_TREE        = 2,
    RECOVERY_MANAGER     = 3,
    GCS_OVERRIDE         = 4,
};

class IAuthorityManager {
public:
    virtual ~IAuthorityManager() = default;

    virtual bool requestAuthority(CommandSource source, uint8_t priority) = 0;
    virtual void releaseAuthority(CommandSource source) = 0;
    virtual CommandSource getCurrentAuthority() const = 0;
    virtual bool isAuthorityGranted(CommandSource source) const = 0;
};

}  // namespace argos
