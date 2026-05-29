#include "argos_core_authority/AuthorityManagerImpl.hpp"

namespace argos {

AuthorityManagerImpl::AuthorityManagerImpl()
{
    // Default static priorities — GCS_OVERRIDE always wins, NONE never holds authority.
    priority_table_[CommandSource::GCS_OVERRIDE]         = 255;
    priority_table_[CommandSource::RECOVERY_MANAGER]     = 200;
    priority_table_[CommandSource::FLIGHT_LIFECYCLE_FSM] = 100;
    priority_table_[CommandSource::BEHAVIOR_TREE]        = 50;
    priority_table_[CommandSource::NONE]                 = 0;
}

bool AuthorityManagerImpl::requestAuthority(CommandSource source, uint8_t priority)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (current_authority_ == CommandSource::NONE) {
        current_authority_ = source;
        current_priority_  = priority;
        return true;
    }

    if (current_authority_ == source) {
        current_priority_ = priority;
        return true;
    }

    return resolveConflict(source, priority);
}

void AuthorityManagerImpl::releaseAuthority(CommandSource source)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_authority_ == source) {
        current_authority_ = CommandSource::NONE;
        current_priority_  = 0;
    }
}

CommandSource AuthorityManagerImpl::getCurrentAuthority() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return current_authority_;
}

bool AuthorityManagerImpl::isAuthorityGranted(CommandSource source) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return current_authority_ == source;
}

bool AuthorityManagerImpl::resolveConflict(CommandSource challenger, uint8_t challenger_priority)
{
    // Static table priority takes precedence over the runtime priority argument.
    // This prevents a misbehaving node from escalating itself above GCS_OVERRIDE.
    auto it_challenger = priority_table_.find(challenger);
    auto it_holder     = priority_table_.find(current_authority_);

    uint8_t static_challenger = (it_challenger != priority_table_.end()) ? it_challenger->second : challenger_priority;
    uint8_t static_holder     = (it_holder != priority_table_.end()) ? it_holder->second : current_priority_;

    if (static_challenger > static_holder) {
        current_authority_ = challenger;
        current_priority_  = challenger_priority;
        return true;
    }

    return false;
}

}  // namespace argos
