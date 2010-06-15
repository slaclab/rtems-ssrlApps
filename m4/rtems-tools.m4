# Check for critical programs we need for building
AC_DEFUN([TILLAC_RTEMS_CHECK_TOOLS],
	[
     # Cannot conditionally use AC_PROG_CC, AC_PROG_CXX, AM_PROG_AS
	 # This is an autoconf 'feature'. Therefore we use an ugly hack
	 # to pass the gcc tests on a build system that has no native
	 # compilers.
	 if TILLAC_RTEMS_NOT_CONFIG_TOP; then : ; else
	   CC="$SHELL $srcdir/dummycxx"
	   CXX="$SHELL $srcdir/dummycxx"
	 fi
	 AC_PROG_CC
	 AC_PROG_CXX
	 AM_PROG_AS
	 if TILLAC_RTEMS_NOT_CONFIG_TOP; then
	   AC_SUBST([GCC])
	   AC_PROG_CPP
	   AC_CHECK_PROGS([HOSTCC], gcc cc)
	   RTEMS_CHECK_TOOL([AR],ar)
	   RTEMS_CHECK_TOOL([LD],ld)
	   RTEMS_CHECK_TOOL([NM],nm)
	   RTEMS_CHECK_TOOL([OBJCOPY],objcopy)
	   RTEMS_CHECK_TOOL([RANLIB],ranlib)
	   AC_PROG_INSTALL
	   AC_CHECK_PROG([INSTALL_IF_CHANGE],[install-if-change],[install-if-change],[${INSTALL}])
	 fi]dnl
)

dnl m4_syscmd is executed when aclocal is run
m4_syscmd([cat - > dummycxx <<'EOF_'
#!/bin/sh
# dummy 'compiler' to just pass the 'gcc' test
ofile="a.out"
while getopts "cgo:" opt ; do
	echo found opt $opt
	case $opt in
		o)
		ofile="$OPTARG"
		o_seen=yes
		shift
		;;
		c)
		c_seen=yes
		;;
		g)
		;;
		*)
			exit 1;
		;;
	esac
	shift
	OPTIND=1
done
if [ x"${c_seen}" = "xyes" ] && [ ! x"${o_seen}" = "xyes" ] ; then
	srcnam=`basename $1 | sed -e 's/[.][^.]*$//g'`
	ofile="${srcnam}.o"
fi
echo '#!/bin/sh'                > $ofile
echo 'echo "dummy compilation"' >> $ofile
chmod a+x $ofile

EOF_
])
