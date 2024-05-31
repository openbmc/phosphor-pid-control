#pragma once

#include <cstdint>
#include <string>

namespace pid_control
{

struct SensorProperties
{
    int64_t scale = 0;
    double value = 0.0;
    double min = 0.0;
    double max = 0.0;
    std::string unit;
    bool available = false;
    bool unavailableAsFailed = false;
};

class DbusHelperInterface
{
  public:
    virtual ~DbusHelperInterface() = default;

    /** @brief Get the service providing the interface for the path.
     *
     * @warning Throws exception on dbus failure.
     */
    virtual std::string getService(const std::string& intf,
                                   const std::string& path) = 0;

    /** @brief Get all Sensor.Value properties for a service and path.
     *
     * @param[in] bus - A bus to use for the call.
     * @param[in] service - The service providing the interface.
     * @param[in] path - The dbus path.
     * @param[out] prop - A pointer to a properties to fill out.
     *
     * @warning Throws exception on dbus failure.
     */
    virtual void getProperties(const std::string& service,
                               const std::string& path,
                               SensorProperties* prop) = 0;

    /** @brief Get Critical Threshold current assert status
     *
     * @param[in] bus - A bus to use for the call.
     * @param[in] service - The service providing the interface.
     * @param[in] path - The dbus path.
     */
    virtual bool thresholdsAsserted(const std::string& service,
                                    const std::string& path) = 0;
};

} // namespace pid_control
