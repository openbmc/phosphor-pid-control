#pragma once

#include "conf.hpp"
#include "controller.hpp"
#include "pidcontroller.hpp"
#include "sensors/manager.hpp"
#include "sensors/sensor.hpp"
#include "tuning.hpp"
#include "zone_interface.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Control/Mode/server.hpp>

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

template <typename... T>
using ServerObject = typename sdbusplus::server::object::object<T...>;
using ModeInterface = sdbusplus::xyz::openbmc_project::Control::server::Mode;
using ModeObject = ServerObject<ModeInterface>;

namespace pid_control
{

/*
 * The DbusPidZone inherits from the Mode object so that it can listen for
 * control mode changes.  It primarily holds all PID loops and holds the sensor
 * value cache that's used per iteration of the PID loops.
 */
class DbusPidZone : public ZoneInterface, public ModeObject
{
  public:
    DbusPidZone(int64_t zone, double minThermalOutput, double failSafePercent,
                const SensorManager& mgr, sdbusplus::bus::bus& bus,
                const char* objPath, bool defer) :
        ModeObject(bus, objPath, defer),
        _zoneId(zone), _maximumSetPoint(),
        _minThermalOutputSetPt(minThermalOutput),
        _failSafePercent(failSafePercent), _mgr(mgr)
    {
        if (loggingEnabled)
        {
            _log.open(loggingPath + "/zone_" + std::to_string(zone) + ".log");
        }
    }

    double getMaxSetPointRequest(void) const override;
    bool getManualMode(void) const;

    /* Could put lock around this since it's accessed from two threads, but
     * only one reader/one writer.
     */
    void setManualMode(bool mode);
    bool getFailSafeMode(void) const override;
    int64_t getZoneID(void) const;
    void addSetPoint(double setpoint) override;
    void addRPMCeiling(double ceiling) override;
    void clearSetPoints(void);
    void clearRPMCeilings(void);
    double getFailSafePercent(void) const override;
    double getMinThermalSetpoint(void) const;

    Sensor* getSensor(const std::string& name) override;
    void determineMaxSetPointRequest(void);
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

    void initializeLog(void);
    std::ofstream& getLogHandle(void);

    /* Method for setting the manual mode over dbus */
    bool manual(bool value) override;
    /* Method for reading whether in fail-safe mode over dbus */
    bool failSafe() const override;

  private:
    std::ofstream _log;

    const int64_t _zoneId;
    double _maximumSetPoint = 0;
    bool _manualMode = false;
    const double _minThermalOutputSetPt;
    const double _failSafePercent;

    std::set<std::string> _failSafeSensors;

    std::vector<double> _SetPoints;
    std::vector<double> _RPMCeilings;
    std::vector<std::string> _fanInputs;
    std::vector<std::string> _thermalInputs;
    std::map<std::string, double> _cachedValuesByName;
    const SensorManager& _mgr;

    std::vector<std::unique_ptr<Controller>> _fans;
    std::vector<std::unique_ptr<Controller>> _thermals;
};

} // namespace pid_control
