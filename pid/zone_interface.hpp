#pragma once

#include "sensors/sensor.hpp"

#include <string>

namespace pid_control
{

/**
 * In a Zone you have a set of PIDs which feed each other.  Fan PIDs are fed set
 * points from Thermal PIDs.
 */
class ZoneInterface
{
  public:
    virtual ~ZoneInterface() = default;

    /** Get Current Zone ID */
    virtual int64_t getZoneID(void) const = 0;

    /** If the zone implementation supports logging, initialize the log. */
    virtual void initializeLog(void) = 0;
    /** If the zone implementation supports logging, write string to log. */
    virtual void writeLog(const std::string& value) = 0;

    /** Return a pointer to the sensor specified by name. */
    virtual Sensor* getSensor(const std::string& name) = 0;

    /** Return the list of sensor names in the zone. */
    virtual std::vector<std::string> getSensorNames(void) = 0;

    /* updateFanTelemetry() and updateSensors() both clear the failsafe state
     * for a sensor if it's no longer in that state.
     */
    /** For each fan input in the zone, read each to update the cachedValue and
     * check if the fan is beyond its timeout to trigger a failsafe condition.
     */
    virtual void updateFanTelemetry(void) = 0;
    /** For each thermal input in the zone, read each to update the cachedValue
     * and check if the sensor is beyond its timeout to trigger a failsafe
     * condition.
     */
    virtual void updateSensors(void) = 0;
    /** For each fan and thermal input in the zone, set the cachedValue to 0 and
     * set the input as failsafe - to default the zone to failsafe before it
     * starts processing values to control fans.
     */
    virtual void initializeCache(void) = 0;

    /** Optionally adds fan outputs to an output cache, which is different
     * from the input cache accessed by getCachedValue(), so it is possible
     * to have entries with the same name in both the output cache and
     * the input cache. The output cache is used for logging, to show
     * the PWM values determined by the PID loop, next to the resulting RPM.
     */
    virtual void setOutputCache(std::string_view name,
                                const ValueCacheEntry& values) = 0;

    /** Return cached value for sensor by name. */
    virtual double getCachedValue(const std::string& name) = 0;
    /** Return cached values, both scaled and original unscaled values,
     * for sensor by name. Subclasses can add trivial return {value, value},
     * for subclasses that only implement getCachedValue() and do not care
     * about maintaining the distinction between scaled and unscaled values.
     */
    virtual ValueCacheEntry getCachedValues(const std::string& name) = 0;

    /** Add a set point value for the Max Set Point computation. */
    virtual void addSetPoint(double setpoint, const std::string& name) = 0;
    /** Clear all set points specified via addSetPoint */
    virtual void clearSetPoints(void) = 0;

    /** Add maximum RPM value to drive fan pids. */
    virtual void addRPMCeiling(double ceiling) = 0;
    /** Clear any RPM value set with addRPMCeiling. */
    virtual void clearRPMCeilings(void) = 0;

    /** Compute the value returned by getMaxSetPointRequest - called from the
     * looping mechanism before triggering any Fan PIDs. The value computed is
     * used by each fan PID.
     */
    virtual void determineMaxSetPointRequest(void) = 0;
    /** Given the set points added via addSetPoint, return the maximum value -
     * called from the PID loop that uses that value to drive the fans.
     */
    virtual double getMaxSetPointRequest() const = 0;

    /** Return if the zone has any sensors in fail safe mode. */
    virtual bool getFailSafeMode() const = 0;
    /** Return the rpm or pwm percent value to drive fan pids when zone is in
     * fail safe.
     */
    virtual double getFailSafePercent() = 0;

    /** Return the zone's cycle time settings */
    virtual uint64_t getCycleIntervalTime(void) const = 0;
    virtual uint64_t getUpdateThermalsCycle(void) const = 0;

    /** Return if the zone is set to manual mode.  false equates to automatic
     * mode (the default).
     */
    virtual bool getManualMode(void) const = 0;

    /** Returns true if a redundant fan PWM write is needed. Redundant write
     * is used when returning the fan to automatic mode from manual mode.
     */
    virtual bool getRedundantWrite(void) const = 0;

    /** Returns true if user wants to accumulate the output PWM of different
     * controllers with same sensor
     */
    virtual bool getAccSetPoint(void) const = 0;

    /** For each fan pid, do processing. */
    virtual void processFans(void) = 0;
    /** For each thermal pid, do processing. */
    virtual void processThermals(void) = 0;

    /** Update thermal/power debug dbus properties */
    virtual void updateThermalPowerDebugInterface(
        std::string pidName, std::string leader, double input,
        double output) = 0;
};

} // namespace pid_control
