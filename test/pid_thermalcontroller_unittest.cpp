#include "conf.hpp"
#include "pid/ec/pid.hpp"
#include "pid/pidcontroller.hpp"
#include "pid/thermalcontroller.hpp"
#include "test/zone_mock.hpp"

#include <cmath>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace pid_control
{
namespace
{

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

TEST(ThermalControllerTest, BoringFactoryTest)
{
    // Verifies building a ThermalPIDController with the factory works as
    // expected in the boring (uninteresting) case.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {{"fleeting0"}};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    // Success
    EXPECT_FALSE(p == nullptr);
}

TEST(ThermalControllerTest, VerifyFactoryFailsWithZeroInputs)
{
    // A thermal controller needs at least one input.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {};
    double setpoint = 10.0;
    ec::pidinfo initial;
    std::unique_ptr<PIDController> p;
    EXPECT_THROW(
        {
            p = ThermalController::createThermalPid(
                &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
        },
        std::exception);
    EXPECT_TRUE(p == nullptr);
}

TEST(ThermalControllerTest, InputProc_BehavesAsExpected)
{
    // This test just verifies inputProc behaves as expected.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {{"fleeting0"}};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0"))).WillOnce(Return(5.0));

    EXPECT_EQ(5.0, p->inputProc());
}

TEST(ThermalControllerTest, SetPtProc_BehavesAsExpected)
{
    // This test just verifies inputProc behaves as expected.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {{"fleeting0"}};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    EXPECT_EQ(setpoint, p->setptProc());
}

TEST(ThermalControllerTest, OutputProc_BehavesAsExpected)
{
    // This test just verifies outputProc behaves as expected.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {{"fleeting0"}};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    double value = 90.0;
    EXPECT_CALL(z, addSetPoint(value, "therm1"));

    p->outputProc(value);
}

TEST(ThermalControllerTest, InputProc_MultipleInputsAbsolute)
{
    // This test verifies inputProc behaves as expected with multiple absolute
    // inputs.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {
        {"fleeting0"}, {"fleeting1"}};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::absolute);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0"))).WillOnce(Return(5.0));
    EXPECT_CALL(z, getCachedValue(StrEq("fleeting1"))).WillOnce(Return(10.0));

    EXPECT_EQ(10.0, p->inputProc());
}

TEST(ThermalControllerTest, InputProc_MultipleInputsMargin)
{
    // This test verifies inputProc behaves as expected with multiple margin
    // inputs.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {
        {"fleeting0"}, {"fleeting1"}};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0"))).WillOnce(Return(5.0));
    EXPECT_CALL(z, getCachedValue(StrEq("fleeting1"))).WillOnce(Return(10.0));

    EXPECT_EQ(5.0, p->inputProc());
}

TEST(ThermalControllerTest, InputProc_MultipleInputsSummation)
{
    // This test verifies inputProc behaves as expected with multiple summation
    // inputs.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {
        {"fleeting0"}, {"fleeting1"}};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::summation);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0"))).WillOnce(Return(5.0));
    EXPECT_CALL(z, getCachedValue(StrEq("fleeting1"))).WillOnce(Return(10.0));

    EXPECT_EQ(15.0, p->inputProc());
}

TEST(ThermalControllerTest, InputProc_MultipleInputsTempToMargin)
{
    // This test verifies inputProc behaves as expected with multiple margin
    // inputs and TempToMargin in use.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {
        {"absolute0", 85.0, true}, {"margin1"}};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("absolute0"))).WillOnce(Return(82.0));
    EXPECT_CALL(z, getCachedValue(StrEq("margin1"))).WillOnce(Return(5.0));

    // 82 degrees temp, 85 degrees Tjmax => 3 degrees of safety margin
    EXPECT_EQ(3.0, p->inputProc());
}

TEST(ThermalControllerTest, NegHysteresis_BehavesAsExpected)
{
    // This test verifies Negative hysteresis behaves as expected by
    // crossing the setpoint and noticing readings don't change until past the
    // hysteresis value

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {{"fleeting0"}};
    double setpoint = 10.0;
    ec::pidinfo initial;
    initial.checkHysterWithSetpt = false;
    initial.negativeHysteresis = 4.0;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0")))
        .Times(3)
        .WillOnce(Return(12.0))
        .WillOnce(Return(9.0))
        .WillOnce(Return(7.0));

    EXPECT_CALL(z, addSetPoint(_, "therm1")).Times(3);

    std::vector<double> lastReadings = {12.0, 12.0, 7.0};
    for (auto& reading : lastReadings)
    {
        p->process();
        EXPECT_EQ(p->getLastInput(), reading);
    }
}

TEST(ThermalControllerTest, PosHysteresis_BehavesAsExpected)
{
    // This test verifies Positive hysteresis behaves as expected by
    // crossing the setpoint and noticing readings don't change until past the
    // hysteresis value

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {{"fleeting0"}};
    double setpoint = 10.0;
    ec::pidinfo initial;
    initial.checkHysterWithSetpt = false;
    initial.positiveHysteresis = 5.0;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0")))
        .Times(3)
        .WillOnce(Return(8.0))
        .WillOnce(Return(13.0))
        .WillOnce(Return(14.0));

    EXPECT_CALL(z, addSetPoint(_, "therm1")).Times(3);

    std::vector<double> lastReadings = {8.0, 8.0, 14.0};
    for (auto& reading : lastReadings)
    {
        p->process();
        EXPECT_EQ(p->getLastInput(), reading);
    }
}

// =========================================================================
// Incremental mode hysteresis tests
// =========================================================================

TEST(ThermalControllerTest, IncrementalCheckHysterWithSetpt_ResetState)
{
    // Verifies that when input falls below (setpt - negativeHysteresis) in
    // incremental mode with checkHysterWithSetpt=true, the PID state is
    // properly reset (initialized=false, integral=0, output=0).

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {{"fleeting0"}};
    double setpoint = 10.0;
    ec::pidinfo initial;
    initial.checkHysterWithSetpt = true;
    initial.positiveHysteresis = 2.0;
    initial.negativeHysteresis = 2.0;
    initial.incrementalMode = true;
    initial.ts = 0.1;
    initial.proportionalCoeff = 1.0;
    initial.outLim.min = -100.0;
    initial.outLim.max = 100.0;
    initial.integralLimit.min = -100.0;
    initial.integralLimit.max = 100.0;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    // First call: input=15 (above setpt+posHysteresis=12), PID runs
    // Second call: input=5 (below setpt-negHysteresis=8), PID resets
    // Third call: input=15 again, PID runs fresh (initialized was reset)
    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0")))
        .Times(3)
        .WillOnce(Return(15.0))
        .WillOnce(Return(5.0))
        .WillOnce(Return(15.0));

    EXPECT_CALL(z, addSetPoint(_, "therm1")).Times(3);

    // First call: PID runs, output should be non-zero
    p->process();
    auto info = p->getPIDInfo();
    EXPECT_TRUE(info->initialized);
    EXPECT_NE(info->lastOutput, 0.0);

    // Second call: input below lower hysteresis bound, PID resets
    p->process();
    EXPECT_FALSE(info->initialized);
    EXPECT_EQ(info->integral, 0.0);
    EXPECT_EQ(info->lastOutput, 0.0);

    // Third call: PID runs fresh from reset state
    p->process();
    EXPECT_TRUE(info->initialized);
}

TEST(ThermalControllerTest, IncrementalCheckHysterWithSetpt_HoldOutput)
{
    // Verifies that when input is inside hysteresis bounds in incremental
    // mode with checkHysterWithSetpt=true, the output is held and the
    // incremental state (lastInput, lastLastInput) is kept current.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {{"fleeting0"}};
    double setpoint = 10.0;
    ec::pidinfo initial;
    initial.checkHysterWithSetpt = true;
    initial.positiveHysteresis = 3.0;
    initial.negativeHysteresis = 3.0;
    initial.incrementalMode = true;
    initial.ts = 0.1;
    initial.proportionalCoeff = 1.0;
    initial.outLim.min = -100.0;
    initial.outLim.max = 100.0;
    initial.integralLimit.min = -100.0;
    initial.integralLimit.max = 100.0;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    // First call: input=15 (above 13), PID runs and produces output
    // Second call: input=11 (inside [7,13]), output held, state updated
    // Third call: input=15 (above 13), PID runs again, no spike from stale state
    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0")))
        .Times(3)
        .WillOnce(Return(15.0))
        .WillOnce(Return(11.0))
        .WillOnce(Return(15.0));

    EXPECT_CALL(z, addSetPoint(_, "therm1")).Times(3);

    // First call: PID runs
    p->process();
    auto info = p->getPIDInfo();
    double firstOutput = info->lastOutput;
    EXPECT_TRUE(info->initialized);

    // Second call: inside hysteresis, output held
    p->process();
    EXPECT_EQ(info->lastOutput, firstOutput);
    // State should be updated to current input to avoid stale history
    EXPECT_DOUBLE_EQ(info->lastInput, 11.0);

    // Third call: PID runs again, should produce finite output
    p->process();
    EXPECT_TRUE(std::isfinite(info->lastOutput));
}

TEST(ThermalControllerTest, IncrementalNegHysteresis_BehavesAsExpected)
{
    // Same as NegHysteresis_BehavesAsExpected but with incrementalMode=true.
    // Verifies hysteresis behavior is unchanged in incremental mode when
    // checkHysterWithSetpt=false.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {{"fleeting0"}};
    double setpoint = 10.0;
    ec::pidinfo initial;
    initial.checkHysterWithSetpt = false;
    initial.negativeHysteresis = 4.0;
    initial.incrementalMode = true;
    initial.ts = 0.1;
    initial.proportionalCoeff = 1.0;
    initial.outLim.min = -100.0;
    initial.outLim.max = 100.0;
    initial.integralLimit.min = -100.0;
    initial.integralLimit.max = 100.0;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0")))
        .Times(3)
        .WillOnce(Return(12.0))
        .WillOnce(Return(9.0))
        .WillOnce(Return(7.0));

    EXPECT_CALL(z, addSetPoint(_, "therm1")).Times(3);

    std::vector<double> lastReadings = {12.0, 12.0, 7.0};
    for (auto& reading : lastReadings)
    {
        p->process();
        EXPECT_EQ(p->getLastInput(), reading);
    }
}

TEST(ThermalControllerTest, IncrementalPosHysteresis_BehavesAsExpected)
{
    // Same as PosHysteresis_BehavesAsExpected but with incrementalMode=true.
    // Verifies hysteresis behavior is unchanged in incremental mode when
    // checkHysterWithSetpt=false.

    ZoneMock z;

    std::vector<pid_control::conf::SensorInput> inputs = {{"fleeting0"}};
    double setpoint = 10.0;
    ec::pidinfo initial;
    initial.checkHysterWithSetpt = false;
    initial.positiveHysteresis = 5.0;
    initial.incrementalMode = true;
    initial.ts = 0.1;
    initial.proportionalCoeff = 1.0;
    initial.outLim.min = -100.0;
    initial.outLim.max = 100.0;
    initial.integralLimit.min = -100.0;
    initial.integralLimit.max = 100.0;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0")))
        .Times(3)
        .WillOnce(Return(8.0))
        .WillOnce(Return(13.0))
        .WillOnce(Return(14.0));

    EXPECT_CALL(z, addSetPoint(_, "therm1")).Times(3);

    std::vector<double> lastReadings = {8.0, 8.0, 14.0};
    for (auto& reading : lastReadings)
    {
        p->process();
        EXPECT_EQ(p->getLastInput(), reading);
    }
}

} // namespace
} // namespace pid_control
