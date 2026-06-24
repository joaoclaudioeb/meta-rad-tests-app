project(
  'rad-tests-app',
  ['cpp', 'c'],
  version: '1.3',
  default_options: [
    'warning_level=3',
    'cpp_std=c++20',
    'c_std=gnu11',
    'optimization=3',
    'buildtype=release',
  ],
)

rta_deps = []
rta_srcs = []

rta_inc = include_directories('include')

sqlite3_dep = dependency('sqlite3')

fsatutils = dependency('fsatutils')

libdac81408 = subproject('libdac81408')
dac81408_dep = libdac81408.get_variable('dac81408_dep')

rta_deps += sqlite3_dep
rta_deps += fsatutils
rta_deps += dac81408_dep

subdir('src')

c_args = [
  '-Wno-unused-parameter',
  '-Wno-unused-value',
  '-Wno-missing-field-initializers',
  '-Wno-narrowing',
  '-Wno-pointer-arith',
  '-Wshadow',
  '-Wcast-align',
  '-Wwrite-strings',
]

py = find_program('python3')

version_hpp = vcs_tag(
  input: 'scripts/version.hpp.in',
  output: 'version.hpp',
  command: [
    py,
    meson.project_source_root() / 'scripts/gen_versioning.py',
    meson.project_version(),
  ],
  fallback: meson.project_version(),
)

rad_tests_app = executable(
  meson.project_name(),
  sources: [rta_srcs, version_hpp],
  include_directories: rta_inc,
  dependencies: rta_deps,
  c_args: c_args,
  install: true,
)

install_data(
  'services/rad-tests-app.service',
  install_dir: get_option('systemd_unitdir'),
  rename: 'rad-tests-app.service',
)
