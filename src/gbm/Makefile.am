pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = main/gbm.pc

AM_CFLAGS = \
	-D_OS_UNIX=1 \
	-DMODULEDIR='"$(libdir)/gbm"' \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/loader \
	-I$(top_srcdir)/src/gbm/main \
	$(DLOPEN_CFLAGS) \
	$(DEFINES) \
	$(VISIBILITY_CFLAGS)

lib_LTLIBRARIES = libgbm.la
include_HEADERS = main/gbm.h

libgbm_la_SOURCES = \
	main/backend.c \
	main/backend.h \
	main/common_drm.h \
	main/gbm.c \
	main/gbm.h \
	main/gbmint.h

libgbm_la_LDFLAGS = \
	-no-undefined \
	-version-info 1:0 \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

libgbm_la_LIBADD = \
	$(top_builddir)/src/loader/libloader.la \
	$(DLOPEN_LIBS)

if HAVE_PLATFORM_WAYLAND
AM_CPPFLAGS = -DHAVE_WAYLAND_PLATFORM
AM_CFLAGS += $(WAYLAND_CFLAGS)
libgbm_la_LIBADD += $(top_builddir)/src/egl/wayland/wayland-drm/libwayland-drm.la $(WAYLAND_LIBS)
endif

if HAVE_DRI2
libgbm_la_SOURCES += \
	backends/dri/gbm_dri.c \
	backends/dri/gbm_driint.h

AM_CFLAGS += \
	-DDEFAULT_DRIVER_DIR='"$(DRI_DRIVER_SEARCH_DIR)"' \
	$(LIBDRM_CFLAGS)

libgbm_la_LIBADD += \
	$(LIBDRM_LIBS)
endif

TESTS = gbm-symbols-check
EXTRA_DIST = gbm-symbols-check

include $(top_srcdir)/install-lib-links.mk
