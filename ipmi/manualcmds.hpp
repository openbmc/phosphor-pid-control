#pragma once

#include "control.hpp"

#include <ipmid/api-types.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

namespace pid_control_ipmi
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

    ipmi::Cc getFailsafeModeState(const uint8_t* reqBuf, uint8_t* replyBuf,
                                  size_t* dataLen);

    ipmi::Cc getManualModeState(const uint8_t* reqBuf, uint8_t* replyBuf,
                                size_t* dataLen);

    ipmi::Cc setManualModeState(const uint8_t* reqBuf, uint8_t* replyBuf,
                                const size_t* dataLen);

  private:
    std::unique_ptr<ZoneControlInterface> _control;
};

ipmi::Cc manualModeControl(ZoneControlIpmiHandler* handler, uint8_t cmd,
                           const uint8_t* reqBuf, uint8_t* replyCmdBuf,
                           size_t* dataLen);

} // namespace pid_control_ipmi
