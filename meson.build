project(
    'phosphor-pid-control',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++17'
    ],
    license: 'Apache-2.0',
    version: '0.1',
)

add_project_arguments(
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_NO_RTTI',
    '-DBOOST_NO_TYPEID',
    '-DBOOST_ALL_NO_LIB',
    language: 'cpp',
)

bindir = get_option('prefix') + '/' +get_option('bindir')

build_tests = get_option('tests')
sdbusplus = dependency('sdbusplus')
sdbusplus = sdbusplus.as_system('system')

systemd = dependency('systemd')
systemd = systemd.as_system('system')

systemd_system_unit_dir = systemd.get_pkgconfig_variable(
    'systemdsystemunitdir',
    define_variable: ['prefix', get_option('prefix')])

threads = dependency('threads')

phosphor_logging = dependency('phosphor-logging')
phosphor_logging = phosphor_logging.as_system('system')

phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_dbus_interfaces = phosphor_dbus_interfaces.as_system('system')


libipmid = dependency('libipmid')
libipmid = libipmid.as_system('system')

boost = dependency('boost',version : '>=1.75.0', required : false)

if get_option('tests').enabled()
  gtest = dependency('gtest', main: true,disabler: true, required : false)
  gmock = dependency('gmock', required : false)
  gtest = gtest.as_system('system')
  gmock = gmock.as_system('system')
endif

cxx = meson.get_compiler('cpp')

add_project_arguments(
  cxx.get_supported_arguments([
  '-Wno-unused-parameter',
  ]),
  language: 'cpp'
)

if cxx.has_header('nlohmann/json.hpp')
    nlohmann_json = declare_dependency()
endif

if cxx.has_header('CLI/CLI.hpp')
    cli = declare_dependency()
endif

libswampd_a = static_library(
    'swampd_a',
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
	'build/buildjson.cpp',
	'experiments/drive.cpp',
    include_directories: [
        'sysfs',
    ],
)

libmanualcommands_a = shared_library(
    'manualcmds',
	'ipmi/main_ipmi.cpp',
	'ipmi/manualcmds.cpp',
	'ipmi/dbus_mode.cpp',
    dependencies: [
        systemd,
        phosphor_dbus_interfaces,
        phosphor_logging,
        libipmid,
    ],
    version: '1',
    install_dir: 'lib/ipmid-providers/',
    install: true,
)

if get_option('tests').enabled()
    subdir('test')
endif

conf_data = configuration_data()
conf_data.set('MESON_INSTALL_PREFIX', get_option('prefix'))

configure_file(input: 'phosphor-pid-control.service.in',
               output: 'phosphor-pid-control.service',
               install_dir: systemd_system_unit_dir,
               configuration: conf_data,
               install: true)

configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : conf_data)

executable('swampd', 'main.cpp',
            dependencies: [
                nlohmann_json,
                sdbusplus,
                systemd,
                phosphor_logging,
                threads,
            ],
            link_args: '-Wl,--gc-sections',
            link_with: [
                libswampd_a,
            ],
            install: true,
            install_dir: bindir)