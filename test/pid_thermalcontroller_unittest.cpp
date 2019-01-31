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
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
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

    std::vector<std::string> inputs = {"fleeting0"};
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

    std::vector<std::string> inputs = {"fleeting0"};
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

    std::vector<std::string> inputs = {"fleeting0"};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    double value = 90.0;
    EXPECT_CALL(z, addRPMSetPoint(value));

    p->outputProc(value);
}

TEST(ThermalControllerTest, InputProc_MultipleInputsAbsolute)
{
    // This test verifies inputProc behaves as expected with multiple absolute
    // inputs.

    ZoneMock z;

    std::vector<std::string> inputs = {"fleeting0", "fleeting1"};
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

    std::vector<std::string> inputs = {"fleeting0", "fleeting1"};
    double setpoint = 10.0;
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0"))).WillOnce(Return(5.0));
    EXPECT_CALL(z, getCachedValue(StrEq("fleeting1"))).WillOnce(Return(10.0));

    EXPECT_EQ(5.0, p->inputProc());
}

TEST(ThermalControllerTest, NegHysteresis_BehavesAsExpected)
{

    // This test verifies Negative hysteresis behaves as expected by
    // crossing the setpoint and noticing readings don't change until past the
    // hysteresis value

    ZoneMock z;

    std::vector<std::string> inputs = {"fleeting0"};
    double setpoint = 10.0;
    ec::pidinfo initial;
    initial.negativeHysteresis = 4.0;
    initial.p_c = -500.0;
    initial.out_lim.max = 10000;
    initial.slew_neg = 0;
    initial.slew_pos = 0;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    // get two readings that are different, in reality we don't care what the
    // actual numbers are
    double readingAbove = ec::pid(p->getPIDInfo(), 12, setpoint);
    double readingBelow = ec::pid(p->getPIDInfo(), 7, setpoint);

    EXPECT_NE(readingBelow, readingAbove);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0")))
        .Times(3)
        .WillOnce(Return(12.0))
        .WillOnce(Return(8.0))
        .WillOnce(Return(7.0));

    EXPECT_CALL(z, addRPMSetPoint(readingAbove)).Times(2);
    EXPECT_CALL(z, addRPMSetPoint(readingBelow)).Times(1);

    for (int ii = 0; ii < 3; ii++)
    {
        p->process();
    }
}

TEST(ThermalControllerTest, PosHysteresis_BehavesAsExpected)
{
    // This test verifies Positive hysteresis behaves as expected by
    // crossing the setpoint and noticing readings don't change until past the
    // hysteresis value

    ZoneMock z;

    std::vector<std::string> inputs = {"fleeting0"};
    double setpoint = 10.0;
    ec::pidinfo initial;
    initial.positiveHysteresis = 5.0;
    initial.p_c = -500.0;
    initial.out_lim.max = 10000;
    initial.slew_neg = 0;
    initial.slew_pos = 0;

    std::unique_ptr<PIDController> p = ThermalController::createThermalPid(
        &z, "therm1", inputs, setpoint, initial, ThermalType::margin);
    EXPECT_FALSE(p == nullptr);

    // get two readings that are different, in reality we don't care what the
    // actual numbers are
    double readingBelow = ec::pid(p->getPIDInfo(), 8, setpoint);
    double readingAbove = ec::pid(p->getPIDInfo(), 14, setpoint);

    EXPECT_NE(readingBelow, readingAbove);

    EXPECT_CALL(z, getCachedValue(StrEq("fleeting0")))
        .Times(3)
        .WillOnce(Return(8.0))
        .WillOnce(Return(13.0))
        .WillOnce(Return(14.0));

    EXPECT_CALL(z, addRPMSetPoint(readingBelow)).Times(2);
    EXPECT_CALL(z, addRPMSetPoint(readingAbove)).Times(1);

    for (int ii = 0; ii < 3; ii++)
    {
        p->process();
    }
}