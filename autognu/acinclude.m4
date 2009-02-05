#
# TILLAC_CVSTAG([$Name$], [pattern])
#
# Strip $Name$ from first argument extracting
# the CVS tag. If the second optional argument is
# given then it must specify a regexp pattern that
# is stripped from the resulting tag.
#
# This macro is intended to be used as follows:
#
# AC_INIT(package, TILLAC_CVSTAG([$Name$]))
#
# CVS inserts a tag which is extracted by this macro.
# Thus the CVS tag of 'configure.ac' is propagated to
# the PACKAGE_VERSION and VERSION Makefile variables.
#
# E.g., a checked-out copy may be tagged 'Release_foo'
# and using the macro:
#
# AC_INIT(package, TILLAC_CVSTAG([$Name$],'Release_'))
#
# results in the Makefile defining
#
#   PACKAGE_VERSION=foo
# 
# NOTE: if [] characters are required in the regexp pattern
# then they must be quoted ([[ ]]).
#
m4_define(TILLAC_CVSTAG,
	[m4_if(
		_TILLAC_CVSTAG($1,[$2]),
		,
		[untagged],
		_TILLAC_CVSTAG($1,[$2]))]dnl
)
m4_define(_TILLAC_CVSTAG,
	[m4_bregexp(
		[$1],
		\([[$]]Name:[[ ]]*\)\($2\)\([[^$]]*\)[[$]],
		\3)]dnl
)

# Declare --enable-rtemsbsp --with-rtems-top options
#
# TILLAC_RTEMS_OPTIONS
AC_DEFUN([TILLAC_RTEMS_OPTIONS],
	[AC_ARG_ENABLE(rtemsbsp,
		AC_HELP_STRING([--enable-rtemsbsp="bsp1 bsp2 ..."],
			[BSPs to include in build (ignore bsps not found in RTEMS installation)]dnl
		)
	)
	AC_ARG_WITH(rtems-top,
		AC_HELP_STRING([--with-rtems-top=<rtems installation topdir>],
			[point to RTEMS installation]dnl
		)
	)
	AC_ARG_WITH(extra-incdirs,
		AC_HELP_STRING([--with-extra-incdirs=<additional header dirs>],
			[point to directories with additional headers]dnl
		)
	)
	AC_ARG_WITH(extra-libdirs,
		AC_HELP_STRING([--with-extra-libdirs=<additional library dirs (w/o -L)>],
			[point to directories with additional libraries]dnl
		)
	)
	AC_ARG_WITH(hostbindir,
		AC_HELP_STRING([--with-hostbindir=<installation dir for native binaries>],
			[default is <prefix>/host/<build_alias>/bin],
		),
		[AC_SUBST([hostbindir],[$with_hostbindir])],
		[AC_SUBST([hostbindir],['$(prefix)/host/$(build_alias)/bin'])]
	)
	AC_ARG_ENABLE([std-rtems-installdirs],
		AC_HELP_STRING([--enable-std-rtems-installdirs],
		[install directly into
		the RTEMS installation directories; by default a location *outside*
		of the standard location is used. If you don't use this option you
		can also fine-tune the installation using the usual --prefix, 
		--exec-prefix, --libdir, --includedir etc. options]dnl
		)
	)]dnl
)

# Find out if host_os is *rtems*;
#
# Result is exit status, i.e., this macro can e.g., be used
# in a 'if MACRO ; then list; fi' statement.
#
# TILLAC_RTEMS_HOSTOS_IS_RTEMS
AC_DEFUN([TILLAC_RTEMS_HOSTOS_IS_RTEMS],
	[AC_REQUIRE([AC_CANONICAL_HOST])
    case "${host_os}" in *rtems* ) : ;; *) false;; esac]dnl
)


# Find out if either '--with-rtems-top' was given or
# host_os is *rtems* (or both).
#
# Result is exit status, i.e., this macro can e.g., be used
# in a 'if MACRO ; then list; fi' statement.
#
# TILLAC_RTEMS_OS_IS_RTEMS
AC_DEFUN([TILLAC_RTEMS_OS_IS_RTEMS],
	[AC_REQUIRE([AC_CANONICAL_HOST])
    test "${with_rtems_top+set}" = "set" || TILLAC_RTEMS_HOSTOS_IS_RTEMS]dnl
)

# Find out if this is a multilibbed RTEMS installation
#
# Result is exit status, i.e., this macro can e.g., be used
# in a 'if MACRO ; then list; fi' statement.
#
# TILLAC_RTEMS_CPUKIT_MULTILIB
AC_DEFUN([TILLAC_RTEMS_CPUKIT_MULTILIB],
	[AC_REQUIRE([AC_CANONICAL_HOST])
	AC_REQUIRE([TILLAC_RTEMS_OPTIONS])
	test -d ${with_rtems_top}/${host_cpu}-${host_os}/include]dnl
)

# Verify that the --with-rtems-top option has been given
# and that the directory it specifies has a subdirectory
# ${with_rtems_top}/${host_cpu}-${host_os}
#
# Throw an error if this test fails.
#
# TILLAC_RTEMS_CHECK_TOP
#
AC_DEFUN([TILLAC_RTEMS_CHECK_TOP],
	[AC_REQUIRE([AC_CANONICAL_HOST])
    AC_REQUIRE([TILLAC_RTEMS_OPTIONS])
    if TILLAC_RTEMS_OS_IS_RTEMS ; then
        if test ! "${with_rtems_top+set}" = "set" ; then
            AC_MSG_ERROR([No RTEMS topdir given; use --with-rtems-top option])
        fi
        AC_MSG_CHECKING([Checking RTEMS installation topdir])
        if test ! -d $with_rtems_top/${host_cpu}-${host_os}/ ; then
            AC_MSG_ERROR([RTEMS topdir $with_rtems_top/${host_cpu}-${host_os}/ not found])
        fi
        AC_MSG_RESULT([OK])
    fi
    ]dnl
)

# Assemble a list of BSPs in 'enable_rtemsbsp'
#
#  a) if 'enable_rtemsbsp' is not set when this macro is expanded
#     then set it to a (whitespace separated) list of all
#     BSPs found under ${with_rtems_top}/${host_cpu}-${host_os}/
#  b) if 'enable_rtemsbsp' is already set then remove all BSPs
#     from it which are not installed under
#     ${with_rtems_top}/${host_cpu}-${host_os}/
#
# -> After expanding this macro 'enable_rtemsbsp' contains a list
#    of all BSPs that are installed and -- if 'enable_rtemsbsp' was
#    initially set -- which are mentioned in 'enable_rtembsp'.
#
# TILLAC_RTEMS_CHECK_BSPS
#
# NOTE: This macro *modifies* the 'enable_rtemsbsp' variable.
#
AC_DEFUN([TILLAC_RTEMS_CHECK_BSPS],
	[AC_REQUIRE([TILLAC_RTEMS_OPTIONS])
    if test ! "${enable_rtemsbsp+set}" = "set" ; then
        _tillac_rtems_bsplist="`ls $with_rtems_top/${host_cpu}-${host_os}/ | tr '\n\r' '  '`"
	else
		_tillac_rtems_bsplist=$enable_rtemsbsp
	fi
	enable_rtemsbsp=
	AC_MSG_CHECKING([Looking for RTEMS BSPs $_tillac_rtems_bsplist])
	for _tillac_rtems_bspcand in $_tillac_rtems_bsplist ; do
		if test -d $with_rtems_top/${host_cpu}-${host_os}/$_tillac_rtems_bspcand/lib/include ; then
			if test "${enable_rtemsbsp}"xx = xx ; then
				enable_rtemsbsp="$_tillac_rtems_bspcand"
			else
				enable_rtemsbsp="$_tillac_rtems_bspcand $enable_rtemsbsp"
			fi
		fi
	done
	if test "$enable_rtemsbsp"xx = "xx" ; then
		AC_MSG_ERROR("No BSPs found")
	else
		AC_MSG_NOTICE([found \'$enable_rtemsbsp\'])
	fi]dnl
)

#
# Check if the 'enable_rtemsbsp' variable lists a single
# or multiple BSPs and set exit status accordingly:
#
# Result is exit status, i.e., this macro can e.g., be used
# in a 'if MACRO ; then list; fi' statement.
#
# true  - if enable_rtembsp lists more than one BSP
# false - otherwise
#
# if TILLAC_RTEMS_CHECK_MULTI_BSPS ; then list ; fi
AC_DEFUN([TILLAC_RTEMS_CHECK_MULTI_BSPS],
	[AC_REQUIRE([TILLAC_RTEMS_CHECK_BSPS])
	( _tillac_rtems_multi_bsps=no
    for _tillac_rtems_bspcand in $enable_rtemsbsp ; do
        if test "$_tillac_rtems_multi_bsps" = "no" ; then
            _tillac_rtems_multi_bsps=maybe
        else
            _tillac_rtems_multi_bsps=yes
        fi
    done
    test "$_tillac_rtems_multi_bsps" = "yes")]dnl
)

# Takes one argument
#
# TILLAC_RTEMS_TRIM_CONFIG_DIR(dirvar)
#
# If 'srcdir' is a absolute path (a string
# starting with '/' then set 'dirvar' to the
# empty string; otherwise (srcdir is a relative path)
# set 'dirvar' to '../'.
#
# This macro can be used to find 'srcdir' should 
# configure decide to step into a subdirectory inside
# a build tree.
#
AC_DEFUN([TILLAC_RTEMS_TRIM_CONFIG_DIR],
	[AC_MSG_NOTICE([Trimming source directory])
	# leave absolute path alone, relative path needs
	# to step one level up
	case $srcdir in
		/* )
			$1=
		;;
		*)
			$1=../
		;;
	esac]dnl
)

# This macro assembles a list of RTEMS CPUs and if the list
# contains more than one member then it creates a build subdirectory
# for each CPU architecture, 'chdirs' into the subdirectory and recursively
# configures for the CPU.
#
# Several options affect the behavior of this macro:
#
#   --with-rtems-top
#   --host
#   --enable-rtemsbsp
#
#  1) if --with-rtems-top is NOT given (NOT RTEMS) OR if
#     --host=<xyz>-rtems was given (RTEMS CPU defined by user)
#     THEN the macro does nothing.
#
#  2) ELSE (--with-rtems-top given but --host is NOT *rtems* 
#     i)   assemble a list of all '<cpu>-rtems*' subdirectories
#          under ${with_rtems_top}
#
#     ii)  if --enable-rtemsbsp was given then remove CPU
#          architectures not being required by any of the listed
#          BSPS.
#
#     iii) for each remaining CPU architecture create a subdirectory,
#          chdir there and recursively call 'configure' again with
#          the original arguments but --host=<cpu>-rtems appended.
#
# Note: this macro should not be used directly. It is expanded from
#       TILLAC_RTEMS_SETUP
#
# TILLAC_RTEMS_CONFIG_CPUS_RECURSIVE
AC_DEFUN([TILLAC_RTEMS_CONFIG_CPUS_RECURSIVE],
	[if test ! "${with_rtems_top+set}" = "set" || TILLAC_RTEMS_HOSTOS_IS_RTEMS ; then : ; else
	# with_rtems_top is set but host_os is not *rtems*, i.e.,
	# we have to figure out a list of CPUs/arches that are installed.
	AC_MSG_CHECKING([for all installed CPUs/architectures])
	_tillac_rtems_cpulist="`(cd $with_rtems_top; ls -d *-rtems* | tr '\n\r' '  ')`"
	AC_MSG_RESULT([Found: $_tillac_rtems_cpulist])
	# if 'enable-rtemsbsp' was given then filter away
	# architectures that match none of the BSPs
	if test "${enable_rtemsbsp+set}" = "set" ; then
		# convert space separated list into ORed (|) pattern
		_tillac_rtems_bspfilt=`echo "$enable_rtemsbsp" | sed -e 's/[[ \t]]\+/|/g'`
		AC_MSG_NOTICE([Filtering CPU/architecture list against bsps: $_tillac_rtems_bspfilt])
		_tillac_rtems_cpuall="$_tillac_rtems_cpulist"
		_tillac_rtems_cpulist=""
		AC_MSG_CHECKING([CPU/architectures matching requested BSPs])
		for _tillac_rtems_cpucand in $_tillac_rtems_cpuall ; do
			# look for directories which have a 'Makefile.inc'
			for _tillac_rtems_bspcand in `(cd $with_rtems_top/$_tillac_rtems_cpucand ; ls */Makefile.inc | tr '\n\r' '  ')` ; do
				AC_MSG_NOTICE([testing $_tillac_rtems_bspcand])
				# reduce to bsp name
				_tillac_rtems_bspcand=`dirname $_tillac_rtems_bspcand`
				if eval "case `echo $_tillac_rtems_bspcand` in $_tillac_rtems_bspfilt) : ;; *) false ;; esac" ; then
					_tillac_rtems_cpulist="$_tillac_rtems_cpulist $_tillac_rtems_cpucand"
				fi
			done
		done
		AC_MSG_RESULT([found: $_tillac_rtems_cpulist])
	fi
	if test "$_tillac_rtems_cpulist"xx = "xx" ; then
		AC_MSG_ERROR([No RTEMS architectures found])
	fi
	# Create directory and configure
	for _tillac_rtems_cpucand in $_tillac_rtems_cpulist ; do
		if test -d $_tillac_rtems_cpucand || mkdir $_tillac_rtems_cpucand ; then : ; else
			AC_MSG_ERROR([Unable to create subdirectory $_tillac_rtems_cpucand])
		fi
		TILLAC_RTEMS_TRIM_CONFIG_DIR(_tillac_rtems_config_dir)
		# SUB-CONFIGURE
		AC_MSG_NOTICE([Running $_tillac_rtems_config_dir/[$]0 "$ac_configure_args --host=$_tillac_rtems_cpucand" in "'$_tillac_rtems_cpucand'" subdir])
		eval \( cd $_tillac_rtems_cpucand \; $SHELL $_tillac_rtems_config_dir/"[$]0" $ac_configure_args --host=$_tillac_rtems_cpucand \)
	done
	AC_MSG_NOTICE([Creating cpu/arch level makefile])
    AC_SUBST(the_subdirs,[$_tillac_rtems_cpulist])
	AC_SUBST(the_distsub,['$(firstword '"$_tillac_rtems_cpulist"')'])
	_tillac_rtems_recursing=yes
	false
	fi]dnl
)

# This macro
#
#  - removes --enable-rtemsbsp options from the current commandline
#  - for each BSP listed in '${enable_rtemsbsp}'
#     i)   creates a BSP subdirectory in the build tree
#     ii)  chdirs into the subdirectory
#     iii) figures out a few BSP-specific build settings
#          (cflags, gcc specs, ...)
#     iv)  configures for the BSP passing the properties determined
#          under iii) along to 'configure' on the commandline.
#          Also, --enable-rtemsbsp=<this_bsp> is appended to the 
#          configure commandline.
#
# Note: this macro should not be used directly. It is expanded from
#       TILLAC_RTEMS_SETUP
#
# TILLAC_RTEMS_CONFIG_BSPS_RECURSIVE
AC_DEFUN([TILLAC_RTEMS_CONFIG_BSPS_RECURSIVE],
	[if test ! "${RTEMS_TILL_MAKEVARS_SET}" = "YES"; then
		# strip all --enable-rtemsbsp options from original
		# commandline
		AC_MSG_NOTICE([Stripping --enable-rtemsbsp option(s) from commandline])
		_tillac_rtems_config_args=""
	    eval for _tillac_rtems_arg in $ac_configure_args \; do case \$_tillac_rtems_arg in --enable-rtemsbsp\* \) \;\; \*\) _tillac_rtems_config_args=\"\$_tillac_rtems_config_args \'\$_tillac_rtems_arg\'\" \;\; esac done
		AC_MSG_NOTICE([Commandline now: $_tillac_rtems_config_args])
			
		AC_MSG_NOTICE([Creating BSP subdirectories and sub-configuring])
		TILLAC_RTEMS_SAVE_MAKEVARS
		for _tillac_rtems_bsp in $enable_rtemsbsp ; do
			if test ! -d $_tillac_rtems_bsp ; then
				AC_MSG_CHECKING([Creating $_tillac_rtems_bsp])
				if mkdir $_tillac_rtems_bsp ; then
					AC_MSG_RESULT([OK])
				else
					AC_MSG_ERROR([Failed])
				fi
			fi
			TILLAC_RTEMS_TRIM_CONFIG_DIR(_tillac_rtems_config_dir)
			TILLAC_RTEMS_RESET_MAKEVARS
			TILLAC_RTEMS_MAKEVARS(${host_cpu}-${host_os},$_tillac_rtems_bsp)
			tillac_rtems_cppflags="$tillac_rtems_cppflags -I$with_rtems_top/${host_cpu}-${host_os}/$_tillac_rtems_bsp/lib/include"
			TILLAC_RTEMS_EXPORT_MAKEVARS(${host_cpu}-${host_os},$_tillac_rtems_bsp)
			AC_MSG_NOTICE([Running $_tillac_rtems_config_dir/[$]0 $_tillac_rtems_config_args --enable-rtemsbsp=$_tillac_rtems_bsp in $_tillac_rtems_bsp subdir])
			eval \( cd $_tillac_rtems_bsp \; $SHELL $_tillac_rtems_config_dir/"[$]0" $_tillac_rtems_config_args --enable-rtemsbsp=$_tillac_rtems_bsp \)
		done
		TILLAC_RTEMS_RESET_MAKEVARS
		AC_MSG_NOTICE([Creating toplevel makefile])
	    AC_SUBST(the_subdirs,[$enable_rtemsbsp])
		AC_SUBST(the_distsub,['$(firstword '"$enable_rtemsbsp"')'])
	fi]dnl
)

# This macro saves the BSP-specific variables (such as CC, CXX, CPP, ...)
# into intermediate variables from where they can be restored
# with TILLAC_RTEMS_RESET_MAKEVARS
#
# Note: this macro should not be used directly. It is expanded from
#       TILLAC_RTEMS_SETUP
#
# TILLAC_RTEMS_SAVE_MAKEVARS
AC_DEFUN([TILLAC_RTEMS_SAVE_MAKEVARS],
	[
	tillac_rtems_cc_orig="$CC"
	tillac_rtems_cxx_orig="$CXX"
	tillac_rtems_ccas_orig="$CCAS"
	tillac_rtems_cpp_orig="$CPP"
	tillac_rtems_ldflags_orig="$LDFLAGS"
	tillac_rtems_bsp_family_orig=""
	tillac_rtems_bsp_insttop_orig=""]dnl
)

# This macro restores the BSP-specific variables (such as CC, CXX, CPP, ...)
# from intermediate variables that were set by TILLAC_RTEMS_SAVE_MAKEVARS
#
# Note: this macro should not be used directly. It is expanded from
#       TILLAC_RTEMS_SETUP
#
# TILLAC_RTEMS_RESET_MAKEVARS
AC_DEFUN([TILLAC_RTEMS_RESET_MAKEVARS],
	[
	RTEMS_TILL_MAKEVARS_SET=NO
	CC="$tillac_rtems_cc_orig"
	CXX="$tillac_rtems_cxx_orig"
	CCAS="$tillac_rtems_ccas_orig"
	CPP="$tillac_rtems_cpp_orig"
	LDFLAGS="$tillac_rtems_ldflags_orig"
	RTEMS_BSP_FAMILY="$tillac_rtems_family_orig"
	RTEMS_BSP_INSTTOP="$tillac_rtems_insttop_orig"]dnl
)

# Determine critical, BSP-specific build parameters from the
# RTEMS makefiles. These parameters are cflags, gccspecs, ...
#
# Note: this macro should not be used directly. It is expanded from
#       TILLAC_RTEMS_SETUP
#
# TILLAC_RTEMS_MAKEVARS(HOST_SYSTEM, BSP)
AC_DEFUN([TILLAC_RTEMS_MAKEVARS],
	[
	AC_MSG_CHECKING([Determining RTEMS Makefile parameters for BSP:])
dnl DOWNEXT is set in leaf.cfg and we don't include that
	if _tillac_rtems_result=`make -s -f - rtems_makevars <<EOF_
include $with_rtems_top/$1/$2/Makefile.inc
include \\\$(RTEMS_CUSTOM)
include \\\$(CONFIG.CC)

rtems_makevars:
	@echo tillac_rtems_cpu_cflags=\'\\\$(CPU_CFLAGS) \\\$(AM_CFLAGS)\'
	@echo tillac_rtems_gccspecs=\'\\\$(GCCSPECS)\'
	@echo tillac_rtems_cpu_asflags=\'\\\$(CPU_ASFLAGS)\'
	@echo tillac_rtems_ldflags=\'\\\$(AM_LDFLAGS) \\\$(LDFLAGS)\'
	@echo tillac_rtems_cppflags=
	@echo RTEMS_BSP_FAMILY=\'\\\$(RTEMS_BSP_FAMILY)\'
	@echo RTEMS_BSP_INSTTOP=\'\\\$(PROJECT_RELEASE)\'
EOF_
` ; then
	AC_MSG_RESULT([OK: $_tillac_rtems_result])
	else
	AC_MSG_ERROR([$_tillac_rtems_result])
	fi
	# propagate cpu_cflags and gccspecs into currently executing shell
	eval $_tillac_rtems_result
	export RTEMS_BSP_INSTTOP="$RTEMS_BSP_INSTTOP"
	export RTEMS_BSP_FAMILY="$RTEMS_BSP_FAMILY"]dnl
)

# Export the set of critical, BSP-specific build parameters 
# (cflags, gccspecs, ...) that were determined by TILLAC_RTEMS_MAKEVARS
# into the environment.
#
# This macro takes two (optional) arguments: 
#
#    <HOST_SYSTEM> and <LIBSUBDIR>
#
# The macro also, adds the paths listed in the --with-extra-incdirs and
# --with-extra-libdirs options to the cppflags and ldflags, respectively
# (adding -I, -L). If 'LIBSUBDIR' is given then it is appended to
# any of the directories listed in --with-extra-libdirs (if the LIBSUBDIR
# exists) there.
#
# Furthermore, if it is determined that RTEMS was configured for
# a multilibbed cpukit then
#  i)   -I${with_rtems_top}/${host_cpu}-${host_os}/include 
#       is added to the cppflags (if the directory exists)
#  ii)  -B${with_rtems_top}/<HOST_SYSTEM>/lib is added to the
#       gcc specs so that multilibs are found (workaround for
#       a bug in the rtems 4.9.0 makefiles)
#
# Note: this macro should not be used directly. It is expanded from
#       TILLAC_RTEMS_SETUP
#
# TILLAC_RTEMS_EXPORT_MAKEVARS([HOST_SYSTEM],[LIBSUBDIR])
AC_DEFUN([TILLAC_RTEMS_EXPORT_MAKEVARS],
	[
	AC_MSG_CHECKING([Checking if RTEMS CC & friends MAKEVARS are already set])
	if test ! "${RTEMS_TILL_MAKEVARS_SET}" = "YES"; then
		AC_MSG_RESULT([No (probably a multilibbed build)]) 
		export RTEMS_TILL_MAKEVARS_SET=YES
		# if this is a multilibbed cpukit we need to include
		if test -d $with_rtems_top/${host_cpu}-${host_os}/include ; then
			tillac_rtems_cppflags="$tillac_rtems_cppflags -I$with_rtems_top/${host_cpu}-${host_os}/include"
			# and since the RTEMS (4.9) makefiles seem to be broken
			# for multilibbed cpukits (fail to add -B <libdir>) we
			# do it here
			tillac_rtems_gccspecs="$tillac_rtems_gccspecs -B $with_rtems_top/$1/lib"
		fi
		if test "${with_extra_incdirs+set}" = "set" ; then
			for tillac_extra_incs_val in ${with_extra_incdirs} ; do
				tillac_rtems_cppflags="$tillac_rtems_cppflags -I$tillac_extra_incs_val"
			done
		fi
		if test "${with_extra_libdirs+set}" = "set" ; then
			for tillac_extra_libs_val in ${with_extra_libdirs} ; do
				if test -d $tillac_extra_libs_val/$2 ; then
					tillac_rtems_ldflags="$tillac_rtems_ldflags -L$tillac_extra_libs_val/$2"
				else
					tillac_rtems_ldflags="$tillac_rtems_ldflags -L$tillac_extra_libs_val"
				fi
			done
		fi
#export forged CC & friends so that they are used by sub-configures, too
		export CC="$CC $tillac_rtems_gccspecs $tillac_rtems_cpu_cflags $tillac_rtems_cppflags"
		export CXX="$CXX $tillac_rtems_gccspecs $tillac_rtems_cpu_cflags $tillac_rtems_cppflags"
		export CCAS="$CCAS $tillac_rtems_gccspecs $tillac_rtems_cpu_asflags -DASM"
		export CPP="$CPP $tillac_rtems_gccspecs $tillac_rtems_cppflags"
#		export CFLAGS="$CFLAGS $tillac_rtems_cpu_cflags"
#		export CXXFLAGS="$CXXFLAGS  $tillac_rtems_cpu_cflags"
#		export CCASFLAGS="$CCASFLAGS $tillac_rtems_cpu_asflags -DASM"
#		export CPPFLAGS="$CPPFLAGS $tillac_rtems_cppflags"
		export LDFLAGS="$LDFLAGS $tillac_rtems_ldflags"
	else
		AC_MSG_RESULT([yes])
	fi]dnl
)

#
# Prepare for a multilibbed build
#  - check for presence of 'config-ml.in'
#  - expand AM_ENABLE_MULTILIB(MAKEFILE, REL-TO-TOP-SRCDIR)
#  - expand TILLAM_MULTISUB_INSTALLDIR (workaround so that
#    multilibs are installed into proper subdir.
#  - make sure  'enable_multilib' is set to 'no' if it was initially
#    unset; yet another little workaround...
#
# TILLAC_RTEMS_MULTILIB([MAKEFILE], [REL-TO-TOP-SRCDIR])
AC_DEFUN([TILLAC_RTEMS_MULTILIB],
	[if test -f ${srcdir}/config-ml.in || test -f $(srcdir)/../config-ml.in ; then
		AM_ENABLE_MULTILIB([$1],[$2])
		# install multilibs into MULTISUBDIR
		TILLAM_MULTISUB_INSTALLDIR
dnl		AC_SUBST(libdir,[${libdir}'$(MULTISUBDIR)'])
		# in order to properly build multilibs in sub-libraries it seems we
		# must pass the --enable-multilibs arg to sub-configures or multilibs
		# are not built there.
		# To work around, we simply set the default to 'no' so the user must
		# say --enable-multilib to get them.
		if test ! "${enable_multilib+set}" = "set" ; then
		    multilib=no
		fi
	else
		enable_multilib=no
	fi]dnl
)

# Emit two macros
#
#   'RTEMS_VERSION_LATER_THAN(major,minor,revision)'
#
# and
#
#   'RTEMS_VERSION_ATLEAST(major,minor,revision)'
#
# in config.h so applications can test for a particular
# RTEMS version and conditionally deal with API changes
# and other stuff.
# Note that sometimes (mostly during x.y.99  pre-release
# phases) such changes happen w/o an associated change in
# revision number ;-(.
#
#dnl TILLAC_RTEMS_VERSTEST
AC_DEFUN([TILLAC_RTEMS_VERSTEST],
	[AH_VERBATIM([RTEMS_VERSION_TEST],
				[
#ifndef RTEMS_VERSION_LATER_THAN
#define RTEMS_VERSION_LATER_THAN(ma,mi,re) \
	(    __RTEMS_MAJOR__  > (ma)	\
	 || (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__  > (mi))	\
	 || (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__ == (mi) && __RTEMS_REVISION__ > (re)) \
    )
#endif
#ifndef RTEMS_VERSION_ATLEAST
#define RTEMS_VERSION_ATLEAST(ma,mi,re) \
	(    __RTEMS_MAJOR__  > (ma)	\
	|| (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__  > (mi))	\
	|| (__RTEMS_MAJOR__ == (ma) && __RTEMS_MINOR__ == (mi) && __RTEMS_REVISION__ >= (re)) \
	)
#endif
	            ]dnl
	)]dnl
)

AC_DEFUN([TILLAC_RTEMS_RECURSIVE_CONFIG],
	[AC_REQUIRE([TILLAC_RTEMS_OPTIONS])dnl
	AC_ARG_ENABLE(multilib,
		[  --enable-multilib    build many library versions],
		[case "$enableval" in
		  yes) multilib=yes ;;
		  no)  multilib=no ;;
		  *)   AC_MSG_ERROR([bad value $enableval for multilib option]) ;;
		 esac],
		[multilib=no]dnl
	)
	AC_ARG_VAR([TILLAC_RTEMS_CPUS_RECURSING],[Internal use; do NOT set in environment nor on commandline])
	# a few basic checks
	if test "${enable_multilib}" = "yes" && test "${enable_rtemsbsp+set}" = "set" ; then
		AC_MSG_ERROR([Cannot --enable-rtemsbsp AND --enable-multilib; build either multilibs or for particular BSP(s)])
	fi
	# Automake wouldn't let us check for these
	# tools only when we need them (when determining
	# proper parameters for particular BSPs)
	TILLAC_RTEMS_CHECK_TOOLS
	if TILLAC_RTEMS_OS_IS_RTEMS ; then
		if TILLAC_RTEMS_CONFIG_CPUS_RECURSIVE($1) ; then
			# have a --host=xxx-rtems option; recurse into BSPs
			# multilibs are handled inside of CPU recursion
			TILLAC_RTEMS_CHECK_BSPS
			if TILLAC_RTEMS_CONFIG_BSPS_RECURSIVE($1) ; then
				## REALCONFIG -- but we should not get here ##
				false
			fi
		fi
	else
	AC_MSG_NOTICE([Not building for RTEMS])
	# not rtems
	## REALCONFIG ##
	false
	fi]dnl
)

# Grand Master Macro for RTEMS configuration.
#
# This sets up most things for a RTEMS configuration
# for multiple CPU-arches and BSPs.
#
# A package may add the optional (literal) argument
# 'domultilib'. In this case, multilib support is enabled
# and the user may configure with '--enable-multilib'.
# Note that the package must properly support multilibs!
#
# If the host system is not RTEMS (no with-rtems-top given)
# then this macro does *nothing*.
# 
# TILLAC_RTEMS_SETUP([domultilib])
AC_DEFUN([TILLAC_RTEMS_SETUP],
    [AC_REQUIRE([TILLAC_RTEMS_OPTIONS])dnl
	AM_CONDITIONAL(OS_IS_RTEMS,[TILLAC_RTEMS_OS_IS_RTEMS])
	if TILLAC_RTEMS_CONFIG_CPUS_RECURSIVE ; then
	m4_if($1,domultilib,
		[TILLAC_RTEMS_MULTILIB([Makefile],[.])],
		[AC_REQUIRE([TILLAC_RTEMS_OPTIONS])dnl
		if test "${enable_multilib}" = "yes" ; then
		 	AC_MSG_ERROR(["multilibs not supported, sorry"])
		fi]dnl
	)
	if TILLAC_RTEMS_OS_IS_RTEMS ; then
		TILLAC_RTEMS_CHECK_TOP
		AC_ARG_VAR([RTEMS_TILL_MAKEVARS_SET],[Internal use; do NOT set in environment nor on commandline])
		AC_ARG_VAR([DOWNEXT],[extension of downloadable binary (if applicable)])
		AC_ARG_VAR([APPEXEEXT], [extension of linked binary])
		AC_ARG_VAR([RTEMS_BSP_FAMILY],[Internal use; do NOT set in environment nor on commandline])
		AC_ARG_VAR([RTEMS_BSP_INSTTOP],[Internal use; do NOT set in environment nor on commandline])
		if test "$1" = "domultilib" && test "$enable_multilib" = "yes" ; then 
			if test "${enable_rtemsbsp+set}" = "set" ; then
				AC_MSG_ERROR([Cannot --enable-rtemsbsp AND --enable-multilib; build either multilibs or for particular BSP(s)])
			fi
			TILLAC_RTEMS_EXPORT_MAKEVARS(${host_cpu}-${host_os},)
		else
			TILLAC_RTEMS_CHECK_BSPS
		fi
		if test ! "${RTEMS_TILL_MAKEVARS_SET}" = "YES"; then
			TILLAC_RTEMS_CONFIG_BSPS_RECURSIVE(makefile)
			_tillac_rtems_recursing=yes
		else
			TILLAC_RTEMS_FIXUP_PREFIXES
dnl set those in the configure script so that 'configure' uses these settings when trying to compile stuff
dnl		AC_SUBST(rtems_gccspecs,   [$tillac_rtems_gccspecs])
dnl		AC_SUBST(rtems_cpu_cflags, [$tillac_rtems_cpu_cflags])
dnl		AC_SUBST(rtems_cpu_asflags,["$tillac_rtems_cpu_asflags -DASM"])
dnl		AC_SUBST(rtems_cppflags,   [$tillac_rtems_cppflags])
			AC_SUBST(rtems_bsp,        [$enable_rtemsbsp])
			AC_MSG_NOTICE([Setting DOWNEXT to .ralf])
			DOWNEXT=.ralf
			AC_MSG_NOTICE([Setting APPEXEEXT to .exe])
			APPEXEEXT=.exe
			TILLAC_RTEMS_VERSTEST
		fi
	fi
	fi
	if test "${_tillac_rtems_recursing}" = "yes" ; then
		AC_CONFIG_FILES([makefile:makefile.top.in])
		AC_OUTPUT
		exit 0
		false
	fi]dnl
)
	
dnl m4_syscmd is executed when aclocal is run
m4_syscmd([cat - > makefile.top.am <<'EOF_'
AUTOMAKE_OPTIONS=foreign
SUBDIRS=@the_subdirs@
# When making a distribution we only want to 
# recurse into (any) one single BSP subdir.
DIST_SUBDIRS=@the_distsub@

# The dist-hook then removes this extra
# directory level again.
dist-hook:
	cp -frl $(distdir)/$(DIST_SUBDIRS)/* $(distdir)
	rm -fr  $(distdir)/$(DIST_SUBDIRS)
EOF_
])

## Check for a program, similar to AC_CHECK_PROG, but lets
## configure fail if the program is not found
#dnl RTEMS_CHECK_PROG(VARIABLE, PROG-TO-CHECK-FOR, VALUE-IF-FOUND [, VALUE-IF-NOT-FOUND [, PATH [, REJECT]]])
AC_DEFUN([RTEMS_CHECK_PROG],
[
	AC_CHECK_PROG($1,$2,$3,$4,$5,$6)
	AS_IF([test -z "${$1}"],
		[AC_MSG_ERROR([program '$2' not found.])])
])

## Check for a cross tool, similar to AC_CHECK_TOOL, but do not fall back to
## the un-prefixed version of PROG-TO-CHECK-FOR.
## Also - if tool is not found then produce an error.
#dnl RTEMS_CHECK_TOOL(VARIABLE, PROG-TO-CHECK-FOR[, VALUE-IF-NOT-FOUND [, PATH]])
AC_DEFUN([RTEMS_CHECK_TOOL],
[
  AS_IF([test "x$build_alias" != "x$host_alias"],
    [rtems_tool_prefix=${ac_tool_prefix}])
  RTEMS_CHECK_PROG($1, ${rtems_tool_prefix}$2, ${rtems_tool_prefix}$2, $3, $4)
  AC_SUBST($1)
])

# This macro can be provided as a 5th argument
# to AC_CHECK_LIB() so that linking an RTEMS
# application works. Without that, linking would
# fail because the application usually supplies
# rtems_bsdnet_config.
# In order to link, we create a dummy symbol.
AC_DEFUN([TILLAC_RTEMS_CHECK_LIB_ARGS],
	[[-Wl,--defsym,rtems_bsdnet_config=0]]dnl
)

# Check for critical programs we need for building
AC_DEFUN([TILLAC_RTEMS_CHECK_TOOLS],
	[AC_PROG_CC
	 AM_PROG_AS
	 AC_PROG_CXX
	 AC_SUBST([GCC])
	 AC_PROG_CPP
	 AC_CHECK_PROGS([HOSTCC], gcc cc)
	 RTEMS_CHECK_TOOL([AR],ar)
	 RTEMS_CHECK_TOOL([LD],ld)
	 RTEMS_CHECK_TOOL([OBJCOPY],objcopy)
	 RTEMS_CHECK_TOOL([RANLIB],ranlib)
	 AC_PROG_INSTALL
	 AC_CHECK_PROG([INSTALL_IF_CHANGE],[install-if-change],[install-if-change],[${INSTALL}])]dnl
)

# Define 'postlink' commands based on BSP family
#
# NOTE: This is NOT extracted from the RTEMS makefiles but
#       essentially a copy of what rtems-4.9.0 does.
#       It would be too hard to figure this one out ;-(
#
AC_DEFUN([TILLAC_RTEMS_BSP_POSTLINK_CMDS],
	[AC_ARG_VAR([RTEMS_BSP_POSTLINK_CMDS],[Command sequence to convert ELF file into downloadable executable])
	AC_MSG_NOTICE([Setting RTEMS_BSP_POSTLINK_CMDS based on RTEMS_BSP_FAMILY])
	case "$RTEMS_BSP_FAMILY" in
		svgm|beatnik|mvme5500|mvme3100|uC5282|mvme167|mvme162)
# convert ELF -> pure binary
			RTEMS_BSP_POSTLINK_CMDS='$(OBJCOPY) -Obinary $(basename $[@])$(APPEXEEXT) $[@]'
		;;
		motorola_powerpc)
# convert ELF -> special PREP bootloader
			RTEMS_BSP_POSTLINK_CMDS=\
'$(OBJCOPY) -O binary -R .comment -S $(basename $[@])$(APPEXEEXT) rtems ;'\
'gzip -vf9 rtems ; '\
'$(LD) -o $(basename $[@])$(DOWNEXT)  $(RTEMS_BSP_INSTTOP)/lib/bootloader.o '\
'--just-symbols=$(basename $[@])$(APPEXEEXT) '\
'-b binary rtems.gz -T $(RTEMS_BSP_INSTTOP)/lib/ppcboot.lds '\
'-Map $(basename $[@]).map && chmod 755 $(basename $[@])$(DOWNEXT) ; '\
'rm -f rtems.gz'
		;;
# default: empty command
		*)
		;;
	esac
	AC_MSG_NOTICE([RTEMS_BSP_POSTLINK_CMDS: "$RTEMS_BSP_POSTLINK_CMDS"])
	AM_CONDITIONAL([HAVE_BSP_POSTLINK_CMDS], [test ! "$RTEMS_BSP_POSTLINK_CMDS"xx = "xx" ])]dnl
)

# fixup the 'exec-prefix' and 'includedir' options:
#  - if either is given explicitly by the user then do nothing
#  - if user says --enable-std-rtems-installdirs then
#      prefix      -> ${rtems_top} 
#      exec-prefix -> ${prefix}/<cpu>/
#      libdir      -> ${exec-prefix}/<bsp>/lib
#      includedir  -> ${libdir}/include
#
#  - if user says nothing then
#
#      exec-prefix -> ${prefix}/target/ssrlApps/<cpu>/<bsp>/
#      includedir  -> ${exec-prefix}/include
#    
AC_DEFUN([TILLAC_RTEMS_FIXUP_PREFIXES],
[
AC_REQUIRE([TILLAC_RTEMS_OPTIONS])
if TILLAC_RTEMS_OS_IS_RTEMS ; then
if test "${enable_std_rtems_installdirs}" = "yes" ; then
	prefix=${with_rtems_top}
	exec_prefix='${prefix}/${host_cpu}-${host_os}/'
	libdir='${exec_prefix}/'${enable_rtemsbsp}/lib
	if test "$enable_multilib" = "yes" ; then
		includedir='${exec_prefix}/include'
	else
		includedir='${libdir}/include'
	fi
	ac_configure_args="${ac_configure_args} --prefix='${prefix}'"
	ac_configure_args="${ac_configure_args} --exec-prefix='${exec_prefix}'"
	ac_configure_args="${ac_configure_args} --libdir='${libdir}'"
	ac_configure_args="${ac_configure_args} --includedir='${includedir}'"
else
# should be correct also for multilibbed build (rtems_bsp empty)
	if test "${exec_prefix}" = "NONE" ; then
		exec_prefix='${prefix}/target/ssrlApps/${host_cpu}-${host_os}/'${enable_rtemsbsp}/
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
fi
fi]dnl
)


# Automake-1.10's AM_ENABLE_MULTILIB is buggy - it
# does not properly preserve quoting when copying
# ac_configure_args to the 'config.status' it creates.
# I guess one level of quoting is removed when the
# copying happens (by means of a 'here'-document in
# AC_OUTPUT_COMMANDS).
#
# Note that we cannot use a different name since
# automake 'knows' about AM_ENABLE_MULTILIB and
# behaves differently if we would, e.g., name the
# modified macro 'MY_ENABLE_MULTILIB'.
# Hence we hope that we can override automake/aclocal's
# definition.
#
# Below (look at the 'sed' code) we replace all occurrences
# of '$' by '\$' so that 'config.status' again says '$'.
#
# This is important if we want to pass e.g.,
#
#   --exec-prefix='${prefix}/xxx'
#
# correctly to the multisubdir configurations.
#
# AM_ENABLE_MULTILIB([MAKEFILE], [REL-TO-TOP-SRCDIR])
# ---------------------------------------------------
# Add --enable-multilib to configure.
AC_DEFUN([AM_ENABLE_MULTILIB],
[# Default to --enable-multilib
AC_ARG_ENABLE(multilib,
[  --enable-multilib       build many library versions (default)],
[case "$enableval" in
  yes) multilib=yes ;;
  no)  multilib=no ;;
  *)   AC_MSG_ERROR([bad value $enableval for multilib option]) ;;
 esac],
	      [multilib=yes])

# We may get other options which we leave undocumented:
# --with-target-subdir, --with-multisrctop, --with-multisubdir
# See config-ml.in if you want the gory details.

if test "$srcdir" = "."; then
  if test "$with_target_subdir" != "."; then
    multi_basedir="$srcdir/$with_multisrctop../$2"
  else
    multi_basedir="$srcdir/$with_multisrctop$2"
  fi
else
  multi_basedir="$srcdir/$2"
fi
AC_SUBST(multi_basedir)

# Even if the default multilib is not a cross compilation,
# it may be that some of the other multilibs are.
if test $cross_compiling = no && test $multilib = yes \
   && test "x${with_multisubdir}" != x ; then
   cross_compiling=maybe
fi

AC_OUTPUT_COMMANDS([
# Only add multilib support code if we just rebuilt the top-level
# Makefile.
case " $CONFIG_FILES " in
 *" ]m4_default([$1],Makefile)[ "*)
   ac_file=]m4_default([$1],Makefile)[ . ${multi_basedir}/config-ml.in
   ;;
esac],
		   [
srcdir="$srcdir"
host="$host"
target="$target"
with_multisubdir="$with_multisubdir"
with_multisrctop="$with_multisrctop"
with_target_subdir="$with_target_subdir"
ac_configure_args="${multilib_arg} `echo ${ac_configure_args} | sed -e 's/[$]/\\\\$/g'`"
multi_basedir="$multi_basedir"
CONFIG_SHELL=${CONFIG_SHELL-/bin/sh}
CC="$CC"])])dnl

# TILLAM_MULTISUB_INSTALLDIR
#
# tweak 'libdir' so that libraries are
# installed in proper multisubdir.
#
# For use by 'sub-packages', i.e., from
# configure.ac in a subdir of a main
# package. Only the toplevel configure.ac
# should say AM_ENABLE_MULTILIB
#
AC_DEFUN([TILLAM_MULTISUB_INSTALLDIR],
[# Install multilib into proper multisubdir
if test "${with_multisubdir+set}" = "set" ; then
  the_multisubdir="/${with_multisubdir}"
else
  the_multisubdir=
fi
AC_SUBST(libdir,[${libdir}${the_multisubdir}])])dnl


])dnl
