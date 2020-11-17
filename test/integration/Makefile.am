top_integbuilddir = $(top_builddir)/test/integration
top_mockgendir = $(top_integbuilddir)/xyz/openbmc_project

AM_LDFLAGS = \
	$(GTEST_LIBS) \
	$(GMOCK_LIBS) \
	-lgmock_main \
	$(OESDK_TESTCASE_FLAGS) \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(CODE_COVERAGE_LIBS)

check_PROGRAMS = fan_control_itest

TESTS = $(check_PROGRAMS)

BUILT_SOURCES = $(dbus_interface_generated_files)

dbus_interface_generated_files = \
	$(top_mockgendir)/Sensor/Value/server.cpp \
	$(top_mockgendir)/Control/FanPwm/server.cpp \
	$(top_mockgendir)/Control/FanPwm/mock_server.hpp \
	$(top_mockgendir)/Sensor/Value/mock_server.hpp

$(dbus_interface_generated_files):
	sdbus++-gendir --tool sdbus++ --output $(top_integbuilddir) --gen-mocks xyz

integration_test_common_files = \
	common/mapperd_manager.cpp \
	common/integration_test.cpp \
	common/services/sensor_server.cpp \
	common/services/fan_server.cpp

fan_control_itest_SOURCES = \
	$(dbus_interface_generated_files) \
	$(integration_test_common_files) \
	swampd_test.cpp \
	fan_control_itest.cpp

CLEANFILES = \
	$(swampd_test_generated_files)
