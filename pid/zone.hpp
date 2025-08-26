#pragma once

#include "conf.hpp"
#include "controller.hpp"
#include "failsafeloggers/failsafe_logger_utility.hpp"
#include "interfaces.hpp"
#include "manual_mode_manager.hpp"
#include "pidcontroller.hpp"
#include "sensors/manager.hpp"
#include "sensors/sensor.hpp"
#include "tuning.hpp"
#include "zone_interface.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Control/Mode/server.hpp>
#include <xyz/openbmc_project/Debug/Pid/ThermalPower/server.hpp>
#include <xyz/openbmc_project/Debug/Pid/Zone/server.hpp>
#include <xyz/openbmc_project/Object/Enable/server.hpp>

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

template <typename... T>
using ServerObject = typename sdbusplus::server::object_t<T...>;
using ModeInterface = sdbusplus::xyz::openbmc_project::Control::server::Mode;
using DebugZoneInterface =
    sdbusplus::xyz::openbmc_project::Debug::Pid::server::Zone;
using ModeObject = ServerObject<ModeInterface, DebugZoneInterface>;
using ProcessInterface =
    sdbusplus::xyz::openbmc_project::Object::server::Enable;
using DebugThermalPowerInterface =
    sdbusplus::xyz::openbmc_project::Debug::Pid::server::ThermalPower;
using ProcessObject =
    ServerObject<ProcessInterface, DebugThermalPowerInterface>;
using FailSafeSensorsMap =
    std::map<std::string, std::pair<std::string, double>>;
using FailSafeSensorPair =
    std::pair<std::string, std::pair<std::string, double>>;

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
                conf::CycleTime cycleTime, const SensorManager& mgr,
                sdbusplus::bus_t& bus, const char* objPath, bool defer,
                bool accumulateSetPoint, ManualModeManager& manualMgr) :
        ModeObject(bus, objPath,
                   defer ? ModeObject::action::defer_emit
                         : ModeObject::action::emit_object_added),
        _zoneId(zone), _accumulateSetPoint(accumulateSetPoint),
        _minThermalOutputSetPt(minThermalOutput),
        _zoneFailSafePercent(failSafePercent), _cycleTime(cycleTime), _mgr(mgr),
        _manualMgr(manualMgr)
    {
        if (loggingEnabled)
        {
            _log.open(loggingPath + "/zone_" + std::to_string(zone) + ".log");
        }

        _manualMode = _manualMgr.get(_zoneId);
        ModeObject::manual(_manualMode);

        if (!_manualMode)
        {
            _redundantWrite = true;
        }
    }

    bool getManualMode(void) const override;
    /* Could put lock around this since it's accessed from two threads, but
     * only one reader/one writer.
     */

    bool getRedundantWrite(void) const override;
    void setManualMode(bool mode);
    bool getFailSafeMode(void) const override;
    void markSensorMissing(const std::string& name,
                           const std::string& failReason);
    bool getAccSetPoint(void) const override;

    int64_t getZoneID(void) const override;
    void addSetPoint(double setPoint, const std::string& name) override;
    double getMaxSetPointRequest(void) const override;
    void addRPMCeiling(double ceiling) override;
    void clearSetPoints(void) override;
    void clearRPMCeilings(void) override;
    double getFailSafePercent(void) override;
    FailSafeSensorsMap getFailSafeSensors(void) const override;
    double getMinThermalSetPoint(void) const;
    uint64_t getCycleIntervalTime(void) const override;
    uint64_t getUpdateThermalsCycle(void) const override;

    Sensor* getSensor(const std::string& name) override;
    std::vector<std::string> getSensorNames(void) override;
    void determineMaxSetPointRequest(void) override;
    void updateFanTelemetry(void) override;
    void updateSensors(void) override;
    void initializeCache(void) override;
    void setOutputCache(std::string_view, const ValueCacheEntry&) override;
    void dumpCache(void);

    void processFans(void) override;
    void processThermals(void) override;

    void addFanPID(std::unique_ptr<Controller> pid);
    void addThermalPID(std::unique_ptr<Controller> pid);
    double getCachedValue(const std::string& name) override;
    ValueCacheEntry getCachedValues(const std::string& name) override;

    void addFanInput(const std::string& fan, bool missingAcceptable);
    void addThermalInput(const std::string& therm, bool missingAcceptable);

    void initializeLog(void) override;
    void writeLog(const std::string& value) override;

    /* Method for setting the manual mode over dbus */
    bool manual(bool value) override;
    /* Method for reading whether in fail-safe mode over dbus */
    bool failSafe() const override;
    /* Method for recording the maximum SetPoint PID config name */
    std::string leader() const override;
    /* Method for control process for each loop at runtime */
    void addPidControlProcess(const std::string& name, const std::string& type,
                              double setpoint, sdbusplus::bus_t& bus,
                              const std::string& objPath, bool defer);
    bool isPidProcessEnabled(const std::string& name);

    void addPidFailSafePercent(const std::vector<std::string>& inputs,
                               double percent);

    void updateThermalPowerDebugInterface(std::string pidName,
                                          std::string leader, double input,
                                          double output) override;

  private:
    template <bool fanSensorLogging>
    void processSensorInputs(const std::vector<std::string>& sensorInputs,
                             std::chrono::high_resolution_clock::time_point now)
    {
        for (const auto& sensorInput : sensorInputs)
        {
            auto sensor = _mgr.getSensor(sensorInput);
            ReadReturn r = sensor->read();
            _cachedValuesByName[sensorInput] = {r.value, r.unscaled};
            int64_t timeout = sensor->getTimeout();
            std::chrono::high_resolution_clock::time_point then = r.updated;

            auto duration =
                std::chrono::duration_cast<std::chrono::seconds>(now - then)
                    .count();
            auto period = std::chrono::seconds(timeout).count();
            /*
             * TODO(venture): We should check when these were last read.
             * However, these are the fans, so if I'm not getting updated values
             * for them... what should I do?
             */
            if constexpr (fanSensorLogging)
            {
                if (loggingEnabled)
                {
                    const auto& v = _cachedValuesByName[sensorInput];
                    _log << "," << v.scaled << "," << v.unscaled;
                    const auto& p = _cachedFanOutputs[sensorInput];
                    _log << "," << p.scaled << "," << p.unscaled;
                }
            }

            if (debugEnabled)
            {
                std::cerr << sensorInput << " sensor reading: " << r.value
                          << "\n";
            }

            // check if fan fail.
            if (sensor->getFailed())
            {
                markSensorMissing(sensorInput, sensor->getFailReason());

                if (debugEnabled)
                {
                    std::cerr << sensorInput << " sensor get failed\n";
                }
            }
            else if (timeout != 0 && duration >= period)
            {
                markSensorMissing(sensorInput, "Sensor timeout");

                if (debugEnabled)
                {
                    std::cerr << sensorInput << " sensor timeout\n";
                }
                outputFailsafeLogWithZone(_zoneId, this->getFailSafeMode(),
                                          sensorInput,
                                          "The sensor has timed out.");
            }
            else
            {
                // Check if it's in there: remove it.
                auto kt = _failSafeSensors.find(sensorInput);
                if (kt != _failSafeSensors.end())
                {
                    if (debugEnabled)
                    {
                        std::cerr << sensorInput
                                  << " is erased from failsafe sensor set\n";
                    }

                    _failSafeSensors.erase(kt);
                    outputFailsafeLogWithZone(_zoneId, this->getFailSafeMode(),
                                              sensorInput,
                                              "The sensor has recovered.");
                }
            }
        }
    }

    std::ofstream _log;

    const int64_t _zoneId;
    double _maximumSetPoint = 0;
    std::string _maximumSetPointName;
    std::string _maximumSetPointNamePrev;
    bool _manualMode = false;
    bool _redundantWrite = false;
    bool _accumulateSetPoint = false;
    const double _minThermalOutputSetPt;
    // Zone fail safe Percent setting by configuration.
    const double _zoneFailSafePercent;
    const conf::CycleTime _cycleTime;

    /*
     * <map key = sensor name, value = sensor fail reason and failsafe percent>
     */
    FailSafeSensorsMap _failSafeSensors;
    std::set<std::string> _missingAcceptable;

    std::map<std::string, double> setPoints;
    std::vector<double> rpmCeilings;
    std::vector<std::string> _fanInputs;
    std::vector<std::string> _thermalInputs;
    std::map<std::string, ValueCacheEntry> _cachedValuesByName;
    std::map<std::string, ValueCacheEntry> _cachedFanOutputs;
    const SensorManager& _mgr;
    ManualModeManager& _manualMgr;

    std::vector<std::unique_ptr<Controller>> _fans;
    std::vector<std::unique_ptr<Controller>> _thermals;

    std::map<std::string, std::unique_ptr<ProcessObject>> _pidsControlProcess;
    /*
     * <key = sensor name, value = sensor failsafe percent>
     * sensor fail safe Percent setting by each pid controller configuration.
     */
    std::map<std::string, double> _sensorFailSafePercent;
};

} // namespace pid_control
