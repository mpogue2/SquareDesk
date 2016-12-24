TEMPLATE = subdirs

SUBDIRS = \
           sd      \      # relative paths
           sdApp   \
           taglib  \
           test123

# where to find the sub projects
sd.subdir = sd
sdApp.subdir = sdApp
taglib.subdir = taglib/taglib
test123.subdir = test123

# what subproject depends on others -- test123 and sdApp depend on sd, test123 also depends on taglib
sdApp.depends = sd
test123.depends = sd taglib
