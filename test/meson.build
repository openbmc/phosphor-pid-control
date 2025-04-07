gtest = dependency('gtest', main: true, disabler: true, required: false)
gmock = dependency('gmock', disabler: true, required: false)

if not gtest.found() or not gmock.found()
    gtest_proj = import('cmake').subproject('googletest', required: false)
    if gtest_proj.found()
        gtest = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ],
        )
        gmock = gtest_proj.dependency('gmock')
    else
        assert(not get_option('tests').enabled(), 'Googletest is required')
    endif
endif

swampd_sources = include_directories('../')

unit_tests = [
    'dbus_passive_unittest',
    'dbus_util_unittest',
    'json_parse_unittest',
    'pid_json_unittest',
    'pid_fancontroller_unittest',
    'pid_stepwisecontroller_unittest',
    'pid_thermalcontroller_unittest',
    'pid_zone_unittest',
    'sensor_host_unittest',
    'sensor_manager_unittest',
    'sensor_pluggable_unittest',
    'sensors_json_unittest',
    'util_unittest',
]

unittest_source = {
    'dbus_passive_unittest': [
        '../dbus/dbuspassive.cpp',
        '../dbus/dbuspassiveredundancy.cpp',
        '../dbus/dbusutil.cpp',
        '../failsafeloggers/failsafe_logger_utility.cpp',
    ],
    'dbus_util_unittest': ['../dbus/dbusutil.cpp'],
    'json_parse_unittest': ['../buildjson/buildjson.cpp'],
    'pid_json_unittest': ['../pid/buildjson.cpp', '../util.cpp'],
    'pid_fancontroller_unittest': [
        '../pid/ec/pid.cpp',
        '../pid/ec/logging.cpp',
        '../pid/fancontroller.cpp',
        '../pid/pidcontroller.cpp',
        '../pid/tuning.cpp',
        '../pid/util.cpp',
    ],
    'pid_stepwisecontroller_unittest': [
        '../pid/ec/stepwise.cpp',
        '../pid/stepwisecontroller.cpp',
        '../pid/tuning.cpp',
        '../pid/util.cpp',
    ],
    'pid_thermalcontroller_unittest': [
        '../pid/ec/pid.cpp',
        '../pid/ec/logging.cpp',
        '../pid/pidcontroller.cpp',
        '../pid/thermalcontroller.cpp',
        '../pid/tuning.cpp',
        '../pid/util.cpp',
    ],
    'pid_zone_unittest': [
        '../failsafeloggers/builder.cpp',
        '../failsafeloggers/failsafe_logger.cpp',
        '../failsafeloggers/failsafe_logger_utility.cpp',
        '../pid/ec/pid.cpp',
        '../pid/ec/logging.cpp',
        '../pid/pidcontroller.cpp',
        '../pid/tuning.cpp',
        '../pid/zone.cpp',
        '../sensors/manager.cpp',
    ],
    'sensor_host_unittest': [
        '../failsafeloggers/failsafe_logger.cpp',
        '../failsafeloggers/failsafe_logger_utility.cpp',
        '../sensors/host.cpp',
    ],
    'sensor_manager_unittest': ['../sensors/manager.cpp'],
    'sensor_pluggable_unittest': ['../sensors/pluggable.cpp'],
    'sensors_json_unittest': ['../sensors/buildjson.cpp'],
    'util_unittest': ['../sensors/build_utils.cpp'],
}

foreach t : unit_tests
    test(
        t,
        executable(
            t.underscorify(),
            t + '.cpp',
            unittest_source.get(t),
            include_directories: [swampd_sources],
            link_args: dynamic_linker,
            build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
            dependencies: [gtest, gmock, deps],
        ),
    )
endforeach
