#include "failsafe_logger_utility.hpp"

#include "failsafeloggers/failsafe_logger.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

std::unordered_map<int64_t, std::shared_ptr<pid_control::FailsafeLogger>>
    zoneIdToFailsafeLogger =
        std::unordered_map<int64_t,
                           std::shared_ptr<pid_control::FailsafeLogger>>();

std::unordered_map<std::string, std::vector<int64_t>> sensorNameToZoneId =
    std::unordered_map<std::string, std::vector<int64_t>>();
