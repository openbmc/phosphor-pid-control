#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>
#include <sdbusplus/test/integration/mock_service.hpp>
#include <sdbusplus/test/integration/private_bus.hpp>
#include <xyz/openbmc_project/Control/FanPwm/mock_server.hpp>
#include <xyz/openbmc_project/Sensor/Value/mock_server.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>

using sdbusplus::SdBusDuration;
using sdbusplus::test::integration::MockObject;
using sdbusplus::test::integration::MockService;
using sdbusplus::test::integration::PrivateBus;
using sdbusplus::xyz::openbmc_project::Control::server::MockFanPwm;
using sdbusplus::xyz::openbmc_project::Sensor::server::MockValue;

using SensorProperties = std::map<std::string, MockValue::PropertiesVariant>;
using FanPwmProperties = std::map<std::string, MockFanPwm::PropertiesVariant>;
/** The list of mock interfaces is passed as the template parameter
 * to this object.
 */
using MockFanObjectBase = sdbusplus::server::object_t<MockValue, MockFanPwm>;
using FanSensorValChangeHandler =
    std::function<void(std::string, double, double, double)>;
using FanPwmChangeHandler =
    std::function<void(std::string, double, double, double)>;

class FanObject : public MockObject
{
  public:
    /** Constructs a fan object.
     * @see FanService.addFan()
     */
    FanObject(const std::string& path, const SensorProperties& sensorVals,
              const FanPwmProperties& fanPwmVals,
              FanSensorValChangeHandler fanSensorValChangeHanlder,
              FanPwmChangeHandler fanPWMChangeHandler);

    FanObject(const std::string& path, const SensorProperties& sensorVals,
              const FanPwmProperties& fanPwmVals,
              FanSensorValChangeHandler fanSensorValChangeHanlder);

    ~FanObject();

    /** Overrides the start method in base class to initialize the fan
     * mockBase object.
     * It also registers signal handlers to catch the properties changes.
     */
    void start(sdbusplus::bus::bus& bus) override;

    std::shared_ptr<MockFanObjectBase> getMockBase();

    void changeFanSensorVal(double percentage);

  private:
    void catchFanTargetChange(sdbusplus::message::message& message);

    void catchFanSensorValChange(sdbusplus::message::message& message);

    void defaultOnFanPWMChange(std::string path, double pTarget,
                               double newTarget, double targetRange);

    const SensorProperties& initialSensorVals;
    const FanPwmProperties& initialFanPwmVals;

    FanSensorValChangeHandler onFanSensorValChange;
    FanPwmChangeHandler onFanPWMChange;

    std::shared_ptr<sdbusplus::bus::match::match> fanTargetPropsMatch;
    std::shared_ptr<sdbusplus::bus::match::match> fanSensorPropsMatch;
    /** The reference to sdbusplus object that implements mock interfaces.
     */
    std::shared_ptr<MockFanObjectBase> mockBase;
    uint64_t prevTarget;
    double prevValue;
    double sensorValueRange;
    static const uint64_t maxPWMTarget = 255;
    static const uint64_t minPWMTarget = 0;
};

/** This service and its associated object is a special implementation of
 * sensor.Value interface for Fans. It also implements Control.FanPwm interface.
 */
class FanService : public MockService
{
  public:
    /** Constructs this service by passing the service name, a pointer to the
     * private bus, and the active duration of the service.
     *
     * Note that the service does not start on D-Bus in constructor.
     * @see MockService
     */
    FanService(std::string name, std::shared_ptr<PrivateBus> privateBus,
               SdBusDuration microsecondsToRun);

    /** Adds a new fan object to this service.
     *
     * @param objectPath - The path of the object on D-Bus.
     * @param sensorVals - initial sensor values.
     * @param fanPwmVals - initial values for FanPwm properties.
     * @param onFanSensorValChange - The function pointer to call in response
     * to changes in the fan sensor value.
     * @param onFanPWMChange - The function pointer to call in response
     * to changes in the fan control target value.
     * @see FanObject.defaultOnFanPWMChange for an example.
     */
    void addFan(const std::string& objectPath,
                const SensorProperties& sensorVals,
                const FanPwmProperties& fanPwmVals,
                FanSensorValChangeHandler onFanSensorValChange,
                FanPwmChangeHandler onFanPWMChange);

    /** Adds a new fan object to this service.
     *
     * @param objectPath - The path of the object on D-Bus.
     * @param sensorVals - initial sensor values.
     * @param fanPwmVals - initial values for FanPwm properties.
     * @param onFanSensorValChange - The function pointer to call in response
     * to changes in the fan sensor value.
     *
     * By calling this method, the default behavior in response to changes in
     * fan control target value is executed.
     * @see FanObject.defaultOnFanPWMChange
     */
    void addFan(const std::string& objectPath,
                const SensorProperties& sensorVals,
                const FanPwmProperties& fanPwmVals,
                FanSensorValChangeHandler onFanSensorValChange);

    FanObject& getObject(std::string path);

    FanObject& getMainObject();

    void changeSensorVal(double percentage);

    void changeSensorVal(double percentage, std::string objectPath);
};
