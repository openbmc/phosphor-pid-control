/**
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "manualcmds.hpp"

#include "control.hpp"
#include "dbus_mode.hpp"
#include "manual_messages.hpp"

#include <ipmid/api.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <variant>

namespace pid_control
{
namespace ipmi
{

static constexpr auto manualProperty = "Manual";
static constexpr auto failsafeProperty = "FailSafe";

ipmi_ret_t ZoneControlIpmiHandler::getFailsafeModeState(
    const uint8_t* reqBuf, uint8_t* replyBuf, size_t* dataLen)
{
    bool current;

    if (*dataLen < sizeof(struct FanCtrlRequest))
    {
        return IPMI_CC_INVALID;
    }

    const auto request =
        reinterpret_cast<const struct FanCtrlRequest*>(&reqBuf[0]);

    ipmi_ret_t rc =
        _control->getFanCtrlProperty(request->zone, &current, failsafeProperty);
    if (rc)
    {
        return rc;
    }

    *replyBuf = (uint8_t)current;
    *dataLen = sizeof(uint8_t);
    return IPMI_CC_OK;
}

/*
 * <method name="GetAll">
 *   <arg name="interface" direction="in" type="s"/>
 *   <arg name="properties" direction="out" type="a{sv}"/>
 * </method>
 */
ipmi_ret_t ZoneControlIpmiHandler::getManualModeState(
    const uint8_t* reqBuf, uint8_t* replyBuf, size_t* dataLen)
{
    bool current;

    if (*dataLen < sizeof(struct FanCtrlRequest))
    {
        return IPMI_CC_INVALID;
    }

    const auto request =
        reinterpret_cast<const struct FanCtrlRequest*>(&reqBuf[0]);

    ipmi_ret_t rc =
        _control->getFanCtrlProperty(request->zone, &current, manualProperty);
    if (rc)
    {
        return rc;
    }

    *replyBuf = (uint8_t)current;
    *dataLen = sizeof(uint8_t);
    return IPMI_CC_OK;
}

/*
 * <method name="Set">
 *   <arg name="interface" direction="in" type="s"/>
 *   <arg name="property" direction="in" type="s"/>
 *   <arg name="value" direction="in" type="v"/>
 * </method>
 */
ipmi_ret_t ZoneControlIpmiHandler::setManualModeState(
    const uint8_t* reqBuf, [[maybe_unused]] uint8_t* replyBuf,
    const size_t* dataLen)
{
    if (*dataLen < sizeof(struct FanCtrlRequestSet))
    {
        return IPMI_CC_INVALID;
    }

    const auto request =
        reinterpret_cast<const struct FanCtrlRequestSet*>(&reqBuf[0]);

    /* 0 is false, 1 is true */
    ipmi_ret_t rc = _control->setFanCtrlProperty(
        request->zone, static_cast<bool>(request->value), manualProperty);
    return rc;
}

/* Three command packages: get, set true, set false */
ipmi_ret_t manualModeControl(
    ZoneControlIpmiHandler* handler, [[maybe_unused]] ipmi_cmd_t cmd,
    const uint8_t* reqBuf, uint8_t* replyCmdBuf, size_t* dataLen)
{
    // FanCtrlRequest is the smaller of the requests, so it's at a minimum.
    if (*dataLen < sizeof(struct FanCtrlRequest))
    {
        return IPMI_CC_INVALID;
    }

    const auto request =
        reinterpret_cast<const struct FanCtrlRequest*>(&reqBuf[0]);

    ipmi_ret_t rc = IPMI_CC_OK;

    switch (request->command)
    {
        case getControlState:
            return handler->getManualModeState(reqBuf, replyCmdBuf, dataLen);
        case setControlState:
            return handler->setManualModeState(reqBuf, replyCmdBuf, dataLen);
        case getFailsafeState:
            return handler->getFailsafeModeState(reqBuf, replyCmdBuf, dataLen);
        default:
            rc = IPMI_CC_INVALID;
    }

    return rc;
}

} // namespace ipmi
} // namespace pid_control
