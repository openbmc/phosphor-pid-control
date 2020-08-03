#pragma once

#include <cstdint>

namespace pid_control
{
namespace ipmi
{

enum ManualSubCmd
{
    GET_CONTROL_STATE = 0,
    SET_CONTROL_STATE = 1,
    GET_FAILSAFE_STATE = 2,
};

struct FanCtrlRequest
{
    uint8_t command;
    uint8_t zone;
} __attribute__((packed));

struct FanCtrlRequestSet
{
    uint8_t command;
    uint8_t zone;
    uint8_t value;
} __attribute__((packed));

} // namespace ipmi
} // namespace pid_control
