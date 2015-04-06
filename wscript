# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

VERSION = "0.0.0"
APPNAME = "ndn-cxx-ext"

from waflib import Logs, Utils, Context
import os

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['boost', 'default-compiler-flags', 'compiler-features'],
             tooldir=['.waf-tools'])

    opt.add_option('--with-tests', action='store_true', default=False,
                      dest='with_tests', help='''Build unit tests''')
    opt.add_option('--without-tools', action='store_false', default=True, dest='with_tools',
                   help='''Do not build tools''')

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'default-compiler-flags', 'boost', 'compiler-features'])

    conf.env['WITH_TESTS'] = conf.options.with_tests
    conf.env['WITH_TOOLS'] = conf.options.with_tools

    USED_BOOST_LIBS = ['system', 'filesystem', 'date_time', 'iostreams',
                       'regex', 'program_options', 'chrono', 'random']
    if conf.env['WITH_TESTS']:
        USED_BOOST_LIBS += ['unit_test_framework']
        conf.define('HAVE_TESTS', 1)

    conf.check_boost(lib=USED_BOOST_LIBS, mandatory=True)

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    conf.write_config_header('src/ndn-cxx-ext-config.hpp', define_prefix='NDNCXXEXT_')

def build(bld):
    libndn_cxx_ext = bld(
        features=['cxx', 'cxxstlib'], # 'cxxshlib',
        target="ndn-cxx-ext",
        name="ndn-cxx-ext",
        source=bld.path.ant_glob('src/**/*.cpp'),
        use='BOOST NDN_CXX',
        includes=". src",
        export_includes="src",
        install_path='${LIBDIR}',
        )

    if bld.env['WITH_TESTS']:
        bld.recurse('tests')

    if bld.env['WITH_TOOLS']:
        bld.recurse("tools")

    headers = bld.path.ant_glob(['src/**/*.hpp'])
    bld.install_files("%s/ndn-cxx-ext" % bld.env['INCLUDEDIR'], headers,
                      relative_trick=True, cwd=bld.path.find_node('src'))

    bld.install_files("%s/ndn-cxx-ext" % bld.env['INCLUDEDIR'],
                      bld.path.find_resource('src/ndn-cxx-ext-config.hpp'))
