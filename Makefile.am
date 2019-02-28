AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = swampd setsensor

setsensor_SOURCES = setsensor.cpp
setsensor_LDADD = \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS)
setsensor_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

swampd_SOURCES = main.cpp util.cpp
swampd_LDADD = \
	$(SDBUSPLUS_LIBS) \
	$(PTHREAD_LIBS) \
	libswampd.la
swampd_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PTHREAD_CFLAGS)

noinst_LTLIBRARIES = libswampd.la
libswampd_la_LDFLAGS = -static
libswampd_la_LIBADD = \
	-lstdc++fs \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS)
libswampd_la_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	-flto

libswampd_la_SOURCES = \
	notimpl/readonly.cpp \
	notimpl/writeonly.cpp \
	dbus/util.cpp \
	dbus/dbuspassive.cpp \
	dbus/dbusactiveread.cpp \
	dbus/dbuswrite.cpp \
	sysfs/sysfsread.cpp \
	sysfs/sysfswrite.cpp \
	sysfs/util.cpp \
	sensors/pluggable.cpp \
	sensors/host.cpp \
	sensors/builder.cpp \
	sensors/buildjson.cpp \
	sensors/manager.cpp \
	pid/ec/pid.cpp \
	pid/ec/stepwise.cpp \
	pid/fancontroller.cpp \
	pid/thermalcontroller.cpp \
	pid/pidcontroller.cpp \
	pid/stepwisecontroller.cpp \
	pid/builder.cpp \
	pid/buildjson.cpp \
	pid/zone.cpp \
	pid/util.cpp \
	pid/pidthread.cpp \
	pid/tuning.cpp \
	threads/busthread.cpp \
	build/buildjson.cpp \
	experiments/drive.cpp

if CONFIGURE_DBUS
libswampd_la_SOURCES += dbus/dbusconfiguration.cpp
endif

libmanualcmdsdir = ${libdir}/ipmid-providers
libmanualcmds_LTLIBRARIES = libmanualcmds.la
libmanualcmds_la_SOURCES = \
	ipmi/manualcmds.cpp
libmanualcmds_la_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(LIBIPMID_LIBS) \
	-version-info 0:0:0 -shared
libmanualcmds_la_CXXFLAGS = \
	$(SYSTEMD_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(LIBIPMID_CFLAGS) \
	-flto

SUBDIRS = . test
