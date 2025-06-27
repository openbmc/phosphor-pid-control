#pragma once

#include "control.hpp"

#include <ipmid/api.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

namespace pid_control
{
namespace ipmi
{

// Implements validation of IPMI commands and handles sending back the
// responses.
class ZoneControlIpmiHandler
{
  public:
    explicit ZoneControlIpmiHandler(
        std::unique_ptr<ZoneControlInterface> control) :
        _control(std::move(control))
    {}

    ipmi_ret_t getFailsafeModeState(const uint8_t* reqBuf, uint8_t* replyBuf,
                                    size_t* dataLen);

    ipmi_ret_t getManualModeState(const uint8_t* reqBuf, uint8_t* replyBuf,
                                  size_t* dataLen);

    ipmi_ret_t setManualModeState(const uint8_t* reqBuf, uint8_t* replyBuf,
                                  const size_t* dataLen);

  private:
    std::unique_ptr<ZoneControlInterface> _control;
};

ipmi_ret_t manualModeControl(ZoneControlIpmiHandler* handler, ipmi_cmd_t cmd,
                             const uint8_t* reqBuf, uint8_t* replyCmdBuf,
                             size_t* dataLen);

} // namespace ipmi
} // namespace pid_control
