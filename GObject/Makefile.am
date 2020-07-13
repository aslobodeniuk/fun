bin_PROGRAMS = my-plugin-system-inspect
my_plugin_system_inspect_SOURCES = my-plugin-system-inspect.c
my_plugin_system_inspect_CFLAGS = $(GOBJ_CFLAGS)
my_plugin_system_inspect_LDADD = $(GOBJ_LIBS) $(GMODULE_LIBS)


lib_LTLIBRARIES = \
libGlutWindow.la \
libGLcube.la

libGlutWindow_la_DEPENDENCIES = $(top_builddir)/base/libbasewindow.la
libGlutWindow_la_SOURCES = GlutWindow.c
libGlutWindow_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS)
libGlutWindow_la_LIBADD = $(GOBJ_LIBS) -lglut -lGL -L$(top_builddir)/base/.libs/ -lbasewindow
libGlutWindow_la_LDFLAGS = $(PLUGIN_LDFLAGS)

libGLcube_la_DEPENDENCIES = $(top_builddir)/base/libbasedrawable.la
libGLcube_la_SOURCES = GLcube.c
libGLcube_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS)
libGLcube_la_LIBADD = $(GOBJ_LIBS) -lGL -L$(top_builddir)/base/.libs/ -lbasedrawable
libGLcube_la_LDFLAGS = $(PLUGIN_LDFLAGS)


# headers we need but don't want installed
noinst_HEADERS = my-plugin-system.h
EXTRA_DIST = autogen.sh

SUBDIRS=base
