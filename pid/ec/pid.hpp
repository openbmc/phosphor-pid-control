#pragma once

#include <pid_common.h>

#include <cstdint>
#include <string>

namespace pid_control
{
namespace ec
{

double pid(pid_info_t* pidinfoptr, double input, double setpoint,
           const std::string* nameptr = nullptr);

} // namespace ec
} // namespace pid_control
