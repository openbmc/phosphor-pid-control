#include "failsafe_logger_utility.hpp"

#include "failsafeloggers/failsafe_logger.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>

std::unordered_map<int64_t, std::shared_ptr<pid_control::FailsafeLogger>>
    zoneIdToFailsafeLogger =
        std::unordered_map<int64_t,
                           std::shared_ptr<pid_control::FailsafeLogger>>();
