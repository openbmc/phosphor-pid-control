// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "writeonly.hpp"

#include "interfaces.hpp"

#include <stdexcept>

namespace pid_control
{

ReadReturn WriteOnly::read(void)
{
    throw std::runtime_error("Not supported.");
}

} // namespace pid_control
