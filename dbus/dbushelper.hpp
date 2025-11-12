#pragma once

#include "dbushelper_interface.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/Critical/common.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/Warning/common.hpp>
#include <xyz/openbmc_project/State/Decorator/Availability/common.hpp>

#include <string>
#include <variant>

using SensorThresholdWarning =
    sdbusplus::common::xyz::openbmc_project::sensor::threshold::Warning;
using SensorThresholdCritical =
    sdbusplus::common::xyz::openbmc_project::sensor::threshold::Critical;
using StateDecoratorAvailability =
    sdbusplus::common::xyz::openbmc_project::state::decorator::Availability;

namespace pid_control
{

class DbusHelper : public DbusHelperInterface
{
  public:
    static constexpr char propertiesintf[] = "org.freedesktop.DBus.Properties";

    explicit DbusHelper(sdbusplus::bus_t& bus) : _bus(bus) {}
    DbusHelper() = delete;
    ~DbusHelper() override = default;

    DbusHelper(const DbusHelper&) = delete;
    DbusHelper& operator=(const DbusHelper&) = delete;

    DbusHelper(DbusHelper&&) = default;
    DbusHelper& operator=(DbusHelper&&) = delete;

    std::string getService(const std::string& intf,
                           const std::string& path) override;

    void getProperties(const std::string& service, const std::string& path,
                       SensorProperties* prop) override;

    bool thresholdsAsserted(const std::string& service,
                            const std::string& path) override;

    template <typename T>
    void getProperty(const std::string& service, const std::string& path,
                     const std::string& interface,
                     const std::string& propertyName, T& prop)
    {
        namespace log = phosphor::logging;

        auto msg = _bus.new_method_call(service.c_str(), path.c_str(),
                                        propertiesintf, "Get");

        msg.append(interface, propertyName);

        std::variant<T> result;
        try
        {
            auto valueResponseMsg = _bus.call(msg);
            valueResponseMsg.read(result);
        }
        catch (const sdbusplus::exception_t& ex)
        {
            log::log<log::level::ERR>("Get Property Failed",
                                      log::entry("WHAT=%s", ex.what()));
            throw;
        }

        prop = std::get<T>(result);
    }

  private:
    sdbusplus::bus_t& _bus;
};

} // namespace pid_control
