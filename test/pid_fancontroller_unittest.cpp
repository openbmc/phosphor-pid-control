#include "pid/ec/pid.hpp"
#include "pid/fancontroller.hpp"
#include "test/sensor_mock.hpp"
#include "test/zone_mock.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace pid_control
{
namespace
{

using ::testing::_;
using ::testing::DoubleEq;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrEq;

TEST(FanControllerTest, BoringFactoryTest)
{
    // Verify the factory will properly build the FanPIDController in the
    // boring (uninteresting) case.
    ZoneMock z;

    std::vector<std::string> inputs = {"fan0"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    // Success
    EXPECT_FALSE(p == nullptr);
}

TEST(FanControllerTest, VerifyFactoryFailsWithZeroInputs)
{
    // A fan controller needs at least one input.

    ZoneMock z;

    std::vector<std::string> inputs = {};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    EXPECT_TRUE(p == nullptr);
}

TEST(FanControllerTest, InputProc_AllSensorsReturnZero)
{
    // If all your inputs are 0, return 0.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fan0"))).WillOnce(Return(0));
    EXPECT_CALL(z, getCachedValue(StrEq("fan1"))).WillOnce(Return(0));

    EXPECT_EQ(0.0, p->inputProc());
}

TEST(FanControllerTest, InputProc_IfSensorNegativeIsIgnored)
{
    // A sensor value returning sub-zero is ignored as an error.
    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fan0"))).WillOnce(Return(-1));
    EXPECT_CALL(z, getCachedValue(StrEq("fan1"))).WillOnce(Return(-1));

    EXPECT_EQ(0.0, p->inputProc());
}

TEST(FanControllerTest, InputProc_ChoosesMinimumValue)
{
    // Verify it selects the minimum value from its inputs.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1", "fan2"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getCachedValue(StrEq("fan0"))).WillOnce(Return(10.0));
    EXPECT_CALL(z, getCachedValue(StrEq("fan1"))).WillOnce(Return(30.0));
    EXPECT_CALL(z, getCachedValue(StrEq("fan2"))).WillOnce(Return(5.0));

    EXPECT_EQ(5.0, p->inputProc());
}

// The direction is unused presently, but these tests validate the logic.
TEST(FanControllerTest, SetPtProc_SpeedChanges_VerifyDirection)
{
    // The fan direction defaults to neutral, because we have no data.  Verify
    // that after this point it appropriately will indicate speeding up or
    // slowing down based on the RPM values specified.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);
    // Grab pointer for mocking.
    FanController* fp = reinterpret_cast<FanController*>(p.get());

    // Fanspeed starts are Neutral.
    EXPECT_EQ(FanSpeedDirection::NEUTRAL, fp->getFanDirection());

    // getMaxSetPointRequest returns a higher value than 0, so the fans should
    // be marked as speeding up.
    EXPECT_CALL(z, getMaxSetPointRequest()).WillOnce(Return(10.0));
    EXPECT_EQ(10.0, p->setptProc());
    EXPECT_EQ(FanSpeedDirection::UP, fp->getFanDirection());

    // getMaxSetPointRequest returns a lower value than 10, so the fans should
    // be marked as slowing down.
    EXPECT_CALL(z, getMaxSetPointRequest()).WillOnce(Return(5.0));
    EXPECT_EQ(5.0, p->setptProc());
    EXPECT_EQ(FanSpeedDirection::DOWN, fp->getFanDirection());

    // getMaxSetPointRequest returns the same value, so the fans should be
    // marked as neutral.
    EXPECT_CALL(z, getMaxSetPointRequest()).WillOnce(Return(5.0));
    EXPECT_EQ(5.0, p->setptProc());
    EXPECT_EQ(FanSpeedDirection::NEUTRAL, fp->getFanDirection());
}

TEST(FanControllerTest, OutputProc_VerifiesIfFailsafeEnabledInputIsIgnored)
{
    // Verify that if failsafe mode is enabled and the input value for the fans
    // is below the failsafe minimum value, the input is not used and the fans
    // are driven at failsafe RPM if zoneFlags do not have CAP_FAILSAFE_PWM set.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    double failsafePercent = 75.0;
    EXPECT_CALL(z, getFailSafeMode()).WillOnce(Return(true));
    EXPECT_CALL(z, getZoneFlags()).WillOnce(Return(0xfffe));
    EXPECT_CALL(z, getFailSafePercent())
        .Times(2)
        .WillRepeatedly(Return(failsafePercent));

    int64_t timeout = 0;
    std::unique_ptr<Sensor> s1 = std::make_unique<SensorMock>("fan0", timeout);
    std::unique_ptr<Sensor> s2 = std::make_unique<SensorMock>("fan1", timeout);
    // Grab pointers for mocking.
    SensorMock* sm1 = reinterpret_cast<SensorMock*>(s1.get());
    SensorMock* sm2 = reinterpret_cast<SensorMock*>(s2.get());

    double value = failsafePercent / 100;
    EXPECT_CALL(z, getSensor(StrEq("fan0"))).WillOnce(Return(s1.get()));
    EXPECT_CALL(*sm1, write(value));
    EXPECT_CALL(z, getSensor(StrEq("fan1"))).WillOnce(Return(s2.get()));
    EXPECT_CALL(*sm2, write(value));

    // This is a fan PID, so calling outputProc will try to write this value
    // to the sensors.

    // Setting 50%, will end up being 75% because the sensors are in failsafe
    // mode.
    p->outputProc(50.0);
}

TEST(FanControllerTest, OutputProcIfFailsafeInputIsIgnoredWithCapFailsafePWM)
{
    // Identical to OutputProc_VerifiesIfFailsafeEnabledInputIsIgnore, but set
    // CAP_FAILSAFE_PWM (1) flag to verify that the failsafe function has not
    // regressed.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    double failsafePercent = 75.0;
    EXPECT_CALL(z, getFailSafeMode()).WillOnce(Return(true));
    EXPECT_CALL(z, getZoneFlags()).WillOnce(Return(1));
    // Since CAP_FAILSAFE_PWM flag is enabled, the second getFailSafePercent is
    // skipped
    EXPECT_CALL(z, getFailSafePercent()).WillOnce(Return(failsafePercent));

    int64_t timeout = 0;
    std::unique_ptr<Sensor> s1 = std::make_unique<SensorMock>("fan0", timeout);
    std::unique_ptr<Sensor> s2 = std::make_unique<SensorMock>("fan1", timeout);
    // Grab pointers for mocking.
    SensorMock* sm1 = reinterpret_cast<SensorMock*>(s1.get());
    SensorMock* sm2 = reinterpret_cast<SensorMock*>(s2.get());

    double value = failsafePercent / 100;
    EXPECT_CALL(z, getSensor(StrEq("fan0"))).WillOnce(Return(s1.get()));
    EXPECT_CALL(*sm1, write(value));
    EXPECT_CALL(z, getSensor(StrEq("fan1"))).WillOnce(Return(s2.get()));
    EXPECT_CALL(*sm2, write(value));

    // This is a fan PID, so calling outputProc will try to write this value
    // to the sensors.

    // Setting 50%, will end up being 75% because the sensors are in failsafe
    // mode.
    p->outputProc(50.0);
}

TEST(FanControllerTest, OutputProc_BehavesAsExpected)
{
    // Verifies that when the system is not in failsafe mode, the input value
    // to outputProc is used to drive the sensors (fans).

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0", "fan1"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getFailSafeMode()).WillOnce(Return(false));

    int64_t timeout = 0;
    std::unique_ptr<Sensor> s1 = std::make_unique<SensorMock>("fan0", timeout);
    std::unique_ptr<Sensor> s2 = std::make_unique<SensorMock>("fan1", timeout);
    // Grab pointers for mocking.
    SensorMock* sm1 = reinterpret_cast<SensorMock*>(s1.get());
    SensorMock* sm2 = reinterpret_cast<SensorMock*>(s2.get());

    EXPECT_CALL(z, getSensor(StrEq("fan0"))).WillOnce(Return(s1.get()));
    EXPECT_CALL(*sm1, write(0.5));
    EXPECT_CALL(z, getSensor(StrEq("fan1"))).WillOnce(Return(s2.get()));
    EXPECT_CALL(*sm2, write(0.5));

    // This is a fan PID, so calling outputProc will try to write this value
    // to the sensors.
    p->outputProc(50.0);
}

TEST(FanControllerTest, OutputProc_VerifyFailSafeIgnoredIfInputHigher)
{
    // If the requested output is higher than the failsafe value, then use the
    // value provided to outputProc.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    EXPECT_CALL(z, getFailSafeMode()).WillOnce(Return(true));
    EXPECT_CALL(z, getZoneFlags()).WillOnce(Return(0));
    EXPECT_CALL(z, getFailSafePercent()).WillOnce(Return(75.0));

    int64_t timeout = 0;
    std::unique_ptr<Sensor> s1 = std::make_unique<SensorMock>("fan0", timeout);
    // Grab pointer for mocking.
    SensorMock* sm1 = reinterpret_cast<SensorMock*>(s1.get());

    // Converting from double to double for expectation.
    double percent = 80;
    double value = percent / 100;

    EXPECT_CALL(z, getSensor(StrEq("fan0"))).WillOnce(Return(s1.get()));
    EXPECT_CALL(*sm1, write(value));

    // This is a fan PID, so calling outputProc will try to write this value
    // to the sensors.
    p->outputProc(percent);
}

TEST(FanControllerTest, OutputProc_VerifyFailSafeCapFeature)
{
    // Identical to OutputProc_VerifyFailSafeIgnoredIfInputHigher, but set
    // CAP_FAILSAFE_PWM (1) flag to verify that that the capping mechanism
    // works as intended.

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    double failsafePercent = 75.0;
    EXPECT_CALL(z, getFailSafeMode()).WillOnce(Return(true));
    EXPECT_CALL(z, getZoneFlags()).WillOnce(Return(1));
    // Since CAP_FAILSAFE_PWM flag is enabled, the second getFailSafePercent is
    // skipped
    EXPECT_CALL(z, getFailSafePercent()).WillOnce(Return(failsafePercent));

    int64_t timeout = 0;
    std::unique_ptr<Sensor> s1 = std::make_unique<SensorMock>("fan0", timeout);
    // Grab pointer for mocking.
    SensorMock* sm1 = reinterpret_cast<SensorMock*>(s1.get());

    // Converting from double to double for expectation.
    double percent = 80;
    double value = failsafePercent / 100;

    EXPECT_CALL(z, getSensor(StrEq("fan0"))).WillOnce(Return(s1.get()));
    EXPECT_CALL(*sm1, write(value));

    // This is a fan PID, so calling outputProc will try to write this value
    // to the sensors.
    p->outputProc(percent);
}

TEST(FanControllerTest, OutputProc_VerifyFailSafeCapFeatureWithMoreFlags)
{
    // Identical to OutputProc_VerifyFailSafeCapFeature, but set zoneFlags with
    // more flags (including CAP_FAILSAFE_PWM) to ensure the capping mechanism
    // still works

    ZoneMock z;

    std::vector<std::string> inputs = {"fan0"};
    ec::pidinfo initial;

    std::unique_ptr<PIDController> p =
        FanController::createFanPid(&z, "fan1", inputs, initial);
    EXPECT_FALSE(p == nullptr);

    double failsafePercent = 75.0;
    EXPECT_CALL(z, getFailSafeMode()).WillOnce(Return(true));
    EXPECT_CALL(z, getZoneFlags()).WillOnce(Return(3));
    // Since CAP_FAILSAFE_PWM flag is enabled, the second getFailSafePercent is
    // skipped
    EXPECT_CALL(z, getFailSafePercent()).WillOnce(Return(failsafePercent));

    int64_t timeout = 0;
    std::unique_ptr<Sensor> s1 = std::make_unique<SensorMock>("fan0", timeout);
    // Grab pointer for mocking.
    SensorMock* sm1 = reinterpret_cast<SensorMock*>(s1.get());

    // Converting from double to double for expectation.
    double percent = 80;
    double value = failsafePercent / 100;

    EXPECT_CALL(z, getSensor(StrEq("fan0"))).WillOnce(Return(s1.get()));
    EXPECT_CALL(*sm1, write(value));

    // This is a fan PID, so calling outputProc will try to write this value
    // to the sensors.
    p->outputProc(percent);
}

} // namespace
} // namespace pid_control
