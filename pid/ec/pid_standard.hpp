#pragma once

#include <cstdint>
#include <string>

// For pid_info_t struct
#include "pid_standard.hpp"

namespace pid_control
{
namespace ec
{

double pid_standard(pid_info_t* pidinfoptr, double input, double setpoint,
                    const std::string* nameptr = nullptr);

} // namespace ec
} // namespace pid_control
