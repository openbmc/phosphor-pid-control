#pragma once

#include <limits>
#include <sdbusplus/bus.hpp>

struct SensorProperties
{
    int64_t scale;
    double value;
    std::string unit;
};

struct SensorThresholds
{
    double lowerThreshold = std::numeric_limits<double>::quiet_NaN();
    double upperThreshold = std::numeric_limits<double>::quiet_NaN();
};

const std::string sensorintf = "xyz.openbmc_project.Sensor.Value";
const std::string criticalThreshInf =
    "xyz.openbmc_project.Sensor.Threshold.Critical";
const std::string propertiesintf = "org.freedesktop.DBus.Properties";

class DbusHelperInterface
{
  public:
    virtual ~DbusHelperInterface() = default;

    /** @brief Get the service providing the interface for the path.
     */
    virtual std::string getService(sdbusplus::bus::bus& bus,
                                   const std::string& intf,
                                   const std::string& path) = 0;

    /** @brief Get all Sensor.Value properties for a service and path.
     *
     * @param[in] bus - A bus to use for the call.
     * @param[in] service - The service providing the interface.
     * @param[in] path - The dbus path.
     * @param[out] prop - A pointer to a properties struct to fill out.
     */
    virtual void getProperties(sdbusplus::bus::bus& bus,
                               const std::string& service,
                               const std::string& path,
                               struct SensorProperties* prop) = 0;

    /** @brief Get Critical Threshold current assert status
     *
     * @param[in] bus - A bus to use for the call.
     * @param[in] service - The service providing the interface.
     * @param[in] path - The dbus path.
     */
    virtual bool thresholdsAsserted(sdbusplus::bus::bus& bus,
                                    const std::string& service,
                                    const std::string& path) = 0;
};

class DbusHelper : public DbusHelperInterface
{
  public:
    DbusHelper() = default;
    ~DbusHelper() = default;
    DbusHelper(const DbusHelper&) = default;
    DbusHelper& operator=(const DbusHelper&) = default;
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
};

std::string getSensorPath(const std::string& type, const std::string& id);
std::string getMatch(const std::string& type, const std::string& id);
bool validType(const std::string& type);

struct VariantToFloatVisitor
{
    template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value, float>
        operator()(const T& t) const
    {
        return static_cast<float>(t);
    }

    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value, float>
        operator()(const T& t) const
    {
        throw std::invalid_argument("Cannot translate type to float");
    }
};

struct VariantToDoubleVisitor
{
    template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value, double>
        operator()(const T& t) const
    {
        return static_cast<double>(t);
    }

    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value, double>
        operator()(const T& t) const
    {
        throw std::invalid_argument("Cannot translate type to double");
    }
};
