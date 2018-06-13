#include "dbus/dbuspassive.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <string>

#include "test/dbushelper_mock.hpp"

using ::testing::Invoke;
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
                    prop->scale = scale;
                    prop->value = value;
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
        int64_t scale = -3;
        int64_t value = 10;

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
    EXPECT_EQ(scale, passive->getScale());
}

TEST_F(DbusPassiveTestObj, GetIdReturnsExpectedValue) {
    // Verify getId returns the expected value.
    EXPECT_EQ(id, passive->getId());
}

TEST_F(DbusPassiveTestObj, VerifyHandlesDbusSignal) {
    // The dbus passive sensor listens for updates and if it's the Value
    // property, it needs to handle it.

    sdbusplus::message::message msg(nullptr, &sdbus_mock);

    int r = HandleSensorValue(msg, passive);
    EXPECT_EQ(r, 0); // It's always 0.
}

