#pragma once

#include "conf.hpp"
#include "controller.hpp"
#include "pidcontroller.hpp"
#include "sensors/manager.hpp"
#include "sensors/sensor.hpp"

#include <fstream>
#include <map>
#include <memory>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <set>
#include <string>
#include <vector>
#include <xyz/openbmc_project/Control/Mode/server.hpp>

template <typename... T>
using ServerObject = typename sdbusplus::server::object::object<T...>;
using ModeInterface = sdbusplus::xyz::openbmc_project::Control::server::Mode;
using ModeObject = ServerObject<ModeInterface>;

class ZoneInterface
{
  public:
    virtual ~ZoneInterface() = default;

    virtual double getCachedValue(const std::string& name) = 0;
    virtual void addRPMSetPoint(float setpoint) = 0;
    virtual float getMaxRPMRequest() const = 0;
    virtual bool getFailSafeMode() const = 0;
    virtual float getFailSafePercent() const = 0;
    virtual Sensor* getSensor(const std::string& name) = 0;
};

/*
 * The PIDZone inherits from the Mode object so that it can listen for control
 * mode changes.  It primarily holds all PID loops and holds the sensor value
 * cache that's used per iteration of the PID loops.
 */
class PIDZone : public ZoneInterface, public ModeObject
{
  public:
    PIDZone(int64_t zone, float minThermalRpm, float failSafePercent,
            const SensorManager& mgr, sdbusplus::bus::bus& bus,
            const char* objPath, bool defer) :
        ModeObject(bus, objPath, defer),
        _zoneId(zone), _maximumRPMSetPt(), _minThermalRpmSetPt(minThermalRpm),
        _failSafePercent(failSafePercent), _mgr(mgr)
    {
#ifdef __TUNING_LOGGING__
        _log.open("/tmp/swampd.log");
#endif
    }

    float getMaxRPMRequest(void) const override;
    bool getManualMode(void) const;

    /* Could put lock around this since it's accessed from two threads, but
     * only one reader/one writer.
     */
    void setManualMode(bool mode);
    bool getFailSafeMode(void) const override;
    int64_t getZoneID(void) const;
    void addRPMSetPoint(float setpoint) override;
    void clearRPMSetPoints(void);
    float getFailSafePercent(void) const override;
    float getMinThermalRpmSetPt(void) const;

    Sensor* getSensor(const std::string& name) override;
    void determineMaxRPMRequest(void);
    void updateFanTelemetry(void);
    void updateSensors(void);
    void initializeCache(void);
    void dumpCache(void);
    void processFans(void);
    void processThermals(void);

    void addFanPID(std::unique_ptr<Controller> pid);
    void addThermalPID(std::unique_ptr<Controller> pid);
    double getCachedValue(const std::string& name) override;
    void addFanInput(const std::string& fan);
    void addThermalInput(const std::string& therm);

#ifdef __TUNING_LOGGING__
    void initializeLog(void);
    std::ofstream& getLogHandle(void);
#endif

    /* Method for setting the manual mode over dbus */
    bool manual(bool value) override;
    /* Method for reading whether in fail-safe mode over dbus */
    bool failSafe() const override;

  private:
#ifdef __TUNING_LOGGING__
    std::ofstream _log;
#endif

    const int64_t _zoneId;
    float _maximumRPMSetPt = 0;
    bool _manualMode = false;
    const float _minThermalRpmSetPt;
    const float _failSafePercent;

    std::set<std::string> _failSafeSensors;

    std::vector<float> _RPMSetPoints;
    std::vector<std::string> _fanInputs;
    std::vector<std::string> _thermalInputs;
    std::map<std::string, double> _cachedValuesByName;
    const SensorManager& _mgr;

    std::vector<std::unique_ptr<Controller>> _fans;
    std::vector<std::unique_ptr<Controller>> _thermals;
};
