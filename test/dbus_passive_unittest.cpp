#include "conf.hpp"
#include "dbus/dbushelper_interface.hpp"
#include "dbus/dbuspassive.hpp"
#include "interfaces.hpp"
#include "test/dbushelper_mock.hpp"

#include <systemd/sd-bus.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/Critical/common.hpp>
#include <xyz/openbmc_project/Sensor/Value/common.hpp>
#include <xyz/openbmc_project/State/Decorator/Availability/common.hpp>

#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace pid_control
{
namespace
{

using ::testing::Invoke;
using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrEq;

using SensorValue = sdbusplus::common::xyz::openbmc_project::sensor::Value;
using ThresholdCritical =
    sdbusplus::common::xyz::openbmc_project::sensor::threshold::Critical;
using DecoratorAvailability =
    sdbusplus::common::xyz::openbmc_project::state::decorator::Availability;

TEST(DbusPassiveTest, FactoryFailsWithInvalidType)
{
    // Verify the type is checked by the factory.

    sdbusplus::SdBusMock sdbus_mock;
    auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
    std::string type = "invalid";
    std::string id = "id";

    auto helper = std::make_unique<DbusHelperMock>();
    auto info = conf::SensorConfig();

    std::unique_ptr<ReadInterface> ri = DbusPassive::createDbusPassive(
        bus_mock, type, id, std::move(helper), &info, nullptr);

    EXPECT_EQ(ri, nullptr);
}

TEST(DbusPassiveTest, BoringConstructorTest)
{
    // Simply build the object, does no error checking.

    sdbusplus::SdBusMock sdbus_mock;
    auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
    std::string type = "invalid";
    std::string id = "id";
    std::string path =
        std::format("{}/unknown/id", SensorValue::namespace_path::value);

    auto helper = std::make_unique<DbusHelperMock>();
    SensorProperties properties;

    DbusPassive(bus_mock, type, id, std::move(helper), properties, false, false,
                path, nullptr);
    // Success
}

class DbusPassiveTestObj : public ::testing::Test
{
  protected:
    DbusPassiveTestObj() :
        sdbus_mock(), bus_mock(sdbusplus::get_mocked_new(&sdbus_mock)),
        helper(std::make_unique<DbusHelperMock>())
    {
        EXPECT_CALL(*helper,
                    getService(StrEq(SensorValue::interface), StrEq(path)))
            .WillOnce(Return("asdf"));

        EXPECT_CALL(*helper,
                    getProperties(StrEq("asdf"), StrEq(path), NotNull()))
            .WillOnce(Invoke([&]([[maybe_unused]] const std::string& service,
                                 [[maybe_unused]] const std::string& path,
                                 SensorProperties* prop) {
                prop->scale = _scale;
                prop->value = _value;
                prop->unit = "x";
                prop->min = 0;
                prop->max = 0;
                prop->available = true;
            }));
        EXPECT_CALL(*helper, thresholdsAsserted(StrEq("asdf"), StrEq(path)))
            .WillOnce(Return(false));

        auto info = conf::SensorConfig();
        info.unavailableAsFailed = true;
        ri = DbusPassive::createDbusPassive(bus_mock, type, id,
                                            std::move(helper), &info, nullptr);
        passive = reinterpret_cast<DbusPassive*>(ri.get());
        EXPECT_FALSE(passive == nullptr);
    }

    sdbusplus::SdBusMock sdbus_mock;
    sdbusplus::bus_t bus_mock;
    std::unique_ptr<DbusHelperMock> helper;
    std::string type = "temp";
    std::string id = "id";
    std::string path =
        std::format("{}/{}/id", SensorValue::namespace_path::value,
                    SensorValue::namespace_path::temperature);
    int64_t _scale = -3;
    int64_t _value = 10;

    std::unique_ptr<ReadInterface> ri;
    DbusPassive* passive;
};

TEST_F(DbusPassiveTestObj, ReadReturnsExpectedValues)
{
    // Verify read is returning the values.
    ReadReturn v;
    v.value = 0.01;
    // TODO: updated is set when the value is created, so we can range check
    // it.
    ReadReturn r = passive->read();
    EXPECT_EQ(v.value, r.value);
}

TEST_F(DbusPassiveTestObj, SetValueUpdatesValue)
{
    // Verify setvalue does as advertised.

    double value = 0.01;
    passive->setValue(value);

    // TODO: updated is set when the value is set, so we can range check it.
    ReadReturn r = passive->read();
    EXPECT_EQ(value, r.value);
}

TEST_F(DbusPassiveTestObj, GetScaleReturnsExpectedValue)
{
    // Verify the scale is returned as expected.
    EXPECT_EQ(_scale, passive->getScale());
}

TEST_F(DbusPassiveTestObj, getIDReturnsExpectedValue)
{
    // Verify getID returns the expected value.
    EXPECT_EQ(id, passive->getID());
}

TEST_F(DbusPassiveTestObj, GetMinValueReturnsExpectedValue)
{
    EXPECT_DOUBLE_EQ(0, passive->getMin());
}

TEST_F(DbusPassiveTestObj, VerifyHandlesDbusSignal)
{
    // The dbus passive sensor listens for updates and if it's the Value
    // property, it needs to handle it.

    EXPECT_CALL(sdbus_mock, sd_bus_message_ref(IsNull()))
        .WillOnce(Return(nullptr));
    sdbusplus::message_t msg(nullptr, &sdbus_mock);

    const char* Value = "Value";
    int64_t xValue = 10000;
    // string, std::map<std::string, std::variant<int64_t>>
    // msg.read(msgSensor, msgData);

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 's', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            // Read the first parameter, the string.
            *s = SensorValue::interface;
            return 0;
        }))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            *s = Value;
            // Read the string in the pair (dictionary).
            return 0;
        }));

    // std::map
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'a', StrEq("{sv}")))
        .WillOnce(Return(0));

    // while !at_end()
    EXPECT_CALL(sdbus_mock, sd_bus_message_at_end(IsNull(), 0))
        .WillOnce(Return(0))
        .WillOnce(Return(1)); // So it exits the loop after reading one pair.

    // std::pair
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'e', StrEq("sv")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("x")))
        .WillOnce(Return(1));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'v', StrEq("x")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 'x', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            int64_t* s = static_cast<int64_t*>(p);
            *s = xValue;
            return 0;
        }));

    EXPECT_CALL(sdbus_mock, sd_bus_message_exit_container(IsNull()))
        .WillOnce(Return(0))  /* variant. */
        .WillOnce(Return(0))  /* std::pair */
        .WillOnce(Return(0)); /* std::map */

    int rv = handleSensorValue(msg, passive);
    EXPECT_EQ(rv, 0); // It's always 0.

    ReadReturn r = passive->read();
    EXPECT_EQ(10, r.value);
}

TEST_F(DbusPassiveTestObj, VerifyIgnoresOtherPropertySignal)
{
    // The dbus passive sensor listens for updates and if it's the Value
    // property, it needs to handle it.  In this case, it won't be.

    EXPECT_CALL(sdbus_mock, sd_bus_message_ref(IsNull()))
        .WillOnce(Return(nullptr));
    sdbusplus::message_t msg(nullptr, &sdbus_mock);

    const char* Scale = "Scale";
    int64_t xScale = -6;
    // string, std::map<std::string, std::variant<int64_t>>
    // msg.read(msgSensor, msgData);

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 's', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            // Read the first parameter, the string.
            *s = SensorValue::interface;
            return 0;
        }))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            *s = Scale;
            // Read the string in the pair (dictionary).
            return 0;
        }));

    // std::map
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'a', StrEq("{sv}")))
        .WillOnce(Return(0));

    // while !at_end()
    EXPECT_CALL(sdbus_mock, sd_bus_message_at_end(IsNull(), 0))
        .WillOnce(Return(0))
        .WillOnce(Return(1)); // So it exits the loop after reading one pair.

    // std::pair
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'e', StrEq("sv")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("x")))
        .WillOnce(Return(1));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'v', StrEq("x")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 'x', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            int64_t* s = static_cast<int64_t*>(p);
            *s = xScale;
            return 0;
        }));

    EXPECT_CALL(sdbus_mock, sd_bus_message_exit_container(IsNull()))
        .WillOnce(Return(0))  /* variant. */
        .WillOnce(Return(0))  /* std::pair */
        .WillOnce(Return(0)); /* std::map */

    int rv = handleSensorValue(msg, passive);
    EXPECT_EQ(rv, 0); // It's always 0.

    ReadReturn r = passive->read();
    EXPECT_EQ(0.01, r.value);
}

TEST_F(DbusPassiveTestObj, VerifyCriticalThresholdAssert)
{
    // Verifies when a threshold is crossed the sensor goes into error state
    EXPECT_CALL(sdbus_mock, sd_bus_message_ref(IsNull()))
        .WillOnce(Return(nullptr));
    sdbusplus::message_t msg(nullptr, &sdbus_mock);

    bool alarm = true;

    passive->setFailed(false);

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 's', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            // Read the first parameter, the string.
            *s = ThresholdCritical::interface;
            return 0;
        }))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            *s = ThresholdCritical::property_names::critical_alarm_high;
            // Read the string in the pair (dictionary).
            return 0;
        }));

    // std::map
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'a', StrEq("{sv}")))
        .WillOnce(Return(0));

    // while !at_end()
    EXPECT_CALL(sdbus_mock, sd_bus_message_at_end(IsNull(), 0))
        .WillOnce(Return(0))
        .WillOnce(Return(1)); // So it exits the loop after reading one pair.

    // std::pair
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'e', StrEq("sv")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("x")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("d")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(1));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 'b', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            bool* s = static_cast<bool*>(p);
            *s = alarm;
            return 0;
        }));

    EXPECT_CALL(sdbus_mock, sd_bus_message_exit_container(IsNull()))
        .WillOnce(Return(0))  /* variant. */
        .WillOnce(Return(0))  /* std::pair */
        .WillOnce(Return(0)); /* std::map */

    int rv = handleSensorValue(msg, passive);
    EXPECT_EQ(rv, 0); // It's always 0.
    bool failed = passive->getFailed();
    EXPECT_EQ(failed, true);
}

TEST_F(DbusPassiveTestObj, VerifyCriticalThresholdDeassert)
{
    // Verifies when a threshold is deasserted a failed sensor goes back into
    // the normal state
    EXPECT_CALL(sdbus_mock, sd_bus_message_ref(IsNull()))
        .WillOnce(Return(nullptr));
    sdbusplus::message_t msg(nullptr, &sdbus_mock);

    bool alarm = false;

    passive->setFailed(true);

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 's', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            // Read the first parameter, the string.
            *s = ThresholdCritical::interface;
            return 0;
        }))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            *s = ThresholdCritical::property_names::critical_alarm_high;
            // Read the string in the pair (dictionary).
            return 0;
        }));

    // std::map
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'a', StrEq("{sv}")))
        .WillOnce(Return(0));

    // while !at_end()
    EXPECT_CALL(sdbus_mock, sd_bus_message_at_end(IsNull(), 0))
        .WillOnce(Return(0))
        .WillOnce(Return(1)); // So it exits the loop after reading one pair.

    // std::pair
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'e', StrEq("sv")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("x")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("d")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(1));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 'b', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            bool* s = static_cast<bool*>(p);
            *s = alarm;
            return 0;
        }));

    EXPECT_CALL(sdbus_mock, sd_bus_message_exit_container(IsNull()))
        .WillOnce(Return(0))  /* variant. */
        .WillOnce(Return(0))  /* std::pair */
        .WillOnce(Return(0)); /* std::map */

    int rv = handleSensorValue(msg, passive);
    EXPECT_EQ(rv, 0); // It's always 0.
    bool failed = passive->getFailed();
    EXPECT_EQ(failed, false);
}

TEST_F(DbusPassiveTestObj, VerifyAvailableDeassert)
{
    // Verifies when Available is deasserted && unavailableAsFailed == true,
    // the sensor goes into error state
    EXPECT_CALL(sdbus_mock, sd_bus_message_ref(IsNull()))
        .WillOnce(Return(nullptr));
    sdbusplus::message_t msg(nullptr, &sdbus_mock);

    bool asserted = false;

    passive->setAvailable(true);

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 's', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            // Read the first parameter, the string.
            *s = DecoratorAvailability::interface;
            return 0;
        }))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            *s = DecoratorAvailability::property_names::available;
            // Read the string in the pair (dictionary).
            return 0;
        }));

    // std::map
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'a', StrEq("{sv}")))
        .WillOnce(Return(0));

    // while !at_end()
    EXPECT_CALL(sdbus_mock, sd_bus_message_at_end(IsNull(), 0))
        .WillOnce(Return(0))
        .WillOnce(Return(1)); // So it exits the loop after reading one pair.

    // std::pair
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'e', StrEq("sv")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("x")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("d")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(1));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 'b', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            bool* s = static_cast<bool*>(p);
            *s = asserted;
            return 0;
        }));

    EXPECT_CALL(sdbus_mock, sd_bus_message_exit_container(IsNull()))
        .WillOnce(Return(0))  /* variant. */
        .WillOnce(Return(0))  /* std::pair */
        .WillOnce(Return(0)); /* std::map */

    int rv = handleSensorValue(msg, passive);
    EXPECT_EQ(rv, 0); // It's always 0.
    bool failed = passive->getFailed();
    EXPECT_EQ(failed, true);
}

TEST_F(DbusPassiveTestObj, VerifyAvailableAssert)
{
    // Verifies when Available is asserted && unavailableAsFailed == true,
    // an error sensor goes back to normal state
    EXPECT_CALL(sdbus_mock, sd_bus_message_ref(IsNull()))
        .WillOnce(Return(nullptr));
    sdbusplus::message_t msg(nullptr, &sdbus_mock);

    bool asserted = true;

    passive->setAvailable(false);
    bool failed = passive->getFailed();
    EXPECT_EQ(failed, true);

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 's', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            // Read the first parameter, the string.
            *s = DecoratorAvailability::interface;
            return 0;
        }))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            *s = DecoratorAvailability::property_names::available;
            // Read the string in the pair (dictionary).
            return 0;
        }));

    // std::map
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'a', StrEq("{sv}")))
        .WillOnce(Return(0));

    // while !at_end()
    EXPECT_CALL(sdbus_mock, sd_bus_message_at_end(IsNull(), 0))
        .WillOnce(Return(0))
        .WillOnce(Return(1)); // So it exits the loop after reading one pair.

    // std::pair
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'e', StrEq("sv")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("x")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("d")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(1));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 'b', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            bool* s = static_cast<bool*>(p);
            *s = asserted;
            return 0;
        }));

    EXPECT_CALL(sdbus_mock, sd_bus_message_exit_container(IsNull()))
        .WillOnce(Return(0))  /* variant. */
        .WillOnce(Return(0))  /* std::pair */
        .WillOnce(Return(0)); /* std::map */

    int rv = handleSensorValue(msg, passive);
    EXPECT_EQ(rv, 0); // It's always 0.
    failed = passive->getFailed();
    EXPECT_EQ(failed, false);
}

class DbusPassiveTestUnaSensorNotAsFailedObj : public ::testing::Test
{
  protected:
    DbusPassiveTestUnaSensorNotAsFailedObj() :
        sdbus_mock(), bus_mock(sdbusplus::get_mocked_new(&sdbus_mock)),
        helper(std::make_unique<DbusHelperMock>())
    {
        EXPECT_CALL(*helper,
                    getService(StrEq(SensorValue::interface), StrEq(path)))
            .WillOnce(Return("asdf"));

        EXPECT_CALL(*helper,
                    getProperties(StrEq("asdf"), StrEq(path), NotNull()))
            .WillOnce(Invoke([&]([[maybe_unused]] const std::string& service,
                                 [[maybe_unused]] const std::string& path,
                                 SensorProperties* prop) {
                prop->scale = _scale;
                prop->value = _value;
                prop->unit = "x";
                prop->min = 0;
                prop->max = 0;
                prop->available = true;
            }));
        EXPECT_CALL(*helper, thresholdsAsserted(StrEq("asdf"), StrEq(path)))
            .WillOnce(Return(false));

        auto info = conf::SensorConfig();
        info.unavailableAsFailed = false;
        ri = DbusPassive::createDbusPassive(bus_mock, type, id,
                                            std::move(helper), &info, nullptr);
        passive = reinterpret_cast<DbusPassive*>(ri.get());
        EXPECT_FALSE(passive == nullptr);
    }

    sdbusplus::SdBusMock sdbus_mock;
    sdbusplus::bus_t bus_mock;
    std::unique_ptr<DbusHelperMock> helper;
    std::string type = "temp";
    std::string id = "id";
    std::string path =
        std::format("{}/{}/id", SensorValue::namespace_path::value,
                    SensorValue::namespace_path::temperature);
    int64_t _scale = -3;
    int64_t _value = 10;

    std::unique_ptr<ReadInterface> ri;
    DbusPassive* passive;
};

TEST_F(DbusPassiveTestUnaSensorNotAsFailedObj, VerifyAvailableDeassert)
{
    // Verifies when Available is deasserted && unavailableAsFailed == false,
    // the sensor remains at OK state but reading goes to NaN.
    EXPECT_CALL(sdbus_mock, sd_bus_message_ref(IsNull()))
        .WillOnce(Return(nullptr));
    sdbusplus::message_t msg(nullptr, &sdbus_mock);

    bool asserted = false;

    passive->setAvailable(true);

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 's', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            // Read the first parameter, the string.
            *s = DecoratorAvailability::interface;
            return 0;
        }))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            *s = DecoratorAvailability::property_names::available;
            // Read the string in the pair (dictionary).
            return 0;
        }));

    // std::map
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'a', StrEq("{sv}")))
        .WillOnce(Return(0));

    // while !at_end()
    EXPECT_CALL(sdbus_mock, sd_bus_message_at_end(IsNull(), 0))
        .WillOnce(Return(0))
        .WillOnce(Return(1)); // So it exits the loop after reading one pair.

    // std::pair
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'e', StrEq("sv")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("x")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("d")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(1));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 'b', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            bool* s = static_cast<bool*>(p);
            *s = asserted;
            return 0;
        }));

    EXPECT_CALL(sdbus_mock, sd_bus_message_exit_container(IsNull()))
        .WillOnce(Return(0))  /* variant. */
        .WillOnce(Return(0))  /* std::pair */
        .WillOnce(Return(0)); /* std::map */

    int rv = handleSensorValue(msg, passive);
    EXPECT_EQ(rv, 0); // It's always 0.
    bool failed = passive->getFailed();
    EXPECT_EQ(failed, false);
    ReadReturn r = passive->read();
    EXPECT_FALSE(std::isfinite(r.value));
}

TEST_F(DbusPassiveTestUnaSensorNotAsFailedObj, VerifyAvailableAssert)
{
    // Verifies when a sensor's state goes from unavailable to available
    // && unavailableAsFailed == false, this sensor remains at OK state.
    EXPECT_CALL(sdbus_mock, sd_bus_message_ref(IsNull()))
        .WillOnce(Return(nullptr));
    sdbusplus::message_t msg(nullptr, &sdbus_mock);

    bool asserted = true;

    passive->setAvailable(false);
    bool failed = passive->getFailed();
    EXPECT_EQ(failed, false);

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 's', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            // Read the first parameter, the string.
            *s = DecoratorAvailability::interface;
            return 0;
        }))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            const char** s = static_cast<const char**>(p);
            *s = DecoratorAvailability::property_names::available;
            // Read the string in the pair (dictionary).
            return 0;
        }));

    // std::map
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'a', StrEq("{sv}")))
        .WillOnce(Return(0));

    // while !at_end()
    EXPECT_CALL(sdbus_mock, sd_bus_message_at_end(IsNull(), 0))
        .WillOnce(Return(0))
        .WillOnce(Return(1)); // So it exits the loop after reading one pair.

    // std::pair
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'e', StrEq("sv")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("x")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("d")))
        .WillOnce(Return(0));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_verify_type(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(1));
    EXPECT_CALL(sdbus_mock,
                sd_bus_message_enter_container(IsNull(), 'v', StrEq("b")))
        .WillOnce(Return(0));

    EXPECT_CALL(sdbus_mock, sd_bus_message_read_basic(IsNull(), 'b', NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] sd_bus_message* m,
                             [[maybe_unused]] char type, void* p) {
            bool* s = static_cast<bool*>(p);
            *s = asserted;
            return 0;
        }));

    EXPECT_CALL(sdbus_mock, sd_bus_message_exit_container(IsNull()))
        .WillOnce(Return(0))  /* variant. */
        .WillOnce(Return(0))  /* std::pair */
        .WillOnce(Return(0)); /* std::map */

    int rv = handleSensorValue(msg, passive);
    EXPECT_EQ(rv, 0); // It's always 0.
    failed = passive->getFailed();
    EXPECT_EQ(failed, false);
}

void GetPropertiesMax3k([[maybe_unused]] const std::string& service,
                        [[maybe_unused]] const std::string& path,
                        SensorProperties* prop)
{
    prop->scale = -3;
    prop->value = 10;
    prop->unit = "x";
    prop->min = 0;
    prop->max = 3000;
}

using GetPropertiesFunction = std::function<void(
    const std::string&, const std::string&, SensorProperties*)>;

// TODO: There is definitely a cleaner way to do this.
class DbusPassiveTest3kMaxObj : public ::testing::Test
{
  protected:
    DbusPassiveTest3kMaxObj() :
        sdbus_mock(), bus_mock(sdbusplus::get_mocked_new(&sdbus_mock)),
        helper(std::make_unique<DbusHelperMock>())
    {
        EXPECT_CALL(*helper,
                    getService(StrEq(SensorValue::interface), StrEq(path)))
            .WillOnce(Return("asdf"));

        EXPECT_CALL(*helper,
                    getProperties(StrEq("asdf"), StrEq(path), NotNull()))
            .WillOnce(_getProps);
        EXPECT_CALL(*helper, thresholdsAsserted(StrEq("asdf"), StrEq(path)))
            .WillOnce(Return(false));

        auto info = conf::SensorConfig();
        ri = DbusPassive::createDbusPassive(bus_mock, type, id,
                                            std::move(helper), &info, nullptr);
        passive = reinterpret_cast<DbusPassive*>(ri.get());
        EXPECT_FALSE(passive == nullptr);
    }

    sdbusplus::SdBusMock sdbus_mock;
    sdbusplus::bus_t bus_mock;
    std::unique_ptr<DbusHelperMock> helper;
    std::string type = "temp";
    std::string id = "id";
    std::string path =
        std::format("{}/{}/id", SensorValue::namespace_path::value,
                    SensorValue::namespace_path::temperature);
    int64_t _scale = -3;
    int64_t _value = 10;

    std::unique_ptr<ReadInterface> ri;
    DbusPassive* passive;
    GetPropertiesFunction _getProps = &GetPropertiesMax3k;
};

TEST_F(DbusPassiveTest3kMaxObj, ReadMinAndMaxReturnsExpected)
{
    EXPECT_DOUBLE_EQ(0, passive->getMin());
    EXPECT_DOUBLE_EQ(3, passive->getMax());
}

class DbusPassiveTest3kMaxIgnoredObj : public ::testing::Test
{
  protected:
    DbusPassiveTest3kMaxIgnoredObj() :
        sdbus_mock(), bus_mock(sdbusplus::get_mocked_new(&sdbus_mock)),
        helper(std::make_unique<DbusHelperMock>())
    {
        EXPECT_CALL(*helper,
                    getService(StrEq(SensorValue::interface), StrEq(path)))
            .WillOnce(Return("asdf"));

        EXPECT_CALL(*helper,
                    getProperties(StrEq("asdf"), StrEq(path), NotNull()))
            .WillOnce(_getProps);
        EXPECT_CALL(*helper, thresholdsAsserted(StrEq("asdf"), StrEq(path)))
            .WillOnce(Return(false));

        auto info = conf::SensorConfig();
        info.ignoreDbusMinMax = true;
        ri = DbusPassive::createDbusPassive(bus_mock, type, id,
                                            std::move(helper), &info, nullptr);
        passive = reinterpret_cast<DbusPassive*>(ri.get());
        EXPECT_FALSE(passive == nullptr);
    }

    sdbusplus::SdBusMock sdbus_mock;
    sdbusplus::bus_t bus_mock;
    std::unique_ptr<DbusHelperMock> helper;
    std::string type = "temp";
    std::string id = "id";
    std::string path =
        std::format("{}/{}/id", SensorValue::namespace_path::value,
                    SensorValue::namespace_path::temperature);
    int64_t _scale = -3;
    int64_t _value = 10;

    std::unique_ptr<ReadInterface> ri;
    DbusPassive* passive;
    GetPropertiesFunction _getProps = &GetPropertiesMax3k;
};

TEST_F(DbusPassiveTest3kMaxIgnoredObj, ReadMinAndMaxReturnsExpected)
{
    EXPECT_DOUBLE_EQ(0, passive->getMin());
    EXPECT_DOUBLE_EQ(0, passive->getMax());
}

} // namespace
} // namespace pid_control
