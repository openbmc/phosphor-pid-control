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
        ])
        gmock = gtest_proj.dependency('gmock')
    else
        assert(not get_option('tests').enabled(), 'Googletest is required')
    endif
endif

swampd_sources = include_directories('../')

test(
    'dbus_active_unittest',
    executable(
        'dbus_active_unittest',
        'dbus_active_unittest.cpp',
        '../dbus/dbusactiveread.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'dbus_passive_unittest',
    executable(
        'dbus_passive_unittest',
        'dbus_passive_unittest.cpp',
        '../dbus/dbuspassive.cpp',
        '../dbus/dbuspassiveredundancy.cpp',
        '../dbus/dbusutil.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'dbus_util_unittest',
    executable(
        'dbus_util_unittest',
        'dbus_util_unittest.cpp',
        '../dbus/dbusutil.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'json_parse_unittest',
    executable(
        'json_parse_unittest',
        'json_parse_unittest.cpp',
        '../buildjson/buildjson.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'pid_json_unittest',
    executable(
        'pid_json_unittest',
        'pid_json_unittest.cpp',
        '../pid/buildjson.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'pid_fancontroller_unittest',
    executable(
        'pid_fancontroller_unittest',
        'pid_fancontroller_unittest.cpp',
        '../pid/ec/pid.cpp',
        '../pid/fancontroller.cpp',
        '../pid/pidcontroller.cpp',
        '../pid/tuning.cpp',
        '../pid/util.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'pid_stepwisecontroller_unittest',
    executable(
        'pid_stepwisecontroller_unittest',
        'pid_stepwisecontroller_unittest.cpp',
        '../pid/ec/stepwise.cpp',
        '../pid/stepwisecontroller.cpp',
        '../pid/util.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'pid_thermalcontroller_unittest',
    executable(
        'pid_thermalcontroller_unittest',
        'pid_thermalcontroller_unittest.cpp',
        '../pid/ec/pid.cpp',
        '../pid/pidcontroller.cpp',
        '../pid/thermalcontroller.cpp',
        '../pid/tuning.cpp',
        '../pid/util.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'pid_zone_unittest',
    executable(
        'pid_zone_unittest',
        'pid_zone_unittest.cpp',
        '../pid/ec/pid.cpp',
        '../pid/pidcontroller.cpp',
        '../pid/tuning.cpp',
        '../pid/zone.cpp',
        '../sensors/manager.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'sensor_host_unittest',
    executable(
        'sensor_host_unittest',
        'sensor_host_unittest.cpp',
        '../sensors/host.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'sensor_manager_unittest',
    executable(
        'sensor_manager_unittest',
        'sensor_manager_unittest.cpp',
        '../sensors/manager.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
	)
)

test(
    'sensor_pluggable_unittest',
    executable(
        'sensor_pluggable_unittest',
        'sensor_pluggable_unittest.cpp',
        '../sensors/pluggable.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'sensors_json_unittest',
    executable(
        'sensors_json_unittest',
        'sensors_json_unittest.cpp',
        '../sensors/buildjson.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)

test(
    'util_unittest',
    executable(
        'util_unittest',
        'util_unittest.cpp',
        '../sensors/build_utils.cpp',
        include_directories: [
            swampd_sources,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        dependencies: [
            gtest,
            gmock,
            deps,
        ]
    )
)
