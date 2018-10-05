#pragma once

#include "dbus/util.hpp"

#include <sdbusplus/bus.hpp>
#include <string>

#include <gmock/gmock.h>

class DbusHelperMock : public DbusHelperInterface
{
  public:
    virtual ~DbusHelperMock() = default;

    MOCK_METHOD3(GetService,
                 std::string(sdbusplus::bus::bus&, const std::string&,
                             const std::string&));
    MOCK_METHOD4(GetProperties,
                 void(sdbusplus::bus::bus&, const std::string&,
                      const std::string&, struct SensorProperties*));

    MOCK_METHOD3(ThresholdsAsserted,
                 bool(sdbusplus::bus::bus& bus, const std::string& service,
                      const std::string& path));
};
