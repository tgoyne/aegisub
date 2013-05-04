#!/usr/bin/python

from subprocess import check_call
import os

# Just manually specifying the build order is less effort than dependency tracking
BUILD_ORDER = [
    'ffmpeg',
    'ffms2',
    'fftw',
    'freetype2',
    'fontconfig',
    'fribidi',
    'icu/source',
    'hunspell',
    'libass',
    'wx'
]

# Project which need to use a C++ compiler. Mostly for the sake of ffmpeg, as
# the other projects are okay with getting C++-specific configure flags.
CPP_PROJECT = [
    'ffms2',
    'hunspell',
    'icu/source',
    'wx'
]

# Project-specific configure flags
EXTRA_CONFIG_FLAGS = {
    # --disable-avx since the AVX optimizations have been a recurring source of
    # crashes and don't seem to actually make anything meaningfully faster
    'ffmpeg' : ['--disable-filters', '--disable-network', '--disable-devices',
        '--enable-gpl', '--disable-encoders', '--enable-small',
        '--disable-muxers', '--disable-postproc', '--disable-avfilter',
        '--disable-ffprobe', '--disable-ffmpeg', '--disable-ffplay',
        '--disable-ffserver', '--disable-avx', '--disable-outdevs',
        '--disable-indevs', '--enable-avresample'],
    'fftw' : ['--enable-threads', '--enable-sse2'],
    'icu/source' : ['--disable-extras', '--disable-icuio', '--disable-layout',
        '--disable-tests', '--disable-samples', '--enable-rpath'],
    # --with-macosx-version-min=10.7 is required for libc++ on OS X
    'wx' : ['--with-opengl', '--enable-stc', '--disable-compat28',
        '--with-cocoa', '--disable-webview', '--without-libjpeg',
        '--without-libtiff', '--with-macosx-version-min=10.7',
        '--with-libpng=yes']
}

COMMON_DEBUG = ['--enable-debug', '--enable-shared', '--disable-static']
COMMON_RELEASE = ['--disable-debug', '--disable-shared', '--enable-static']
LIBCPP = ['CXXFLAGS=-stdlib=libc++ -std=c++11', 'OBJCXXFLAGS=-stdlib=libc++',
    'LDFLAGS=-stdlib=libc++', 'LIBS=-lc++']

def build(dep, mode, common, cppflags):
  try:
    os.mkdir('/src/prefix/%s' % mode)
  except OSError:
    # dir already exists; not an error
    pass

  env = os.environ.copy()
  if mode == 'i386':
    env['CC'] = 'clang -arch i386'
    env['CXX'] = 'clang++ -arch i386'
  else:
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'
  env['PKG_CONFIG_PATH'] = '/src/prefix/%s/lib/pkgconfig' % mode
  env['ACLOCAL_FLAGS'] = '-I/usr/local/Cellar/gettext/0.18.1.1/share/aclocal'

  try:
    os.mkdir('/src/%s/build-%s' % (dep, mode))
  except OSError:
    # dir already exists; not an error
    pass

  os.chdir('/src/%s/build-%s' % (dep, mode))

  conf_args = ['/src/%s/configure' % dep, '--prefix=/src/prefix/%s' % mode]
  conf_args.extend(common)
  if dep in cpp:
    conf_args.extend(cppflags)
  if dep == 'ffmpeg' or dep == 'libav':
      conf_args.extend(["--cc=" + env['CC']])
      if mode == 'i368':
        conf_args.extend(['--arch=i386'])
  conf_args.extend(extra_config_flags[dep]),
  check_call(conf_args, env=env)
  check_call(['make', '-j5'], env=env)
  check_call(['make', 'install'], env=env)


for dep in build_order:
  build(dep, 'libc++-debug', common_debug, libcpp_cpp)
  build(dep, 'libc++-release', common_release, libcpp_cpp)

