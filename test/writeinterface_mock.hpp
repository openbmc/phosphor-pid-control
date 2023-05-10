#pragma once

#include "interfaces.hpp"

#include <gmock/gmock.h>

namespace pid_control
{

class WriteInterfaceMock : public WriteInterface
{
  public:
    virtual ~WriteInterfaceMock() = default;

    WriteInterfaceMock(int64_t min, int64_t max) : WriteInterface(min, max) {}

    MOCK_METHOD1(write, void(double));
};

} // namespace pid_control
