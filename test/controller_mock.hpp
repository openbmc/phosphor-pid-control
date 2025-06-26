#pragma once

#include "pid/controller.hpp"
#include "pid/pidcontroller.hpp"
#include "pid/zone_interface.hpp"

#include <gmock/gmock.h>

namespace pid_control
{

class ControllerMock : public PIDController
{
  public:
    ~ControllerMock() override = default;

    ControllerMock(const std::string& id, ZoneInterface* owner) :
        PIDController(id, owner)
    {}

    MOCK_METHOD0(inputProc, double());
    MOCK_METHOD0(setptProc, double());
    MOCK_METHOD1(outputProc, void(double));
};

} // namespace pid_control
