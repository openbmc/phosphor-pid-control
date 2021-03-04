sensor_manager_unittest = executable('sensor_manager_test',
  'sensor_manager_unittest.cpp',
  include_directories : ['..'],
  install_dir: bindir,
  dependencies: [
    nlohmann_json,
    sdbusplus,
    systemd,
    phosphor_logging,
    threads,
    gtest,
    gmock,
  ],
  link_with: [
    libswampd_a,
    libmanualcommands_a,
  ])

test('sensor_manager_test', sensor_manager_unittest)

sensor_pluggable_unittest = executable('sensor_pluggable_unittest',
  'sensor_pluggable_unittest.cpp',
  include_directories : ['..'],
  install_dir: bindir,
  dependencies: [
    nlohmann_json,
    sdbusplus,
    systemd,
    phosphor_logging,
    threads,
    gtest,
    gmock,
  ],
  link_with: [
    libswampd_a,
    libmanualcommands_a,
  ])

test('sensor_pluggable_test', sensor_pluggable_unittest)