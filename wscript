# WScript for ssrlApps
rtems_version = "6"

import rtems_waf.rtems as rtems
import os

ROOT = os.getcwd()

def _bsp_configure(conf, bsp):
    """
    Configures the build for the specified BSP

    Parameters
    ----------
    conf :
        Configuration context
    bsp : str
        arch + bsp
    """

    # Pretty ugly, but replicates what Till did in his m4 utils package. Throws these into config.h
    conf.undefine('RTEMS_VERSION_LATER_THAN')
    conf.define('RTEMS_VERSION_LATER_THAN(ma,mi,re)',
	'(    __RTEMS_MAJOR__  > (ma)	\
	 || (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__  > (mi))	\
	 || (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__ == (mi) && __RTEMS_REVISION__ > (re)) \
    )', quote=False)
    conf.undefine('RTEMS_VERSION_ATLEAST')
    conf.define('RTEMS_VERSION_ATLEAST(ma,mi,re)',
    '(    __RTEMS_MAJOR__  > (ma)	\
	|| (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__  > (mi))	\
	|| (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__ == (mi) && __RTEMS_REVISION__ >= (re)) \
	)', quote=False)
    conf.write_config_header(f'{conf.env.RTEMS_ARCH_BSP}/config.h')

def _get_install_prefix(ctx) -> str:
    """
    Returns the install prefix for the specified BSP and arch

    Parameters
    ----------
    ctx :
        Build context
    """
    return f'${{PREFIX}}/{ctx.options.SSRLAPPS_VER}/{ctx.env.RTEMS_ARCH_RTEMS}/{ctx.env.RTEMS_BSP}'

def _get_lib_paths(ctx) -> list[str]:
    """
    Returns a list of library search directories for the specified bsp

    Parameters
    ----------
    ctx :
        Build context
    """
    return [
        f'{ctx.env.RTEMS_PATH}/{rtems.arch_bsp_lib_path(rtems_version, ctx.env.RTEMS_ARCH_BSP)}',
        f'{ctx.env.RTEMS_PATH}/{ctx.options.SSRLAPPS_VER}/{rtems.arch_bsp_lib_path(rtems_version, ctx.env.RTEMS_ARCH_BSP)}'
    ]

def _get_includes(ctx) -> list[str]:
    """
    Returns a list of include directories

    Parameters
    ----------
    ctx :
        Build context
    """
    return [
        f'{ctx.env.RTEMS_PATH}/{rtems.arch_bsp_include_path(rtems_version, ctx.env.RTEMS_ARCH_BSP)}',
        '.'
    ]


def _build_module(bld, target: str, sources: list[str] = [], includes: list[str] = [], ldflags: list[str] = [], libs: list[str] = []):
    """
    Builds the specified module with the specified properties.
    Does not link to the standard library or any other RTEMS libraries by default

    Parameters
    ----------
    bld :
        Build context
    target : str
        Name of the target file to generate
    sources : list[str]
        Source files to compile
    includes : list[str]
        Include directories to append
    ldflags : list[str]
        Linker flags to append
    libs : list[str]
        Libraries to link against
    """

    includes = includes.copy()
    includes.extend(_get_includes(bld))

    def link_task(task):
        cmd = [
            bld.env.CC[0],
            '-o',
            task.outputs[0].abspath(),
            '-nostdlib',
            '-Wl,-r',
        ]
        cmd.extend(bld.env.CFLAGS)
        cmd.extend(bld.env.CPPFLAGS)
        cmd.extend([f'-L{x}' for x in _get_lib_paths(bld)])
        # Relative to build dir
        cmd.extend([bld.env.CPPPATH_ST % x for x in task.generator.includes])
        # Relative to srcdir
        cmd.extend([bld.env.CPPPATH_ST % f'{ROOT}/{x}' for x in task.generator.includes])
        cmd.extend(task.generator.ldflags)
        cmd.extend([x.abspath() for x in task.inputs])
        cmd.extend(task.generator.libs)
        print(' '.join(cmd))
        return task.exec_command(cmd)

    bld(
        rule=link_task,
        source=[sources],
        includes=includes,
        libs=libs,
        ldflags=ldflags,
        target=target
    )

    # Install to rtems bsp subdir
    bld.install_files(f'{_get_install_prefix(bld)}/bin', target)


def build_miscUtils(bld):
    source=[
        'miscUtils/icmpping.c',
        'miscUtils/memUtils.c',
        'miscUtils/traceroute.c',
        'miscUtils/ttyconfi.c',
        'miscUtils/sockstats.c',
        'miscUtils/task_uptime.c',
        'miscUtils/exectime.c',
        'miscUtils/loop.c',
    ]

    _build_module(bld, 'miscUtils.obj', sources=source, includes=['miscUtils'])

def build_telnetd(bld):
    _build_module(bld, 'telnetd.obj', ldflags=['-Wl,-u,rtems_telnetd_initialize'], libs=['-ltelnetd'])

def build_libbspExt(bld):
    sources = [
        'libbspExt/bspExt.c',
        'libbspExt/dabrBpnt.c',
        'libbspExt/isrWrap.c',
        'libbspExt/memProbe.c'
    ]
    _build_module(bld, target='bspExt.obj', sources=sources)

    bld(
        target='bspExt',
        features='c cstlib',
        source=sources,
        includes=_get_includes(bld)
    )

    bld.install_files(f'{_get_install_prefix(bld)}/lib', 'libbspExt.a')
    bld.install_files(f'{_get_install_prefix(bld)}/include/bsp', ['libbspExt/bspExt.h'])

def build_monitor(bld):
    sources = [
        'monitor/call.cc',
        'monitor/memusage.c',
        'monitor/stack.c'
    ]

    _build_module(bld, 'monitor', sources=sources)

#####################################################################
# Waf methods; must always be defined
#####################################################################

def build(bld):
    rtems.build(bld)
    bld.env.CFLAGS += ['-O2', '-g']
    bld.env.CPPFLAGS += ['-DHAVE_CONFIG_H=1']

    build_miscUtils(bld)
    build_telnetd(bld)
    # TODO: Needs tecla from cexp!
    #build_monitor(bld)

    # Only available for PPC
    if bld.env.RTEMS_ARCH == 'powerpc':
        build_libbspExt(bld)

def configure(conf):
    """
    Configure the build

    Parameters
    ----------
    conf :
        Configuration context
    """
    rtems.configure(conf, bsp_configure=_bsp_configure)

def init(ctx):
    rtems.init(ctx, version=rtems_version, long_commands=True)

def options(opt):
    rtems.options(opt)
    opt.add_option('--ssrl-version', dest='SSRLAPPS_VER', type=str, default='ssrlApps', help='ssrlApps patch level string (i.e. ssrlApps_p4)')
