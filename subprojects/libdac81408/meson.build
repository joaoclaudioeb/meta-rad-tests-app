project(
  'libdac81408',
  'c',
  version: '1.0.0',
  default_options: ['c_std=gnu11'],
)

dac81408_srcs = files(
    'src/dac81408.c',
    'src/gpio_linux.c',
    'src/spi_linux.c',
)

dac81408_inc = include_directories('include')

dac81408_deps = []

libgpiod = dependency('libgpiod')

dac81408_deps += libgpiod

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

libdac81408 = static_library(
  'dac81408',
  sources: dac81408_srcs,
  include_directories: dac81408_inc,
  dependencies: dac81408_deps,
  c_args: c_args,
)

dac81408_dep = declare_dependency(
  include_directories : dac81408_inc,
  link_with : libdac81408,
)
