TEMPLATE = subdirs

#SUBDIRS = html-tidy taglib test123 sdlib #\
SUBDIRS = taglib test123 sdlib

mac {
# quazip is Mac OS X only right now, for downloading and unpacking lyrics files...

# USE THIS LINE FOR X86_64 MAC BUILDS ONLY
#SUBDIRS += quazip

# M1MAC: USE THIS LINE FOR M1 Silicon BUILDS ONLY
SUBDIRS += # quazip

## what subproject depends on others -- test123 and sdApp depend on sd, test123 also depends on taglib and html-tidy
test123.depends = taglib sdlib

# TEMPORARILY TURN OFF THE VERSION CHECK FOR MACOS 11
CONFIG+=sdk_no_version_check
}

# WIN32: where to find the sub projects -----------------
taglib.subdir = taglib/taglib
test123.subdir = test123
#html-tidy.subdir = html-tidy

win32 {
#SUBDIRS += sd
#sd.subdir = sd
#test123.depends = sd taglib html-tidy
#test123.depends = taglib html-tidy
test123.depends = taglib
}
