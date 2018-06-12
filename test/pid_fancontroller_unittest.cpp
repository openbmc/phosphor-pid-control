#include "pid/fancontroller.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "pid/ec/pid.hpp"
#include "test/zone_mock.hpp"

using ::testing::Return;
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
}

TEST(FanControllerTest, VerifyOutputProc) {
    // Verify the input is correctly interpreted.
}

