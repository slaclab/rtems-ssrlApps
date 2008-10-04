AUTOMAKE_OPTIONS=foreign

SUBDIRS=. @enable_subdirs@

all-local:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL="$(INSTALL_IF_CHANGE)" prefix="$(abs_top_builddir)/data" exec_prefix='$$(prefix)' includedir='$$(prefix)/include' install-data
	$(MAKE) $(AM_MAKEFLAGS) INSTALL="$(INSTALL_IF_CHANGE)" prefix="$(abs_top_builddir)/data" exec_prefix='$$(prefix)' includedir='$$(prefix)/include' install-exec

clean-local:
	$(RM) -r data
