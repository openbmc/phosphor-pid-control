#pragma once

#include <ipmid/api.h>

#include <cstdint>

namespace pid_control
{
namespace ipmi
{

ipmi_ret_t manualModeControl(ipmi_cmd_t cmd, const uint8_t* reqBuf,
                             uint8_t* replyCmdBuf, size_t* dataLen);

} // namespace ipmi
} // namespace pid_control
