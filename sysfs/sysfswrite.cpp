// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "sysfswrite.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>

namespace pid_control
{

void SysFsWritePercent::write(double value)
{
    double minimum = getMin();
    double maximum = getMax();

    double range = maximum - minimum;
    double offset = range * value;
    double ovalue = offset + minimum;

    std::ofstream ofs;
    ofs.open(_writePath);
    ofs << static_cast<int64_t>(ovalue);
    ofs.close();

    return;
}

void SysFsWrite::write(double value)
{
    std::ofstream ofs;
    ofs.open(_writePath);
    ofs << static_cast<int64_t>(value);
    ofs.close();

    return;
}

} // namespace pid_control
