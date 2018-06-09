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
check_PROGRAMS = sensor_manager_unittest pluggable_unittest
TESTS = $(check_PROGRAMS)

# Until libconfig is mocked out or replaced, include it.
sensor_manager_unittest_SOURCES = sensor_manager_unittest.cpp
sensor_manager_unittest_LDADD = $(top_builddir)/sensors/manager.o

pluggable_unittest_SOURCES = pluggable_unittest.cpp
pluggable_unittest_LDADD = $(top_builddir)/sensors/pluggable.o
