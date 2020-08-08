#pragma once

#include "interfaces.hpp"

#include <gmock/gmock.h>

namespace pid_control
{

class ReadInterfaceMock : public ReadInterface
{
  public:
    virtual ~ReadInterfaceMock() = default;

    MOCK_METHOD0(read, ReadReturn());
};

} // namespace pid_control
