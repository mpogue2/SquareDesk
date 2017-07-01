TEMPLATE = subdirs

SUBDIRS = taglib test123 \
    html-tidy

mac {
# sd and sdApp are MAC OS X ONLY for now.
#   sd needs to be compiled outside of Qt, I think,
#   or under cygwin.
SUBDIRS += sd sdApp
sd.subdir = sd
sdApp.subdir = sdApp

# what subproject depends on others -- test123 and sdApp depend on sd, test123 also depends on taglib
sdApp.depends = sd
test123.depends = sd taglib
}

# where to find the sub projects
taglib.subdir = taglib/taglib
test123.subdir = test123

win32 {
SUBDIRS += sd
sd.subdir = sd
test123.depends = sd taglib
}
