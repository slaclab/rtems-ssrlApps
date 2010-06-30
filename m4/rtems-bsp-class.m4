# Certain BSPs are just variants. This macro sets
# the RTEMS_BSP_CLASS variable to the name of a 'base' BSP.
# Note that this matches mostly what RTEMS_BSP_FAMILY used to
# be...
#
AC_DEFUN([TILLAC_RTEMS_BSP_CLASS],
	[case "${rtems_bsp}" in
		pc?86*)
			RTEMS_BSP_CLASS=pc386
		;;
		psim*)
			RTEMS_BSP_CLASS=psim
		;;
		mcp750|mvme2100|mvme2307|mtx603e)
			RTEMS_BSP_CLASS=motorola_powerpc
		;;
		*)
			RTEMS_BSP_CLASS="${rtems_bsp}"
		;;
	esac
	AC_SUBST([RTEMS_BSP_CLASS])]dnl
)
