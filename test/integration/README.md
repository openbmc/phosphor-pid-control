# D-Bus Mocking for Integration Testing - Example

This directory includes an example of using the sdbusplus test infrastructure to
support mocking D-Bus services for integration testing as discussed in
[gerrit/37378](https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/37378).
In this example, mock temperature and fan sensor services are run on a D-Bus
that is shared with an instance of swampd daemon.
The swampd capabilities in adjusting the temperature by controlling the fan
sensor can be evaluated.

## Files

- common (note: This directory is the temporary place for common utilities that
can be used to run integration tests on other OpenBMC daemons too.
The appropriate place for these functions is TBD.)
	- common/mapperd_manager
		- Manages mapperd daemon.
	- common/integration_test
		- This is a base class to share utilities that can be used by all
        integration tests.
        - It will start the mapperx daemon upon construction.
		- The timed loop will start by calling `runFor()`.
		- Simple bus related expectations, such as expecting signals, can be
        added. The expectations can be evaluated by the number of times they met
        using functions such as `exactly`, `atLeast`, and `atMost`.
	- common/services
		- common/services/sensor_server
			- This is an example of an active mock sensor. It will increase the
            temperature sensor value each second.
			- Users add objects through the `addSensor()` function.
			- The service will request its name upon construction.
			- The object will be registered on D-Bus upon construction.
            - The sensor object implements Sensor.Value D-Bus interface.
		- common/services/fan_server
			- This is an example of a non-active mock sensor. The objects in fan
            sensor only react to D-Bus signals such as changes in value/target.
			- User can customize the action in response to the signals.
			- A default action in response to changes in FanPwm.Target exists.
            - Users add objects through the `addFan()` function.
            - The fan object implements Sensor.Value and Control.FanPwm D-Bus
            interfaces.
- swampd_test
	- This is a class to share functionalities used by all tests on swampd.
	- It allows the user to start services and add objects.
	- The swampd daemon will start after a call to `startSwampd`.
    - A default implementation for the action in response to changes in Fan
    Sensor value is provided.
    - In the default implementation, swampd changes FanPwm target, the fan
    sensor changes the sensor value in same direction with the same rate, and
    the temperature changes its sensor value in the oppposite direction with
    the same rate.
- fan_control_itest
	- This file includes example tests that are run using gtest.
