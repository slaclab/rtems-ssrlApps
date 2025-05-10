# WScript for ssrlApps
rtems_version = "6"

import rtems_waf.rtems as rtems
import os

ROOT = os.getcwd()

def _check_headers(conf, headers: dict):
    """
    Checks for a list of headers and generates a define for them

    Parameters
    ----------
    conf :
        Config context
    headers : dict
        Mapping of header -> define
    """
    for k, v in headers.items():
        conf.check_cc(
            use='rtemsdefaultconfig',
            header_name=k,
            features='c',
            define_name=v
        )


def _bsp_configure(conf, bsp, lib_confs : list = []):
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

    # Check for common headers
    _check_headers(conf, {
        'sys/mman.h': 'HAVE_SYS_MMAN_H',
        'strings.h': 'HAVE_STRINGS_H',
        'sys/select.h': 'HAVE_SYS_SELECT_H',
        'sys/termios.h': 'HAVE_SYS_TERMIOS_H',
        'termios.h': 'HAVE_TERMIOS_H',
        'ncurses/term.h': 'HAVE_NCURSES_TERM_H',
        'ncurses/curses.h': 'HAVE_NCURSES_CURSES_H',
    })

    conf.write_config_header(f'{conf.env.RTEMS_ARCH_BSP}/config.h')

    # Now configure all other libraries
    for c in lib_confs:
        c(conf, bsp)

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

def _install_headers(bld, headers: list[str], subdir: str = ''):
    """
    Installs some headers

    Parameters
    ----------
    bld :
        Build context
    headers : list[str]
        List of heaaders to install
    subdir : str
        Subdirectory within the include directory
    """
    bld.install_files(
        f'{_get_install_prefix(bld)}/include/{subdir}',
        headers
    )

def _install_libs(bld, libs: list[str], subdir: str = ''):
    """
    Installs some libraries

    Parameters
    ----------
    bld :
        Build context
    libs : list[str]
        List of libs to install
    subdir : str
        Subdirectory within the include directory
    """
    bld.install_files(
        f'{_get_install_prefix(bld)}/lib/{subdir}',
        libs
    )


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
    """
    Builds the miscUtils module
    """
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
    """
    Builds the libbspExt module
    """
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
    """
    Builds the monitor module
    """
    sources = [
        'monitor/call.cc',
        'monitor/memusage.c',
        'monitor/stack.c'
    ]

    _build_module(bld, 'monitor', sources=sources)

def build_regexp(bld):
    """
    Builds the spencer regexp library
    """

    dir = 'cexp/regexp'
    sources = [
        f'{dir}/regexp.c',
        f'{dir}/regerror.c',
        f'{dir}/regsub.c'
    ]

    bld(
        target='spencer_regexp',
        features='c cstlib',
        source=sources,
        includes=_get_includes(bld)
    )

    _install_libs(bld, ['libspencer_regexp.a'])
    _install_headers(bld, [f'{dir}/regexp/spencer_regexp.h'])


def conf_pmbfd(conf, bsp: str):
    pass

def build_pmbfd(bld):
    """
    Builds the pmbfd library from Cexp
    """

    dir = 'cexp/pmbfd'
    pmelf_sources = [
        f'{dir}/symname.c',
        f'{dir}/secname.c',
        f'{dir}/putdat.c',
        f'{dir}/dmpgrps.c',
        f'{dir}/strm.c',
        f'{dir}/fstrm.c',
        f'{dir}/mstrm.c',
        f'{dir}/dmpsym.c',
        f'{dir}/dmpsymtab.c',
        f'{dir}/dmpshdr.c',
        f'{dir}/dmpshtab.c',
        f'{dir}/dmpehdr.c',
        f'{dir}/dmprels.c',
        f'{dir}/dmpphdr.c',
        f'{dir}/symtab.c',
        f'{dir}/findsymhdrs.c',
        f'{dir}/shtab.c',
        f'{dir}/getgrp.c',
        f'{dir}/getrel.c',
        f'{dir}/getscn.c',
        f'{dir}/getsym.c',
        f'{dir}/putsym.c',
        f'{dir}/getshdr.c',
        f'{dir}/getphdr.c',
        f'{dir}/putshdr.c',
        f'{dir}/getehdr.c',
        f'{dir}/putehdr.c',
        f'{dir}/attpbfasmatch.c',
        f'{dir}/attpbfasdestroy.c',
        f'{dir}/attpbfasread.c',
        f'{dir}/attpbfasprint.c',
        f'{dir}/attpbfaprint.c',
        f'{dir}/attpbprinttag.c',
        f'{dir}/attset.c',
        f'{dir}/attprint.c',
        f'{dir}/attvendfind.c',
        f'{dir}/guleb128.c',
        f'{dir}/getwrd.c',
        f'{dir}/att-gnu-powerpc.c',
    ]

    # TODO: if NO_64BIT
    pmelf_sources += [f'{dir}/noelf64.c']

    bld(
        target='pmelf',
        features='c cstlib',
        source=pmelf_sources,
        includes=_get_includes(bld) + ['cexp/pmbfd']
    )

    _install_headers(bld, [f'{dir}/pmelf.h'])

    pmbfd_sources = [
        f'{dir}/bfd.c',
        f'{dir}/opcodesup.c',
        f'{dir}/bfd-reloc-arm.c',
        f'{dir}/bfd-reloc-m68k.c',
        f'{dir}/bfd-reloc-i386.c',
        f'{dir}/bfd-reloc-powerpc.c',
        f'{dir}/bfd-reloc-sparc.c',
        f'{dir}/bfd-reloc-x86_64.c',
    ]

    bld(
        target='pmbfd',
        features='c cstlib',
        source=pmbfd_sources,
        includes=_get_includes(bld)
    )

    _install_libs(bld, ['libpmbfd.a', 'libpmelf.a'])


def build_tecla(bld):
    """
    Builds libtecla, which is needed for cexp
    """

    sources = [

    ]


#####################################################################
# Waf methods; must always be defined
#####################################################################

def build(bld):
    rtems.build(bld)
    bld.env.CFLAGS += ['-O2', '-g']
    bld.env.CPPFLAGS += ['-DHAVE_CONFIG_H=1']

    build_miscUtils(bld)
    build_telnetd(bld)

    build_regexp(bld)
    build_pmbfd(bld)

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

    # These will be invoked per-BSP
    lib_confs = [
        conf_pmbfd
    ]

    rtems.configure(conf, bsp_configure=lambda c, b : _bsp_configure(c, b, lib_confs))

def init(ctx):
    rtems.init(ctx, version=rtems_version, long_commands=True)

def options(opt):
    rtems.options(opt)
    opt.add_option('--ssrl-version', dest='SSRLAPPS_VER', type=str, default='ssrlApps', help='ssrlApps patch level string (i.e. ssrlApps_p4)')
