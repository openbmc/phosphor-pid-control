#include <iostream>

#include "dbus/util.hpp"

using Property = std::string;
using Value = sdbusplus::message::variant<int64_t, std::string>;
using PropertyMap = std::map<Property, Value>;

/* TODO(venture): Basically all phosphor apps need this, maybe it should be a
 * part of sdbusplus.  There is an old version in libmapper.
 */
std::string GetService(sdbusplus::bus::bus& bus, const std::string& intf,
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

void GetProperties(sdbusplus::bus::bus& bus, const std::string& service,
                   const std::string& path, struct SensorProperties* prop)
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
    prop->unit =
        sdbusplus::message::variant_ns::get<std::string>(propMap["Unit"]);
    prop->scale =
        sdbusplus::message::variant_ns::get<int64_t>(propMap["Scale"]);
    prop->value =
        sdbusplus::message::variant_ns::get<int64_t>(propMap["Value"]);

    return;
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
