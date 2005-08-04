## Makefile.am for klinpopup

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = klinpopup

# set the include path for X, qt and KDE
INCLUDES         = $(all_includes)

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO


# the library search path.
klinpopup_LDFLAGS = $(KDE_RPATH) $(all_libraries)

# the libraries to link against.
klinpopup_LDADD   = $(LIB_KFILE) -lsmbclient

# which sources should be compiled for klinpopup
klinpopup_SOURCES = settings.kcfgc main.cpp klinpopup.cpp klinpopupview.cpp \
	klinpopupview_base.ui systemtray.cpp makepopup.cpp prefs.ui

xdg_apps_DATA = klinpopup.desktop
kde_kcfg_DATA = klinpopup.kcfg
bin_SCRIPTS = receivepopup.sh

# these are the headers for your project
noinst_HEADERS = klinpopup.h klinpopupview.h systemtray.h makepopup.h \
		popupmessage.h prefs.h inotify.h inotify-syscalls.h

messages: rc.cpp
	$(EXTRACTRC) *.ui *.rc >> rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/klinpopup.pot

# this is where the kdelnk file will go
kdelnkdir   = $(kde_appsdir)/Internet
kdelnk_DATA = klinpopup.desktop

# this is where the XML-GUI resource file goes
rcdir = $(kde_datadir)/klinpopup
rc_DATA = klinpopupui.rc

SUBDIRS = icons
