#include "pid/fancontroller.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "pid/ec/pid.hpp"
#include "test/sensor_mock.hpp"
#include "test/zone_mock.hpp"

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrEq;

TEST(FanControllerTest, BoringFactoryTest) {
    // Always use the factory! But don't enforce it. :P

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::CreateFanPid(&z, "fan1", inputs, initial);
    // Success
    EXPECT_FALSE(p == nullptr);
}

TEST(FanControllerTest, ZeroInputTest) {
    // A fan controller needs at least one input.

    ZoneMock z;

    std::vector<std::string> inputs = {};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::CreateFanPid(&z, "fan1", inputs, initial);
    EXPECT_TRUE(p == nullptr);
}

TEST(FanControllerTest, InputProcZeroValidSensors) {
    // If all your inputs are 0 or less, return 0.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::CreateFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fan0"))).WillOnce(Return(0));
    EXPECT_CALL(z, getCachedValue(StrEq("fan1"))).WillOnce(Return(0));

    EXPECT_EQ(0.0, p->input_proc());
}

TEST(FanControllerTest, InputProcChooseMin) {
    // Verify it selects the minimum value from its inputs.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1", "fan2"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::CreateFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fan0"))).WillOnce(Return(10.0));
    EXPECT_CALL(z, getCachedValue(StrEq("fan1"))).WillOnce(Return(30.0));
    EXPECT_CALL(z, getCachedValue(StrEq("fan2"))).WillOnce(Return(5.0));

    EXPECT_EQ(5.0, p->input_proc());
}

// The direction is unused presently, but these tests validate the logic.
TEST(FanControllerTest, SetPtProcSpeedChange) {
    // The set-point starts at 0, so anything larger, then smaller, then equal.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::CreateFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);
    FanController *fp = reinterpret_cast<FanController*>(p.get());

    EXPECT_EQ(FanSpeedDirection::NEUTRAL, fp->getFanDirection());

    EXPECT_CALL(z, getMaxRPMRequest()).WillOnce(Return(10.0));
    EXPECT_EQ(10.0, p->setpt_proc());
    EXPECT_EQ(FanSpeedDirection::UP, fp->getFanDirection());

    EXPECT_CALL(z, getMaxRPMRequest()).WillOnce(Return(5.0));
    EXPECT_EQ(5.0, p->setpt_proc());
    EXPECT_EQ(FanSpeedDirection::DOWN, fp->getFanDirection());

    EXPECT_CALL(z, getMaxRPMRequest()).WillOnce(Return(5.0));
    EXPECT_EQ(5.0, p->setpt_proc());
    EXPECT_EQ(FanSpeedDirection::NEUTRAL, fp->getFanDirection());
}

TEST(FanControllerTest, VerifyFailSafeOutputProc) {
    // Verify if failsafe mode is enabled, the input is ignored.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::CreateFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getFailSafeMode()).WillOnce(Return(true));
    EXPECT_CALL(z, getFailSafePercent()).Times(2).WillRepeatedly(Return(75.0));

    int64_t timeout = 0;
    std::unique_ptr<Sensor> s1 = std::make_unique<SensorMock>("fan0", timeout);
    std::unique_ptr<Sensor> s2 = std::make_unique<SensorMock>("fan1", timeout);
    SensorMock *sm1 = reinterpret_cast<SensorMock*>(s1.get());
    SensorMock *sm2 = reinterpret_cast<SensorMock*>(s2.get());

    EXPECT_CALL(z, getSensor(StrEq("fan0"))).WillOnce(ReturnRef(s1));
    EXPECT_CALL(*sm1, write(0.75));
    EXPECT_CALL(z, getSensor(StrEq("fan1"))).WillOnce(ReturnRef(s2));
    EXPECT_CALL(*sm2, write(0.75));

    // Setting 50%, will end up being 75%.
    p->output_proc(50.0);
}

TEST(FanControllerTest, VerifyFailSafeOutputIgnored) {
    // If the requested output is higher than the failsafe value, then use the
    // specified value.


}

TEST(FanControllerTest, VerifyOutputProc) {
    // Verify the input is correctly interpreted.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::CreateFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getFailSafeMode()).WillOnce(Return(false));

    int64_t timeout = 0;
    std::unique_ptr<Sensor> s1 = std::make_unique<SensorMock>("fan0", timeout);
    std::unique_ptr<Sensor> s2 = std::make_unique<SensorMock>("fan1", timeout);
    SensorMock *sm1 = reinterpret_cast<SensorMock*>(s1.get());
    SensorMock *sm2 = reinterpret_cast<SensorMock*>(s2.get());

    EXPECT_CALL(z, getSensor(StrEq("fan0"))).WillOnce(ReturnRef(s1));
    EXPECT_CALL(*sm1, write(0.5));
    EXPECT_CALL(z, getSensor(StrEq("fan1"))).WillOnce(ReturnRef(s2));
    EXPECT_CALL(*sm2, write(0.5));

    p->output_proc(50.0);
}

