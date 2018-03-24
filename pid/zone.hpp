#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "conf.hpp"
#include "controller.hpp"
#include "sensors/sensor.hpp"
#include "sensors/manager.hpp"

#include "xyz/openbmc_project/Control/Mode/server.hpp"
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>


template <typename... T>
using ServerObject = typename sdbusplus::server::object::object<T...>;
using ModeInterface =
    sdbusplus::xyz::openbmc_project::Control::server::Mode;
using ModeObject = ServerObject<ModeInterface>;

/*
 * The PIDZone inherits from the Mode object so that it can listen for control
 * mode changes.  It primarily holds all PID loops and holds the sensor value
 * cache that's used per iteration of the PID loops.
 */
class PIDZone : public ModeObject
{
    public:
        PIDZone(int64_t zone,
                float minThermalRpm,
                float failSafePercent,
                std::shared_ptr<SensorManager> mgr,
                sdbusplus::bus::bus& bus,
                const char* objPath,
                bool defer)
            : ModeObject(bus, objPath, defer),
              _zoneId(zone),
              _maximumRPMSetPt(),
              _minThermalRpmSetPt(minThermalRpm),
              _failSafePercent(failSafePercent),
              _mgr(mgr)
        {
#ifdef __TUNING_LOGGING__
            _log.open("/tmp/swampd.log");
#endif
        }

        float getMaxRPMRequest(void) const;
        bool getManualMode(void) const;

        /* Could put lock around this since it's accessed from two threads, but
         * only one reader/one writer.
         */
        void setManualMode(bool mode);
        bool getFailSafeMode(void) const;
        int64_t getZoneId(void) const;
        void addRPMSetPoint(float setpoint);
        void clearRPMSetPoints(void);
        float getFailSafePercent(void) const;
        float getMinThermalRpmSetPt(void) const;

        std::unique_ptr<Sensor>& getSensor(std::string name);
        void determineMaxRPMRequest(void);
        void updateFanTelemetry(void);
        void updateSensors(void);
        void initializeCache(void);
        void dumpCache(void);
        void process_fans(void);
        void process_thermals(void);

        void addFanPID(std::unique_ptr<PIDController> pid);
        void addThermalPID(std::unique_ptr<PIDController> pid);
        double getCachedValue(const std::string& name);
        void addFanInput(std::string fan);
        void addThermalInput(std::string therm);

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
        std::shared_ptr<SensorManager> _mgr;

        std::vector<std::unique_ptr<PIDController>> _fans;
        std::vector<std::unique_ptr<PIDController>> _thermals;
};

std::map<int64_t, std::shared_ptr<PIDZone>> BuildZones(
            std::map<int64_t, PIDConf>& ZonePids,
            std::map<int64_t, struct zone>& ZoneConfigs,
            std::shared_ptr<SensorManager> mgmr,
            sdbusplus::bus::bus& ModeControlBus);

std::map<int64_t, std::shared_ptr<PIDZone>> BuildZonesFromConfig(
            std::string& path,
            std::shared_ptr<SensorManager> mgmr,
            sdbusplus::bus::bus& ModeControlBus);
