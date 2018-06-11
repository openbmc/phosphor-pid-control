AM_CPPFLAGS = -I$(top_srcdir)/ $(GTEST_CFLAGS) $(GMOCK_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS) \
	$(GTEST_CFLAGS) \
	$(GMOCK_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS)
AM_LDFLAGS = $(GTEST_LIBS) \
	$(PTHREAD_LIBS) \
	$(GMOCK_LIBS) \
	$(OESDK_TESTCASE_FLAGS) \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS)

# Run all 'check' test programs
check_PROGRAMS = sensor_manager_unittest sensor_pluggable_unittest \
 sensor_host_unittest pid_zone_unittest
TESTS = $(check_PROGRAMS)

# Until libconfig is mocked out or replaced, include it.
sensor_manager_unittest_SOURCES = sensor_manager_unittest.cpp
sensor_manager_unittest_LDADD = $(top_builddir)/sensors/manager.o

sensor_pluggable_unittest_SOURCES = sensor_pluggable_unittest.cpp
sensor_pluggable_unittest_LDADD = $(top_builddir)/sensors/pluggable.o

sensor_host_unittest_SOURCES = sensor_host_unittest.cpp
sensor_host_unittest_LDADD = $(top_builddir)/sensors/host.o

pid_zone_unittest_SOURCES = pid_zone_unittest.cpp
pid_zone_unittest_LDADD = $(top_builddir)/pid/ec/pid.o \
 $(top_builddir)/sensors/manager.o $(top_builddir)/pid/controller.o \
 $(top_builddir)/pid/zone.o
