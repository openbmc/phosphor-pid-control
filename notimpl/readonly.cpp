// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "readonly.hpp"

#include <stdexcept>

namespace pid_control
{

void ReadOnly::write([[maybe_unused]] double value)
{
    throw std::runtime_error("Not supported.");
}

void ReadOnlyNoExcept::write([[maybe_unused]] double value)
{
    return;
}

} // namespace pid_control
