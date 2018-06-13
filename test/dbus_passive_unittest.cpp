#include "dbus/dbuspassive.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <string>

#include "test/dbushelper_mock.hpp"

using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::_;

std::string SensorIntf = "xyz.openbmc_project.Sensor.Value";

TEST(DbusPassiveTest, InvalidTypeFactoryTest) {
    // Verify the type is checked by the factory.

    sdbusplus::SdBusMock sdbus_mock;
    auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
    std::string type = "invalid";
    std::string id = "id";

    DbusHelperMock helper;

    std::unique_ptr<ReadInterface> ri =
        DbusPassive::CreateDbusPassive(bus_mock, type, id, &helper);

    EXPECT_EQ(ri, nullptr);
}

TEST(DbusPassiveTest, BoringConstructorTest) {
    // Just build the object, which should be avoided as this does no error
    // checking at present.

    sdbusplus::SdBusMock sdbus_mock;
    auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
    std::string type = "invalid";
    std::string id = "id";
    std::string path = "/xyz/openbmc_project/sensors/unknown/id";

    DbusHelperMock helper;
    EXPECT_CALL(helper, GetService(_, StrEq(SensorIntf), StrEq(path)))
        .WillOnce(Return("asdf"));

    EXPECT_CALL(helper, GetProperties(_, StrEq("asdf"), StrEq(path),
                                      NotNull()))
        .WillOnce(Invoke([&](sdbusplus::bus::bus& bus,
                             const std::string& service,
                             const std::string& path,
                             struct SensorProperties* prop)  {
            prop->scale = -3;
            prop->value = 10;
            prop->unit = "x";
        }));

    DbusPassive(bus_mock, type, id, &helper);
    // Success
}

class DbusPassiveTestObj : public ::testing::Test {
    protected:
        DbusPassiveTestObj()
        : sdbus_mock(),
          bus_mock(std::move(sdbusplus::get_mocked_new(&sdbus_mock))),
          helper()
        {
            EXPECT_CALL(helper, GetService(_, StrEq(SensorIntf), StrEq(path)))
                .WillOnce(Return("asdf"));

            EXPECT_CALL(helper, GetProperties(_, StrEq("asdf"), StrEq(path),
                                             NotNull()))
                .WillOnce(Invoke([&](sdbusplus::bus::bus& bus,
                                     const std::string& service,
                                     const std::string& path,
                                     struct SensorProperties* prop)  {
                    prop->scale = _scale;
                    prop->value = _value;
                    prop->unit = "x";
                }));

            ri = DbusPassive::CreateDbusPassive(bus_mock, type, id, &helper);
            passive = reinterpret_cast<DbusPassive*>(ri.get());
            EXPECT_FALSE(passive == nullptr);
        }

        sdbusplus::SdBusMock sdbus_mock;
        sdbusplus::bus::bus bus_mock;
        DbusHelperMock helper;
        std::string type = "temp";
        std::string id = "id";
        std::string path = "/xyz/openbmc_project/sensors/temperature/id";
        int64_t _scale = -3;
        int64_t _value = 10;

        std::unique_ptr<ReadInterface> ri;
        DbusPassive *passive;
};

TEST_F(DbusPassiveTestObj, ReadReturnsExpectedValues) {
    // Verify read is returning the values.
    ReadReturn v;
    v.value = 0.01;
    // TODO: updated is set when the value is created, so we can range check
    // it.
    ReadReturn r = passive->read();
    EXPECT_EQ(v.value, r.value);
}

TEST_F(DbusPassiveTestObj, SetValueUpdatesValue) {
    // Verify setvalue does as advertised.

    double value = 0.01;
    passive->setValue(value);

    // TODO: updated is set when the value is set, so we can range check it.
    ReadReturn r = passive->read();
    EXPECT_EQ(value, r.value);
}

TEST_F(DbusPassiveTestObj, GetScaleReturnsExpectedValue) {
    // Verify the scale is returned as expected.
    EXPECT_EQ(_scale, passive->getScale());
}

TEST_F(DbusPassiveTestObj, GetIdReturnsExpectedValue) {
    // Verify getId returns the expected value.
    EXPECT_EQ(id, passive->getId());
}

TEST_F(DbusPassiveTestObj, VerifyHandlesDbusSignal) {
    // The dbus passive sensor listens for updates and if it's the Value
    // property, it needs to handle it.

    EXPECT_CALL(sdbus_mock, sd_bus_message_ref(IsNull()))
        .WillOnce(Return(nullptr));
    sdbusplus::message::message msg(nullptr, &sdbus_mock);

    const char *Value = "Value";
    int64_t xValue = 10000;
    const char *intf = "xyz.openbmc_project.Sensor.Value";
    // string, std::map<std::string, sdbusplus::message::variant<int64_t>>
    // msg.read(msgSensor, msgData);

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_read_basic(IsNull(), 's', NotNull()))
        .WillOnce(Invoke([&](sd_bus_message *m, char type, void *p) {
            const char **s = static_cast<const char **>(p);
            // Read the first parameter, the string.
            *s = intf;
            return 0;
        }))
        .WillOnce(Invoke([&](sd_bus_message *m, char type, void *p) {
            const char **s = static_cast<const char **>(p);
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
        .WillOnce(Return(1));  // So it exits the loop after reading one pair.

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

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_read_basic(IsNull(), 'x', NotNull()))
        .WillOnce(Invoke([&](sd_bus_message *m, char type, void *p) {
            int64_t *s = static_cast<int64_t *>(p);
            *s = xValue;
            return 0;
        }));

    EXPECT_CALL(sdbus_mock, sd_bus_message_exit_container(IsNull()))
        .WillOnce(Return(0))  /* variant. */
        .WillOnce(Return(0))  /* std::pair */
        .WillOnce(Return(0)); /* std::map */

    int rv = HandleSensorValue(msg, passive);
    EXPECT_EQ(rv, 0); // It's always 0.

    ReadReturn r = passive->read();
    EXPECT_EQ(10, r.value);
}

TEST_F(DbusPassiveTestObj, VerifyIgnoresOtherPropertySignal) {
    // The dbus passive sensor listens for updates and if it's the Value
    // property, it needs to handle it.  In this case, it won't be.

    EXPECT_CALL(sdbus_mock, sd_bus_message_ref(IsNull()))
        .WillOnce(Return(nullptr));
    sdbusplus::message::message msg(nullptr, &sdbus_mock);

    const char *Scale = "Scale";
    int64_t xScale = -6;
    const char *intf = "xyz.openbmc_project.Sensor.Value";
    // string, std::map<std::string, sdbusplus::message::variant<int64_t>>
    // msg.read(msgSensor, msgData);

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_read_basic(IsNull(), 's', NotNull()))
        .WillOnce(Invoke([&](sd_bus_message *m, char type, void *p) {
            const char **s = static_cast<const char **>(p);
            // Read the first parameter, the string.
            *s = intf;
            return 0;
        }))
        .WillOnce(Invoke([&](sd_bus_message *m, char type, void *p) {
            const char **s = static_cast<const char **>(p);
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
        .WillOnce(Return(1));  // So it exits the loop after reading one pair.

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

    EXPECT_CALL(sdbus_mock,
                sd_bus_message_read_basic(IsNull(), 'x', NotNull()))
        .WillOnce(Invoke([&](sd_bus_message *m, char type, void *p) {
            int64_t *s = static_cast<int64_t *>(p);
            *s = xScale;
            return 0;
        }));

    EXPECT_CALL(sdbus_mock, sd_bus_message_exit_container(IsNull()))
        .WillOnce(Return(0))  /* variant. */
        .WillOnce(Return(0))  /* std::pair */
        .WillOnce(Return(0)); /* std::map */

    int rv = HandleSensorValue(msg, passive);
    EXPECT_EQ(rv, 0); // It's always 0.

    ReadReturn r = passive->read();
    EXPECT_EQ(0.01, r.value);
}
