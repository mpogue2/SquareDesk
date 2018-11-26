TEMPLATE = subdirs

SUBDIRS = html-tidy taglib test123 sdlib #\
    #mp3gain

mac {
# quazip is Mac OS X only right now, for downloading and unpacking lyrics files...
#SUBDIRS += sd sdApp quazip
SUBDIRS += quazip # mp3gain
#sd.subdir = sd
#sdApp.subdir = sdApp

## what subproject depends on others -- test123 and sdApp depend on sd, test123 also depends on taglib and html-tidy
#sdApp.depends = sd
#test123.depends = sd taglib html-tidy sdlib
test123.depends = taglib html-tidy sdlib
}

# WIN32: where to find the sub projects -----------------
taglib.subdir = taglib/taglib
test123.subdir = test123
html-tidy.subdir = html-tidy

win32 {
#SUBDIRS += sd
#sd.subdir = sd
#test123.depends = sd taglib html-tidy
test123.depends = taglib html-tidy
}
