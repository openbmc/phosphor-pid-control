# Initialization
AC_PREREQ([2.69])
AC_INIT([swampd], [0.1], [https://github.com/openbmc/phosphor-pid-control/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Python
AM_PATH_PYTHON([2.7], [AC_SUBST([PYTHON], [echo "$PYTHON"])],
[AC_MSG_ERROR([Could not find python-2.7 installed...python-2.7 is required])])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for libraries.
PKG_CHECK_MODULES(
    [SYSTEMD],
    [libsystemd >= 221],
    [],
    [AC_MSG_ERROR(["systemd required and not found"])]
)
PKG_CHECK_MODULES(
    [SDBUSPLUS],
    [sdbusplus],
    [],
    [AC_MSG_ERROR([The openbmc/sdbusplus package is required])]
)
PKG_CHECK_MODULES(
    [PHOSPHOR_LOGGING],
    [phosphor-logging],
    [],
    [AC_MSG_ERROR([The openbmc/phosphor-logging package is required])]
)
PKG_CHECK_MODULES(
    [PHOSPHOR_DBUS_INTERFACES],
    [phosphor-dbus-interfaces],
    [],
    [AC_MSG_ERROR(["phosphor-dbus-interfaces required and not found."])]
)
PKG_CHECK_MODULES(
    [LIBIPMID],
    [libipmid],
    [],
    [AC_MSG_ERROR([Could not find libipmid...openbmc/phosphor-host-ipmid package required])]
)
AC_CHECK_HEADER(
    experimental/filesystem,
    [],
    [AC_MSG_ERROR([Could not find experimental/filesystem...libstdc++fs development package required])]
)
AC_CHECK_HEADER(
    nlohmann/json.hpp,
    [],
    [AC_MSG_ERROR([Could not find nlohmann/json.hpp])]
)
AX_PTHREAD([], [AC_MSG_ERROR(["pthread required and not found"])])

# Checks for library functions.
LT_INIT # Required for systemd linking

# Check/set gtest specific functions.
PKG_CHECK_MODULES(
    [GTEST],
    [gtest],
    [],
    [AC_MSG_NOTICE([gtest not found, tests will not build])]
)
PKG_CHECK_MODULES(
    [GMOCK],
    [gmock],
    [],
    [AC_MSG_NOTICE([gmock not found, tests will not build])]
)
PKG_CHECK_MODULES(
    [GTEST_MAIN],
    [gtest_main],
    [],
    [AC_MSG_NOTICE([gtest_main not found, tests will not build])]
)

AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING(
        [--enable-oe-sdk],
        [Link testcases absolutely against OE SDK so they can be ran within it.]
    )
)
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT]
)
AS_IF([test "x$enable_oe_sdk" == "xyes"],
    AS_IF([test "x$OECORE_TARGET_SYSROOT" == "x"],
          AC_MSG_ERROR([OECORE_TARGET_SYSROOT must be set with --enable-oe-sdk])
    )
    AC_MSG_NOTICE([Enabling OE-SDK at $OECORE_TARGET_SYSROOT])
    [
        testcase_flags="-Wl,-rpath,\${OECORE_TARGET_SYSROOT}/lib"
        testcase_flags="${testcase_flags} -Wl,-rpath,\${OECORE_TARGET_SYSROOT}/usr/lib"
        testcase_flags="${testcase_flags} -Wl,-dynamic-linker,`find \${OECORE_TARGET_SYSROOT}/lib/ld-*.so | sort -r -n | head -n1`"
    ]
    AC_SUBST([OESDK_TESTCASE_FLAGS], [$testcase_flags])
)

AS_IF([test "x$SENSOR_YAML_GEN" == "x"], [SENSOR_YAML_GEN="sensor-example.yaml"])
SENSORGEN="$PYTHON ${srcdir}/scripts/sensor_gen.py -i $SENSOR_YAML_GEN"
AC_SUBST(SENSORGEN)

AS_IF([test "x$PID_YAML_GEN" == "x"], [PID_YAML_GEN="pid-example.yaml"])
PIDGEN="$PYTHON ${srcdir}/scripts/pid_gen.py -i $PID_YAML_GEN"
AC_SUBST(PIDGEN)

AS_IF([test "x$ZONE_YAML_GEN" == "x"], [ZONE_YAML_GEN="zone-example.yaml"])
ZONEGEN="$PYTHON ${srcdir}/scripts/zone_gen.py -i $ZONE_YAML_GEN"
AC_SUBST(ZONEGEN)

AC_ARG_ENABLE([configure-dbus],
    AS_HELP_STRING(
        [--enable-configure-dbus], [Enable configuring pid from D-Bus.]
    )
)
AM_CONDITIONAL(CONFIGURE_DBUS, [test "x$enable_configure_dbus" = "xyes"])
AS_IF([test "x$enable_configure_dbus" = "xyes"],
    [AC_DEFINE(CONFIGURE_DBUS, [1], [Read configuration from D-Bus.])],
    [AC_DEFINE(CONFIGURE_DBUS, [0], [Do not read configuration from D-Bus.])]
)

# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile])
AC_OUTPUT
