TEMPLATE = subdirs

SUBDIRS = \
           sd      \      # relative paths
           sdApp   \
           test123

# where to find the sub projects
sd.subdir = sd
sdApp.subdir = sdApp
test123.subdir = test123

# what subproject depends on others -- test123 and sdApp depend on sd
sdApp.depends = sd
test123.depends = sd
