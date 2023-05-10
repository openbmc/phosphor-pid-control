/* Interface that implements an exception throwing write method. */
#pragma once

#include "interfaces.hpp"

namespace pid_control
{

class WriteOnly : public ReadInterface
{
  public:
    WriteOnly() : ReadInterface() {}

    ReadReturn read(void) override;
};

} // namespace pid_control
