TEMPLATE = subdirs

SUBDIRS = taglib test123 sdlib

mac {
## what subproject depends on others -- test123 depends on sdlib and taglib
test123.depends = taglib sdlib

# TEMPORARILY TURN OFF THE VERSION CHECK FOR MACOS 11
CONFIG+=sdk_no_version_check
}

# where to find the sub projects -----------------
taglib.subdir = third_party/taglib/taglib
sdlib.subdir  = third_party/sdlib
test123.subdir = test123
