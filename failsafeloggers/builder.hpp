#pragma once

#include "conf.hpp"
#include "failsafeloggers/failsafe_logger.hpp"
#include "pid/zone_interface.hpp"
#include "sensors/manager.hpp"

#include <memory>
#include <unordered_map>

namespace pid_control
{

void buildFailsafeLoggers(const std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>>& zones,
                          const int logMaxCountPerSecond = 20);

}  // namespace pid_control
