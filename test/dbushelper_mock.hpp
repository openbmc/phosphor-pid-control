#pragma once

#include "dbus/dbushelper_interface.hpp"

#include <string>

#include <gmock/gmock.h>
namespace pid_control
{

class DbusHelperMock : public DbusHelperInterface
{
  public:
    ~DbusHelperMock() override = default;

    MOCK_METHOD2(getService,
                 std::string(const std::string&, const std::string&));
    MOCK_METHOD3(getProperties, void(const std::string&, const std::string&,
                                     SensorProperties*));

    MOCK_METHOD2(thresholdsAsserted,
                 bool(const std::string& service, const std::string& path));
};

} // namespace pid_control
