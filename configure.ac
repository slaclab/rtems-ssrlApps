AC_PREREQ(2.62)
m4_define([SSRLAPPS_VERSION],TILLAC_CVSTAG([$Name$]))

AC_INIT(ssrlApps, SSLRAPPS_VERSION, <strauman@slac.stanford.edu>)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign 1.10])
AM_MAINTAINER_MODE

# Check for critical programs we need for building
TILLAC_RTEMS_CHECK_TOOLS

# Must add this BEFORE TILLAC_RTEMS_SETUP
# so that the dummy-top 'config.status' also
# knows how to make a config.h
AM_CONFIG_HEADER(config.h)

TILLAC_RTEMS_SETUP

AC_SUBST([enable_subdirs])
AC_SUBST([rtems_cpu],[$host_cpu])
AC_SUBST([rtems_bsp],[$enable_rtemsbsp])

if test "${exec_prefix}" = "NONE" ; then
	exec_prefix='${prefix}/target/ssrlApps/${host_cpu}-${host_os}'/${rtems_bsp}/
	ac_configure_args="${ac_configure_args} --exec-prefix='${exec_prefix}'"
fi

# Unfortunately we have no way to check if includedir was set by the user
# other than scanning the argument line :-(
tillac_rtems_includedir_set=no
for tillac_rtems_arg in ${ac_configure_args} ; do
	case $tillac_rtems_arg in
		-includedir | --includedir | --includedi | --included | --include \
		| --includ | --inclu | --incl | --inc \
        | -includedir=* | --includedir=* | --includedi=* | --included=* | --include=* \
	    | --includ=* | --inclu=* | --incl=* | --inc=*)
		tillac_rtems_includedir_set=yes;
		;;
	*)
	    ;;
	esac
done

if test "${tillac_rtems_includedir_set}" = "no" ; then
	includedir='${exec_prefix}/include'
	ac_configure_args="${ac_configure_args} --includedir='${includedir}'"
fi

# Let this configure and sub-configures search
# the temporary installdir for includes and libraries...
CPPFLAGS="$CPPFLAGS -I`pwd`/data/include"
LDFLAGS="$LDFLAGS -L`pwd`/data/lib"

config_files=Makefile
enable_subdirs=

case ${host_cpu} in
	powerpc | i386)
		if test -d $srcdir/libbspExt; then
		config_files="$config_files libbspExt/Makefile"
		enable_subdirs="${enable_subdirs} libbspExt"
		AC_DEFINE(HAVE_LIBBSPEXT,1,[Whether we have the libbspExt extension library])
		have_bspext=yes
		fi 
	;;
	*)
	;;
esac

case ${host_cpu} in
	powerpc | i386 | m68k)
		if test -d $srcdir/rtems-gdb-stub; then
		config_files="$config_files rtems-gdb-stub/Makefile"
		enable_subdirs="${enable_subdirs} rtems-gdb-stub"
		fi 
	;;
	*)
	;;
esac

# Check for PCI support
AC_MSG_NOTICE([Checking for PCI support of your BSP])
# No use to use a 'CHECKING' message; AC_CHECK_FUNC already reports...
AC_CHECK_FUNC(pci_find_device)

# BSP_commandline_string is not a function but
# the macro just checks if it can link the symbol
# which is good enough
AC_CHECK_FUNCS([BSP_commandline_string])
AC_CHECK_DECL([BSP_commandline_string],
	[AC_DEFINE([DECL_BSP_COMMANDLINE_STRING],1,[Whether BSP_commandline_string is declared in <bsp.h>])],
	,,[INCLUDES=bsp.h])

if test -d $srcdir/telnetd ; then
	config_files="$config_files telnetd/Makefile"
	enable_subdirs="${enable_subdirs} telnetd"
	AC_MSG_CHECKING([if bundled rtems_telnetd_initialize() takes 6 args])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <rtems.h>
	#include <rtems/telnetd.h>]],
	[[rtems_telnetd_initialize(0,0,0,0,0,0);]])],
	[AC_MSG_RESULT([Yes])
	 AC_CHECK_LIB([telnetd],[rtems_telnetd_initialize],
		[AC_MSG_NOTICE([Using bundled telnetd library])
		 have_bundled_telnetd=yes
		],
		[AC_MSG_NOTICE([Building unbundled telnetd support])
		 have_bundled_telnetd=no
		],
		TILLAC_RTEMS_CHECK_LIB_ARGS)
	],
	[AC_MSG_NOTICE([No; building unbundled telnetd support])]
	)
fi

if test -d $srcdir/miscUtils ; then
config_files="$config_files miscUtils/Makefile"
enable_subdirs="${enable_subdirs} miscUtils"
fi

if test -d $srcdir/monitor ; then
	if test "${enable_tecla}" = "no" ; then
		AC_MSG_NOTICE(['monitor' package disabled because you disabled TECLA -- the monitor would depend on it])
	else
		config_files="$config_files monitor/Makefile"
		enable_subdirs="${enable_subdirs} monitor"
	fi
fi

if test -d $srcdir/ntpNanoclock ; then
config_files="$config_files ntpNanoclock/Makefile"
enable_subdirs="${enable_subdirs} ntpNanoclock"
fi


if test -d $srcdir/cexp ; then
enable_subdirs="$enable_subdirs cexp"
AC_CONFIG_SUBDIRS([cexp])
AC_DEFINE(HAVE_CEXP,1,[Whether CEXP is installed])
have_cexp=yes
fi

if test -d $srcdir/svgmWatchdog ; then
	# Do we have BSP support for our watchdog
	AC_MSG_CHECKING([Looking for BSP support for the watchdog])
	if test -f $srcdir/svgmWatchdog/bsp_${enable_rtemsbsp}.c ; then
		AC_MSG_RESULT([OK, found BSP support])
		WATCHDOG_BSP_SUPPORT=bsp_${enable_rtemsbsp}
	# No BSP support - is this a known BSP with a E500 CPU ?
	elif case $enable_rtemsbsp in mvme3100 ) true ;; *) false;; esac ; then
		AC_MSG_RESULT([OK, using generic PPC-E500 support])
		WATCHDOG_BSP_SUPPORT=bsp_booke
	# No; if it is a i386 board then we hope it has SMIC
	elif test "${host_cpu}" = "i386" ; then
		AC_MSG_RESULT([MAYBE, using generic SMIC support; hoping your board has SMIC])
		WATCHDOG_BSP_SUPPORT=bsp_smic
	else
		AC_MSG_RESULT([FAILED. Watchdog not supported on this platform])
	fi
	if test "${WATCHDOG_BSP_SUPPORT+set}" = "set" ; then
		config_files="$config_files svgmWatchdog/Makefile"
		enable_subdirs="${enable_subdirs} svgmWatchdog"
		AC_SUBST([WATCHDOG_BSP_SUPPORT])
	fi
fi

if test "${enable_rtemsbsp}" = "uC5282" && test -d $srcdir/coldfUtils ; then
	config_files="$config_files coldfUtils/Makefile"
	enable_subdirs="${enable_subdirs} coldfUtils"
fi

if test "${host_cpu}" = "powerpc" && test -d $srcdir/efence ; then
# should really check if this is a >= 604 PPC
	AC_CHECK_HEADER([libcpu/pte121.h],
		[config_files="$config_files efence/Makefile"
		enable_subdirs="${enable_subdirs} efence"],
		[AC_MSG_NOTICE([No <libcpu/pte121.h> found; not building libefence])]dnl
	)
fi

if test "${host_cpu}" = "powerpc" && test -d $srcdir/altivec ; then
	AC_MSG_NOTICE([Checking if PPC_CACHE_ALIGNMENT is 32 bytes])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM(
[[
#include <rtems.h>
#include <rtems/powerpc/powerpc.h>
#include <rtems/score/powerpc.h>
#if PPC_CACHE_ALIGNMENT != 32
#error "Altivec support assumes cache-line size of 32 bytes"
#endif
]],[[]])],
		[AC_MSG_RESULT([Ok])
		config_files="$config_files altivec/Makefile"
		enable_subdirs="${enable_subdirs} altivec"],
		[AC_MSG_RESULT([No; not building AltiVec extension])]dnl
	)
fi

if test -d $srcdir/amdeth ; then
	if test "$ac_cv_func_pci_find_device" = "yes" ; then
		config_files="$config_files amdeth/Makefile"
		enable_subdirs="${enable_subdirs} amdeth"
	else
		AC_MSG_NOTICE([Your BSP doesn't seem to have PCI; not building the amdeth driver])
	fi
fi

if test -d $srcdir/drvLan9118 ; then
	case $enable_rtemsbsp in 
		uC5282 | beatnik)
			LANIPBASIC_SUPPORT="${enable_rtemsbsp}"
		;;
		*)
			if test "$ac_cv_func_pci_find_device" = "yes" ; then
				LANIPBASIC_SUPPORT=pci
			fi
		;;
	esac
	if test "${LANIPBASIC_SUPPORT+set}" = "set" ; then
		config_files="$config_files drvLan9118/Makefile"
		enable_subdirs="${enable_subdirs} drvLan9118"
		AC_SUBST(LANIPBASIC_SUPPORT)
	else
		AC_MSG_NOTICE([lanIpBasic package not supported by this BSP -- not built])
	fi
fi

# looking for bundled NFS;
build_rtems_nfs=no
AC_MSG_CHECKING([whether we should build unbundled NFS])
# Note the 5th argument; linking would fail because
# the application usually supplies rtems_bsdnet_config.
# In order to link, we create a dummy symbol.
AC_CHECK_LIB([nfs],[nfsMount],
	[AC_MSG_NOTICE([Using RTEMS bundled NFS])
	 AC_SUBST([NFSLIB],["-lnfs"])
	 if test -d $srcdir/rtemsNfs ; then
	 	AC_MSG_NOTICE([Building 'dirutils' only (from unbundled NFS)])
		build_rtems_nfs=dirutils_only
	 fi],
	[if test -d $srcdir/rtemsNfs ; then
		 AC_MSG_NOTICE([Building unbundled NFS])
		 build_rtems_nfs=yes
		 AC_SUBST([NFSLIB],["-lrtemsNfs -lnfsprot"])
	 fi],
	TILLAC_RTEMS_CHECK_LIB_ARGS
)
AC_MSG_RESULT([Building unbundled NFS: $build_rtems_nfs])

if test ! "$build_rtems_nfs" = "no" ; then
	if test "$build_rtems_nfs" = "yes" ; then
		config_files="$config_files rtemsNfs/proto/Makefile"
	fi
	config_files="$config_files rtemsNfs/Makefile"
	config_files="$config_files rtemsNfs/src/Makefile"
	enable_subdirs="${enable_subdirs} rtemsNfs"
fi

if test -d $srcdir/bsd_eth_drivers ; then
	if test "$ac_cv_func_pci_find_device" = "yes" ; then
		if test "${host_cpu}" = i386 && test ! "${have_bspext}" = yes ; then
			AC_MSG_NOTICE([Not building bsd_eth_drivers; on i386 you need libbspExt])
		else
			config_files="$config_files bsd_eth_drivers/Makefile"
			config_files="$config_files bsd_eth_drivers/libbsdport/Makefile"
			config_files="$config_files bsd_eth_drivers/if_pcn/Makefile"
			config_files="$config_files bsd_eth_drivers/if_le/Makefile"
			config_files="$config_files bsd_eth_drivers/if_em/Makefile"
			#FIXME: make these configurable options
			ENBL_82542_SUPPORT=NO
			ENBL_ICH8LAN_SUPPORT=YES
			AC_SUBST([ENBL_82542_SUPPORT])
			AC_SUBST([ENBL_ICH8LAN_SUPPORT])
			enable_subdirs="${enable_subdirs} bsd_eth_drivers"
		fi
	fi
fi

if test -d $srcdir/netboot ; then
	case "$enable_rtemsbsp" in
		svgm|beatnik|uC5282|mvme3100)
			if test "${enable_tecla}" = "no" ; then
				AC_MSG_NOTICE(['netboot' package disabled because you disabled TECLA -- the monitor would depend on it])
			else
				config_files="$config_files netboot/Makefile"
				enable_subdirs="${enable_subdirs} netboot"
				case "$enable_rtemsbsp" in
					svgm)
						netboot_compressed=yes
					;;
					*)
					;;
				esac
			fi
	;;
		*)
	;;
	esac
fi

if test -d $srcdir/system ; then
	case "$enable_rtemsbsp" in
		rce405)
		# real application is built somewhere else
		;;
		*)
			enable_subdirs="$enable_subdirs system"
			AC_CONFIG_SUBDIRS([system])
			AC_CONFIG_COMMANDS_POST([if test ! -h system/data ; then ln -s ../data system/data; fi])
		;;
	esac
fi

# an ugly hack so we can give programs that
# should run on the build host a different
# extension.
# The only way automake does not append $(EXEEXT)
# is if a PROGRAM is configure-substituted
# (see automake doc 'EXEEXT')
AC_SUBST([HOSTPROGRAM],['$(HOSTPROG)$(HOSTEXEEXT)'])
AC_SUBST([installexechostbinprogs],[install-exechostbinPROGRAMS])

AM_CONDITIONAL([HAVE_CEXP],[test "$have_cexp" = "yes"])
AM_CONDITIONAL([BUILD_RTEMS_NFS],[test "$build_rtems_nfs" = "yes"])
AM_CONDITIONAL([HAVE_BUNDLED_TELNETD],[test "$have_bundled_telnetd" = "yes"])
AM_CONDITIONAL([NETBOOT_COMPRESSED],[test "$netboot_compressed" = "yes"])

AC_CONFIG_FILES([$config_files])

AC_OUTPUT
