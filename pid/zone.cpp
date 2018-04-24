/**
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Configuration. */
#include "conf.hpp"

#include "zone.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <libconfig.h++>
#include <memory>

#include "pid/controller.hpp"
#include "pid/fancontroller.hpp"
#include "pid/thermalcontroller.hpp"
#include "pid/ec/pid.hpp"


using tstamp = std::chrono::high_resolution_clock::time_point;
using namespace std::literals::chrono_literals;

static constexpr bool deferSignals = true;
static constexpr auto objectPath = "/xyz/openbmc_project/settings/fanctrl/zone";

float PIDZone::getMaxRPMRequest(void) const
{
    return _maximumRPMSetPt;
}

bool PIDZone::getManualMode(void) const
{
    return _manualMode;
}

void PIDZone::setManualMode(bool mode)
{
    _manualMode = mode;
}

bool PIDZone::getFailSafeMode(void) const
{
    // If any keys are present at least one sensor is in fail safe mode.
    return !_failSafeSensors.empty();
}

int64_t PIDZone::getZoneId(void) const
{
    return _zoneId;
}

void PIDZone::addRPMSetPoint(float setpoint)
{
    _RPMSetPoints.push_back(setpoint);
}

void PIDZone::clearRPMSetPoints(void)
{
    _RPMSetPoints.clear();
}

float PIDZone::getFailSafePercent(void) const
{
    return _failSafePercent;
}

float PIDZone::getMinThermalRpmSetPt(void) const
{
    return _minThermalRpmSetPt;
}

void PIDZone::addFanPID(std::unique_ptr<PIDController> pid)
{
    _fans.push_back(std::move(pid));
}

void PIDZone::addThermalPID(std::unique_ptr<PIDController> pid)
{
    _thermals.push_back(std::move(pid));
}

double PIDZone::getCachedValue(const std::string& name)
{
    return _cachedValuesByName.at(name);
}

void PIDZone::addFanInput(std::string fan)
{
    _fanInputs.push_back(fan);
}

void PIDZone::addThermalInput(std::string therm)
{
    _thermalInputs.push_back(therm);
}

void PIDZone::determineMaxRPMRequest(void)
{
    float max = 0;
    std::vector<float>::iterator result;

    if (_RPMSetPoints.size() > 0)
    {
        result = std::max_element(_RPMSetPoints.begin(), _RPMSetPoints.end());
        max = *result;
    }

    /*
     * If the maximum RPM set-point output is below the minimum RPM
     * set-point, set it to the minimum.
     */
    max = std::max(getMinThermalRpmSetPt(), max);

#ifdef __TUNING_LOGGING__
    /*
     * We received no set-points from thermal sensors.
     * This is a case experienced during tuning where they only specify
     * fan sensors and one large fan PID for all the fans.
     */
    static constexpr auto setpointpath = "/etc/thermal.d/set-point";
    try
    {
        int value;
        std::ifstream ifs;
        ifs.open(setpointpath);
        if (ifs.good()) {
            ifs >> value;
            max = value; // expecting RPM set-point, not pwm%
        }
    }
    catch (const std::exception& e)
    {
        /* This exception is uninteresting. */
        std::cerr << "Unable to read from '" << setpointpath << "'\n";
    }
#endif

    _maximumRPMSetPt = max;
    return;
}

#ifdef __TUNING_LOGGING__
void PIDZone::initializeLog(void)
{
    /* Print header for log file:
     * epoch_ms,setpt,fan1,fan2,fanN,sensor1,sensor2,sensorN,failsafe
     */

    _log << "epoch_ms,setpt";

    for (auto& f : _fanInputs)
    {
        _log << "," << f;
    }
    for (auto& t : _thermalInputs)
    {
        _log << "," << t;
    }
    _log << ",failsafe";
    _log << std::endl;

    return;
}

std::ofstream& PIDZone::getLogHandle(void)
{
    return _log;
}
#endif

/*
 * TODO(venture) This is effectively updating the cache and should check if the
 * values they're using to update it are new or old, or whatnot.  For instance,
 * if we haven't heard from the host in X time we need to detect this failure.
 *
 * I haven't decided if the Sensor should have a lastUpdated method or whether
 * that should be for the ReadInterface or etc...
 */

/**
 * We want the PID loop to run with values cached, so this will get all the
 * fan tachs for the loop.
 */
void PIDZone::updateFanTelemetry(void)
{
    /* TODO(venture): Should I just make _log point to /dev/null when logging
     * is disabled?  I think it's a waste to try and log things even if the
     * data is just being dropped though.
     */
#ifdef __TUNING_LOGGING__
    tstamp now = std::chrono::high_resolution_clock::now();
    _log << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    _log << "," << _maximumRPMSetPt;
#endif

    for (auto& f : _fanInputs)
    {
        auto& sensor = _mgr->getSensor(f);
        ReadReturn r = sensor->read();
        _cachedValuesByName[f] = r.value;

        /*
         * TODO(venture): We should check when these were last read.
         * However, these are the fans, so if I'm not getting updated values
         * for them... what should I do?
         */
#ifdef __TUNING_LOGGING__
        _log << "," << r.value;
#endif
    }

#ifdef __TUNING_LOGGING__
    for (auto& t : _thermalInputs)
    {
        _log << "," << _cachedValuesByName[t];
    }
#endif

    return;
}

void PIDZone::updateSensors(void)
{
    using namespace std::chrono;
    /* margin and temp are stored as temp */
    tstamp now = high_resolution_clock::now();

    for (auto& t : _thermalInputs)
    {
        auto& sensor = _mgr->getSensor(t);
        ReadReturn r = sensor->read();
        int64_t timeout = sensor->GetTimeout();

        _cachedValuesByName[t] = r.value;
        tstamp then = r.updated;

        /* Only go into failsafe if the timeout is set for
         * the sensor.
         */
        if (timeout > 0)
        {
            auto duration = duration_cast<std::chrono::seconds>
                    (now - then).count();
            auto period = std::chrono::seconds(timeout).count();
            if (duration >= period)
            {
                //std::cerr << "Entering fail safe mode.\n";
                _failSafeSensors.insert(t);
            }
            else
            {
                // Check if it's in there: remove it.
                auto kt = _failSafeSensors.find(t);
                if (kt != _failSafeSensors.end())
                {
                    _failSafeSensors.erase(kt);
                }
            }
        }
    }

    return;
}

void PIDZone::initializeCache(void)
{
    for (auto& f : _fanInputs)
    {
        _cachedValuesByName[f] = 0;
    }

    for (auto& t : _thermalInputs)
    {
        _cachedValuesByName[t] = 0;

        // Start all sensors in fail-safe mode.
        _failSafeSensors.insert(t);
    }
}

void PIDZone::dumpCache(void)
{
    std::cerr << "Cache values now: \n";
    for (auto& k : _cachedValuesByName)
    {
        std::cerr << k.first << ": " << k.second << "\n";
    }
}

void PIDZone::process_fans(void)
{
    for (auto& p : _fans)
    {
        p->pid_process();
    }
}

void PIDZone::process_thermals(void)
{
    for (auto& p : _thermals)
    {
        p->pid_process();
    }
}

std::unique_ptr<Sensor>& PIDZone::getSensor(std::string name)
{
    return _mgr->getSensor(name);
}

bool PIDZone::manual(bool value)
{
    std::cerr << "manual: " << value << std::endl;
    setManualMode(value);
    return ModeObject::manual(value);
}

bool PIDZone::failSafe() const
{
    return getFailSafeMode();
}

static std::string GetControlPath(int64_t zone)
{
    return std::string(objectPath) + std::to_string(zone);
}

std::map<int64_t, std::shared_ptr<PIDZone>> BuildZones(
            std::map<int64_t, PIDConf>& ZonePids,
            std::map<int64_t, struct zone>& ZoneConfigs,
            std::shared_ptr<SensorManager> mgr,
            sdbusplus::bus::bus& ModeControlBus)
{
    std::map<int64_t, std::shared_ptr<PIDZone>> zones;

    for (auto& zi : ZonePids)
    {
        auto zoneId = static_cast<int64_t>(zi.first);
        /* The above shouldn't be necessary but is, and I am having trouble
         * locating my notes on why.  If I recall correctly it was casting it
         * down to a byte in at least some cases causing weird behaviors.
         */

        auto zoneConf = ZoneConfigs.find(zoneId);
        if (zoneConf == ZoneConfigs.end())
        {
            /* The Zone doesn't have a configuration, bail. */
            static constexpr auto err =
                "Bailing during load, missing Zone Configuration";
            std::cerr << err << std::endl;
            throw std::runtime_error(err);
        }

        PIDConf& PIDConfig = zi.second;

        auto zone = std::make_shared<PIDZone>(
                        zoneId,
                        zoneConf->second.minthermalrpm,
                        zoneConf->second.failsafepercent,
                        mgr,
                        ModeControlBus,
                        GetControlPath(zi.first).c_str(),
                        deferSignals);

        zones[zoneId] = zone;

        std::cerr << "Zone Id: " << zone->getZoneId() << "\n";

        // For each PID create a Controller and a Sensor.
        PIDConf::iterator pit = PIDConfig.begin();
        while (pit != PIDConfig.end())
        {
            std::vector<std::string> inputs;
            std::string name = pit->first;
            struct controller_info* info = &pit->second;

            std::cerr << "PID name: " << name << "\n";

            /*
             * TODO(venture): Need to check if input is known to the
             * SensorManager.
             */
            if (info->type == "fan")
            {
                for (auto i : info->inputs)
                {
                    inputs.push_back(i);
                    zone->addFanInput(i);
                }

                auto pid = FanController::CreateFanPid(
                               zone,
                               name,
                               inputs,
                               info->info);
                zone->addFanPID(std::move(pid));
            }
            else if (info->type == "temp" || info->type == "margin")
            {
                for (auto i : info->inputs)
                {
                    inputs.push_back(i);
                    zone->addThermalInput(i);
                }

                auto pid = ThermalController::CreateThermalPid(
                               zone,
                               name,
                               inputs,
                               info->setpoint,
                               info->info);
                zone->addThermalPID(std::move(pid));
            }

            std::cerr << "inputs: ";
            for (auto& i : inputs)
            {
                std::cerr << i << ", ";
            }
            std::cerr << "\n";

            ++pit;
        }

        zone->emit_object_added();
    }

    return zones;
}

std::map<int64_t, std::shared_ptr<PIDZone>> BuildZonesFromConfig(
            std::string& path,
            std::shared_ptr<SensorManager> mgr,
            sdbusplus::bus::bus& ModeControlBus)
{
    using namespace libconfig;
    // zone -> pids
    std::map<int64_t, PIDConf> pidConfig;
    // zone -> configs
    std::map<int64_t, struct zone> zoneConfig;

    std::cerr << "entered BuildZonesFromConfig\n";

    Config cfg;

    /* The load was modeled after the example source provided. */
    try
    {
        cfg.readFile(path.c_str());
    }
    catch (const FileIOException& fioex)
    {
        std::cerr << "I/O error while reading file: " << fioex.what() << std::endl;
        throw;
    }
    catch (const ParseException& pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                  << " - " << pex.getError() << std::endl;
        throw;
    }

    try
    {
        const Setting& root = cfg.getRoot();
        const Setting& zones = root["zones"];
        int count = zones.getLength();

        /* For each zone. */
        for (int i = 0; i < count; ++i)
        {
            const Setting& zoneSettings = zones[i];

            int id;
            PIDConf thisZone;
            struct zone thisZoneConfig;

            zoneSettings.lookupValue("id", id);

            thisZoneConfig.minthermalrpm =
                    zoneSettings.lookup("minthermalrpm");
            thisZoneConfig.failsafepercent =
                    zoneSettings.lookup("failsafepercent");

            const Setting& pids = zoneSettings["pids"];
            int pidCount = pids.getLength();

            for (int j = 0; j < pidCount; ++j)
            {
                const Setting& pid = pids[j];

                std::string name;
                controller_info info;

                /*
                 * Mysteriously if you use lookupValue on these, and the type
                 * is float.  It won't work right.
                 *
                 * If the configuration file value doesn't look explicitly like
                 * a float it won't let you assign it to one.
                 */
                name = pid.lookup("name").c_str();
                info.type = pid.lookup("type").c_str();
                /* set-point is only required to be set for thermal. */
                /* TODO(venture): Verify this works optionally here. */
                info.setpoint = pid.lookup("set-point");
                info.info.ts = pid.lookup("pid.sampleperiod");
                info.info.p_c = pid.lookup("pid.p_coefficient");
                info.info.i_c = pid.lookup("pid.i_coefficient");
                info.info.ff_off = pid.lookup("pid.ff_off_coefficient");
                info.info.ff_gain = pid.lookup("pid.ff_gain_coefficient");
                info.info.i_lim.min = pid.lookup("pid.i_limit.min");
                info.info.i_lim.max = pid.lookup("pid.i_limit.max");
                info.info.out_lim.min = pid.lookup("pid.out_limit.min");
                info.info.out_lim.max = pid.lookup("pid.out_limit.max");
                info.info.slew_neg = pid.lookup("pid.slew_neg");
                info.info.slew_pos = pid.lookup("pid.slew_pos");

                std::cerr << "out_lim.min: " << info.info.out_lim.min << "\n";
                std::cerr << "out_lim.max: " << info.info.out_lim.max << "\n";

                const Setting& inputs = pid["inputs"];
                int icount = inputs.getLength();

                for (int z = 0; z < icount; ++z)
                {
                    std::string v;
                    v = pid["inputs"][z].c_str();
                    info.inputs.push_back(v);
                }

                thisZone[name] = info;
            }

            pidConfig[static_cast<int64_t>(id)] = thisZone;
            zoneConfig[static_cast<int64_t>(id)] = thisZoneConfig;
        }
    }
    catch (const SettingTypeException &setex)
    {
        std::cerr << "Setting '" << setex.getPath() << "' type exception!" << std::endl;
        throw;
    }
    catch (const SettingNotFoundException& snex)
    {
        std::cerr << "Setting '" << snex.getPath() << "' not found!" << std::endl;
        throw;
    }

    return BuildZones(pidConfig, zoneConfig, mgr, ModeControlBus);
}
