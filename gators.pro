TEMPLATE = subdirs
DEV_ROOT = $$(SIMLABS_REPO_DIR)

include($$DEV_ROOT/src/predefines.pri)


CONFIG += ordered

SUBDIRS = $$DEV_ROOT/src/_Include    \
          $$DEV_ROOT/src/core/logger    \
          $$DEV_ROOT/src/core/alloc    \
          gators

include($$DEV_ROOT/src/postdefines.pri)
