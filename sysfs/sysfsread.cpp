// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "sysfs/sysfsread.hpp"

#include "interfaces.hpp"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>

namespace pid_control
{

ReadReturn SysFsRead::read(void)
{
    int64_t value;
    std::ifstream ifs;

    ifs.open(_path);
    ifs >> value;
    ifs.close();

    ReadReturn r = {static_cast<double>(value),
                    std::chrono::high_resolution_clock::now()};

    return r;
}

} // namespace pid_control
