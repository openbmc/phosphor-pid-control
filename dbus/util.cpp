#include "dbus/util.hpp"

#include <cmath>
#include <iostream>
#include <set>

using Property = std::string;
using Value = sdbusplus::message::variant<int64_t, double, std::string, bool>;
using PropertyMap = std::map<Property, Value>;

/* TODO(venture): Basically all phosphor apps need this, maybe it should be a
 * part of sdbusplus.  There is an old version in libmapper.
 */
std::string DbusHelper::GetService(sdbusplus::bus::bus& bus,
                                   const std::string& intf,
                                   const std::string& path)
{
    auto mapper =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetObject");

    mapper.append(path);
    mapper.append(std::vector<std::string>({intf}));

    auto responseMsg = bus.call(mapper);
    if (responseMsg.is_method_error())
    {
        throw std::runtime_error("ObjectMapper Call Failure");
    }

    std::map<std::string, std::vector<std::string>> response;
    responseMsg.read(response);

    if (response.begin() == response.end())
    {
        throw std::runtime_error("Unable to find Object: " + path);
    }

    return response.begin()->first;
}

void DbusHelper::GetProperties(sdbusplus::bus::bus& bus,
                               const std::string& service,
                               const std::string& path,
                               struct SensorProperties* prop)
{
    auto pimMsg = bus.new_method_call(service.c_str(), path.c_str(),
                                      propertiesintf.c_str(), "GetAll");

    pimMsg.append(sensorintf);
    auto valueResponseMsg = bus.call(pimMsg);

    if (valueResponseMsg.is_method_error())
    {
        std::cerr << "Error in value call\n";
        throw std::runtime_error("ERROR in value call.");
    }

    // The PropertyMap returned will look like this because it's always
    // reading a Sensor.Value interface.
    // a{sv} 3:
    // "Value" x 24875
    // "Unit" s "xyz.openbmc_project.Sensor.Value.Unit.DegreesC"
    // "Scale" x -3
    PropertyMap propMap;
    valueResponseMsg.read(propMap);

    // If no error was set, the values should all be there.
    auto findUnit = propMap.find("Unit");
    if (findUnit != propMap.end())
    {
        prop->unit =
            sdbusplus::message::variant_ns::get<std::string>(findUnit->second);
    }
    auto findScale = propMap.find("Scale");
    if (findScale != propMap.end())
    {
        prop->scale =
            sdbusplus::message::variant_ns::get<int64_t>(findScale->second);
    }
    else
    {
        prop->scale = 0;
    }

    prop->value =
        mapbox::util::apply_visitor(VariantToDoubleVisitor(), propMap["Value"]);

    return;
}

bool DbusHelper::ThresholdsAsserted(sdbusplus::bus::bus& bus,
                                    const std::string& service,
                                    const std::string& path)
{

    auto critical = bus.new_method_call(service.c_str(), path.c_str(),
                                        propertiesintf.c_str(), "GetAll");
    critical.append(criticalThreshInf);
    PropertyMap criticalMap;

    try
    {
        auto msg = bus.call(critical);
        if (!msg.is_method_error())
        {
            msg.read(criticalMap);
        }
    }
    catch (sdbusplus::exception_t&)
    {
        // do nothing, sensors don't have to expose critical thresholds
        return false;
    }

    auto findCriticalLow = criticalMap.find("CriticalAlarmLow");
    auto findCriticalHigh = criticalMap.find("CriticalAlarmHigh");

    bool asserted = false;
    if (findCriticalLow != criticalMap.end())
    {
        asserted =
            sdbusplus::message::variant_ns::get<bool>(findCriticalLow->second);
    }

    // as we are catching properties changed, a sensor could theoretically jump
    // from one threshold to the other in one event, so check both thresholds
    if (!asserted && findCriticalHigh != criticalMap.end())
    {
        asserted =
            sdbusplus::message::variant_ns::get<bool>(findCriticalHigh->second);
    }
    return asserted;
}

std::string GetSensorPath(const std::string& type, const std::string& id)
{
    std::string layer = type;
    if (type == "fan")
    {
        layer = "fan_tach";
    }
    else if (type == "temp")
    {
        layer = "temperature";
    }
    else
    {
        layer = "unknown"; // TODO(venture): Need to handle.
    }

    return std::string("/xyz/openbmc_project/sensors/" + layer + "/" + id);
}

std::string GetMatch(const std::string& type, const std::string& id)
{
    return std::string("type='signal',"
                       "interface='org.freedesktop.DBus.Properties',"
                       "member='PropertiesChanged',"
                       "path='" +
                       GetSensorPath(type, id) + "'");
}

bool ValidType(const std::string& type)
{
    static std::set<std::string> valid = {"fan", "temp"};
    return (valid.find(type) != valid.end());
}
