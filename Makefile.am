libexec_PROGRAMS = hangul-input-method

appsdir = $(datadir)/applications
dist_apps_DATA = org.gtk.HangulInputMethod.desktop

org.gtk.HangulInputMethod.service: org.gtk.HangulInputMethod.service.in
	$(AM_V_GEN) sed \
		-e "s|[@]PACKAGE_NAME@|$(PACKAGE_NAME)|g" \
		-e "s|[@]libexecdir@|$(libexecdir)|g" \
		$< > $@

servicedir = $(datadir)/dbus-1/services
service_DATA = org.gtk.HangulInputMethod.service

hangul_input_method_SOURCES = hangul-input-method.c
hangul_input_method_CFLAGS = $(DEPS_CFLAGS)
hangul_input_method_LDADD = $(DEPS_LIBS)

EXTRA_DIST = org.gtk.HangulInputMethod.service.in
CLEANFILES = org.gtk.HangulInputMethod.service

-include $(top_srcdir)/git.mk
