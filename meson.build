project(
    'phosphor-pid-control', 'cpp',
    version : '1.0.0',
    meson_version: '>=0.63.0',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20',
    ]
)

conf_data = configuration_data()

bindir = get_option('prefix') / get_option('bindir')
conf_data.set('BINDIR', bindir)
conf_data.set('SYSTEMD_TARGET', get_option('systemd_target'))
conf_data.set('STRICT_FAILSAFE_PWM', get_option('strict-failsafe-pwm'))

configure_file(output: 'config.h',
    configuration: conf_data
)

if get_option('oe-sdk').enabled()
    OECORE_TARGET_SYSROOT = run_command('sh', '-c', 'echo $OECORE_TARGET_SYSROOT').stdout().strip()
    if OECORE_TARGET_SYSROOT == ''
        error('OECORE_TARGET_SYSROOT must be set with enable oe-sdk')
    endif
    message('Enabling OE-SDK at OECORE_TARGET_SYSROOT: ' + OECORE_TARGET_SYSROOT)
    rpath = ':'.join([OECORE_TARGET_SYSROOT + '/lib', OECORE_TARGET_SYSROOT + '/usr/lib'])
    ld_so = run_command('sh', '-c', 'find ' + OECORE_TARGET_SYSROOT + '/lib/ld-*.so | sort -r -n | head -n1').stdout().strip()
    dynamic_linker = ['-Wl,-dynamic-linker,' + ld_so]
else
    dynamic_linker = []
endif

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_variable(
        'systemdsystemunitdir',
        pkgconfig_define: ['prefix', get_option('prefix')])

configure_file(input: 'phosphor-pid-control.service.in',
               output: 'phosphor-pid-control.service',
               configuration: conf_data,
               install: true,
               install_dir: systemd_system_unit_dir)

phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')
sdbusplus_dep = dependency('sdbusplus')
libsystemd_dep = dependency('libsystemd')
ipmid_dep = dependency('libipmid')

deps = [
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
    libsystemd_dep,
    ipmid_dep,
]

root_inc = include_directories(
    '.',
    'buildjson',
    'dbus',
    'errors',
    'experiments',
    'ipmi',
    'notimpl',
    'pid',
    'sensors',
    'sysfs',
)

setsensor_sources = [
    'setsensor.cpp'
]

libswampd_sources = [
    'main.cpp',
    'util.cpp',
    'notimpl/readonly.cpp',
    'notimpl/writeonly.cpp',
    'dbus/dbusconfiguration.cpp',
    'dbus/dbusutil.cpp',
    'dbus/dbushelper.cpp',
    'dbus/dbuspassiveredundancy.cpp',
    'dbus/dbuspassive.cpp',
    'dbus/dbusactiveread.cpp',
    'dbus/dbuswrite.cpp',
    'sysfs/sysfsread.cpp',
    'sysfs/sysfswrite.cpp',
    'sysfs/util.cpp',
    'sensors/pluggable.cpp',
    'sensors/host.cpp',
    'sensors/builder.cpp',
    'sensors/buildjson.cpp',
    'sensors/manager.cpp',
    'sensors/build_utils.cpp',
    'pid/ec/pid.cpp',
    'pid/ec/logging.cpp',
    'pid/ec/stepwise.cpp',
    'pid/fancontroller.cpp',
    'pid/thermalcontroller.cpp',
    'pid/pidcontroller.cpp',
    'pid/stepwisecontroller.cpp',
    'pid/builder.cpp',
    'pid/buildjson.cpp',
    'pid/zone.cpp',
    'pid/util.cpp',
    'pid/pidloop.cpp',
    'pid/tuning.cpp',
    'buildjson/buildjson.cpp',
    'experiments/drive.cpp',
]

libmanualcmds_sources = [
    'ipmi/main_ipmi.cpp',
    'ipmi/manualcmds.cpp',
    'ipmi/dbus_mode.cpp',
]

libmanualcmds = library(
    'manualcmds',
    libmanualcmds_sources,
    implicit_include_directories: false,
    dependencies: deps,
    version: meson.project_version(),
    override_options: ['b_lundef=false'],
    install: true,
    install_dir: get_option('libdir') / 'ipmid-providers')

executable(
    'swampd',
    libswampd_sources,
    implicit_include_directories: false,
    include_directories: root_inc,
    dependencies: deps,
    install: true,
    install_dir: get_option('bindir')
)

executable(
    'setsensor',
    setsensor_sources,
    implicit_include_directories: true,
    dependencies: deps,
    install: true,
    install_dir: get_option('bindir')
)

if not get_option('tests').disabled()
    subdir('test')
endif
