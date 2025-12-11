// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "pluggable.hpp"

#include "hoststatemonitor.hpp"
#include "interfaces.hpp"

#include <cstdint>
#include <string>

namespace pid_control
{

ReadReturn PluggableSensor::read(void)
{
    return _reader->read();
}

void PluggableSensor::write(double value)
{
    _writer->write(value);
}

void PluggableSensor::write(double value, bool force, int64_t* written)
{
    _writer->write(value, force, written);
}

bool PluggableSensor::getFailed(void)
{
    bool isFailed = _reader->getFailed();

    if (isFailed && getIgnoreFailIfHostOff())
    {
        auto& hostState = HostStateMonitor::getInstance();
        if (!hostState.isPowerOn())
        {
            return false;
        }
    }

    return isFailed;
}

FailureReason PluggableSensor::getFailReason(void)
{
    return _reader->getFailReason();
}

} // namespace pid_control
