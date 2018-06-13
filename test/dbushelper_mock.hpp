#pragma once

#include <gmock/gmock.h>
#include <sdbusplus/bus/bus.hpp>
#include <string>

#include "dbus/util.hpp"

class DbusHelperMock : public DbusHelperInterface
{
    public:
        virtual ~DbusHelperMock() = default;

        MOCK_METHOD3(GetService, std::string(sdbusplus::bus::bus&,
                                             const std::string&,
                                             const std::string&));
        MOCK_METHOD4(GetProperties, void(sdbusplus::bus::bus&,
                                         const std::string&,
                                         const std::string&,
                                         struct SensorProperties*));
};
