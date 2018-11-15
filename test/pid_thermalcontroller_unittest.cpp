#include "pid/ec/pid.hpp"
#include "pid/thermalcontroller.hpp"
#include "test/zone_mock.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::StrEq;

TEST(ThermalControllerTest, BoringFactoryTest)
{
    // Verifies building a ThermalPIDController with the factory works as
    // expected in the boring (uninteresting) case.

    ZoneMock z;

    std::vector<std::string> inputs = {"fleeting0"};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial);
    // Success
    EXPECT_FALSE(p == nullptr);
}

TEST(ThermalControllerTest, VerifyFactoryFailsWithZeroInputs)
{
    // A thermal controller needs at least one input.

    ZoneMock z;

    std::vector<std::string> inputs = {};
    double setpoint = 10.0;
    ec::pidinfo initial;
    std::unique_ptr<PIDController> p;
    EXPECT_THROW(
        {
            p = ThermalController::createThermalPid(&z, "therm1", inputs,
                                                    setpoint, initial);
        },
        std::exception);
    EXPECT_TRUE(p == nullptr);
}

TEST(ThermalControllerTest, InputProc_BehavesAsExpected)
{
    // This test just verifies inputProc behaves as expected.

    ZoneMock z;

    std::vector<std::string> inputs = {"fleeting0"};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0"))).WillOnce(Return(5.0));

    EXPECT_EQ(5.0, p->inputProc());
}

TEST(ThermalControllerTest, SetPtProc_BehavesAsExpected)
{
    // This test just verifies inputProc behaves as expected.

    ZoneMock z;

    std::vector<std::string> inputs = {"fleeting0"};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial);
    EXPECT_FALSE(p == nullptr);

    EXPECT_EQ(setpoint, p->setptProc());
}

TEST(ThermalControllerTest, OutputProc_BehavesAsExpected)
{
    // This test just verifies inputProc behaves as expected.

    ZoneMock z;

    std::vector<std::string> inputs = {"fleeting0"};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial);
    EXPECT_FALSE(p == nullptr);

    double value = 90.0;
    EXPECT_CALL(z, addRPMSetPoint(value));

    p->outputProc(value);
}
