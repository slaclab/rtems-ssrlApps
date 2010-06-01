# master 'macro' for our package
AC_DEFUN([TILLAC_RTEMS_MACROS],[
	AC_CANONICAL_HOST	
	TILLAC_RTEMS_CHECK_TOOLS
	# Must add this BEFORE TILLAC_RTEMS_SETUP
	# so that the dummy-top 'config.status' also
	# knows how to make a config.h...
	AM_CONFIG_HEADER(config.h)
	TILLAC_RTEMS_SETUP
	case $host_os in
		*[[rR]][[tT]][[eE]][[mM]][[sS]]*)
			TILLAC_RTEMS_BSP_POSTLINK_CMDS
		;;
	esac]dnl
)
