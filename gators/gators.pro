TEMPLATE = app
DEV_ROOT = $$(SIMLABS_REPO_DIR)

include($$DEV_ROOT/src/predefines.pri)

CONFIG -= qt
CONFIG += console

LIBS += -llogger -lalloc 

PRECOMPILED_HEADER = stdafx.h

SOURCES += $$get_all_sources()
SOURCES -= stdafx.h.cpp

HEADERS += $$get_all_headers()

include($$DEV_ROOT/src/postdefines.pri)

