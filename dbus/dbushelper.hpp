#pragma once

#include "dbushelper_interface.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <string>
#include <variant>

namespace pid_control
{

class DbusHelper : public DbusHelperInterface
{
  public:
    static constexpr char sensorintf[] = "xyz.openbmc_project.Sensor.Value";
    static constexpr char propertiesintf[] = "org.freedesktop.DBus.Properties";
    static constexpr char criticalThreshInf[] =
        "xyz.openbmc_project.Sensor.Threshold.Critical";

    explicit DbusHelper(sdbusplus::bus::bus bus) : _bus(std::move(bus))
    {}
    ~DbusHelper() = default;

    DbusHelper(const DbusHelper&) = delete;
    DbusHelper& operator=(const DbusHelper&) = delete;

    DbusHelper(DbusHelper&&) = default;
    DbusHelper& operator=(DbusHelper&&) = default;

    std::string getService(sdbusplus::bus::bus& bus, const std::string& intf,
                           const std::string& path) override;

    void getProperties(sdbusplus::bus::bus& bus, const std::string& service,
                       const std::string& path,
                       struct SensorProperties* prop) override;

    bool thresholdsAsserted(sdbusplus::bus::bus& bus,
                            const std::string& service,
                            const std::string& path) override;

    template <typename T>
    void getProperty(sdbusplus::bus::bus& bus, const std::string& service,
                     const std::string& path, const std::string& interface,
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
        catch (const sdbusplus::exception::SdBusError& ex)
        {
            log::log<log::level::ERR>("Get Property Failed",
                                      log::entry("WHAT=%s", ex.what()));
            throw;
        }

        prop = std::get<T>(result);
    }

  private:
    sdbusplus::bus::bus _bus;
};

} // namespace pid_control
