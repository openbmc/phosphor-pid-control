#pragma once

#include "controller.hpp"
#include "ec/stepwise.hpp"
#include "fan.hpp"

#include <limits>
#include <memory>
#include <vector>

namespace pid_control
{

class ZoneInterface;

class StepwiseController : public Controller
{
  public:
    static std::unique_ptr<Controller>
        createStepwiseController(ZoneInterface* owner, const std::string& id,
                                 const std::vector<std::string>& inputs,
                                 const ec::StepwiseInfo& initial);

    StepwiseController(const std::string& id,
                       const std::vector<std::string>& inputs,
                       ZoneInterface* owner) :
        Controller(),
        _owner(owner), _id(id), _inputs(inputs)
    {}

    double inputProc(void) override;

    void outputProc(double value) override;

    void process(void) override;

    std::string getID(void) override
    {
        return _id;
    }

    ec::StepwiseInfo& getStepwiseInfo(void)
    {
        return _stepwiseInfo;
    }

  protected:
    ZoneInterface* _owner;

  private:
    // parameters
    ec::StepwiseInfo _stepwiseInfo;
    std::string _id;
    std::vector<std::string> _inputs;
    double _lastInput = std::numeric_limits<double>::quiet_NaN();
    double _lastOutput = std::numeric_limits<double>::quiet_NaN();
};

} // namespace pid_control
