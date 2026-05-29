#pragma once

#include "IAuthorityManager.hpp"
#include <map>
#include <mutex>

namespace argos {

class AuthorityManagerImpl : public IAuthorityManager {
public:
    AuthorityManagerImpl();

    bool          requestAuthority(CommandSource source, uint8_t priority) override;
    void          releaseAuthority(CommandSource source) override;
    CommandSource getCurrentAuthority() const override;
    bool          isAuthorityGranted(CommandSource source) const override;

private:
    mutable std::mutex              mutex_;
    CommandSource                   current_authority_{CommandSource::NONE};
    uint8_t                         current_priority_{0};
    std::map<CommandSource, uint8_t> priority_table_;

    bool resolveConflict(CommandSource challenger, uint8_t challenger_priority);
};

}  // namespace argos
