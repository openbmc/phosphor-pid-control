#pragma once

#include <cstdint>

namespace pid_control::ipmi
{

enum ManualSubCmd
{
    getControlState = 0,
    setControlState = 1,
    getFailsafeState = 2,
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

} // namespace pid_control::ipmi
