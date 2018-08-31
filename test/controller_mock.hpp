#pragma once

#include "pid/controller.hpp"

#include <gmock/gmock.h>

class ControllerMock : public PIDController
{
  public:
    virtual ~ControllerMock() = default;

    ControllerMock(const std::string& id, PIDZone* owner) :
        PIDController(id, owner)
    {
    }

    MOCK_METHOD0(input_proc, float());
    MOCK_METHOD0(setpt_proc, float());
    MOCK_METHOD1(output_proc, void(float));
};
