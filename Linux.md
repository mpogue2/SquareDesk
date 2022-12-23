WARNING: This document is incomplete, I still need to wipe a machine
and verify that from a clean install of some distribution, we can get
a working build.

Building SquareDesk on a Debian-style Linux (eg: Ubuntu). Initial
revision by Tim Schares, revisions for Qt6 and clang-built processing
by Dan Lyke

## Prerequisites

I don't think this is a complete list of packages. I tried to stay
current, but lost track:

`sudo apt install build-essential git g++ qt6-base-dev qt6-multimedia-dev libtag1-dev cmake libsqlite3-dev sqlite3 xsltproc libqt6svg6-dev qt6-webengine-dev ninja-build clang`

Get the libraries we build from source:

```
mkdir -p ~/code/vendor
pushd ~/code/vendor
git clone git@github.com:kfrlib/kfr.git
git clone git@github.com:breakfastquay/minibpm.git
git clone https://codeberg.org/soundtouch/soundtouch.git
popd
```

Build them:

```mkdir -p $HOME/local
pushd kfr
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=$HOME/local -GNinja -DENABLE_CAPI_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ ..
ninja kfr_capi
ninja install
popd
pushd soundtouch
cmake -DCMAKE_INSTALL_PREFIX:PATH=$HOME/local && make && make all && make install
popd
```

# Main Instructions
```
# this'll reset you to /home/<your_username>
cd
# make sure we have a 'code' directory
mkdir -p code
# change into it:
cd code
# checkout the source code
git clone https://github.com/mpogue2/SquareDesk.git
# Go to the working directory
cd SquareDesk/SquareDesk-DEV
# create the build environment from the specification files
qmake SquareDesk.pro
# do the build
make
cd ~/code/SquareDesk/SquareDesk-DEV/sdlib
qmake
make
~/code/SquareDesk/SquareDesk-DEV/test123
make && ./SquareDesk
```

And SquareDesk will run


## Creating a desktop link in xfce (lxde?)

SquareDesk needs to run from the directory it's built in in order to
find the sd_calls.dat file.

One way to do this is to create a "SquareDesk.sh" file that
contains:

```
#!/bin/bash
cd ~/code/SquareDesk/SquareDesk-DEV/test123
./SquareDesk
```

*Make sure that file is executable (from the command-line,

`chmod +x SquareDesk.sh`

Then make your link to that, and everything should be groovy, and we'll
sort out the rest as we get closer to packaging.

(You could even create this file in ~/Desktop and then it'd just be
sitting on your desktop and you wouldn't need to link it...)

## Creating a desktop link in GNOME3

Got it!  In ubuntu (GNOME3), you can create an executable shell, but it has the extension of ".desktop" instead of ".sh", and then make it executable.  Here is the script I use:

```
[Desktop Entry]
Encoding=UTF-8
Name=SquareDesk
Comment=Launch SquareDesk
Exec=gnome-terminal -e /home/tj/code/SquareDesk/SquareDesk-DEV/test123/SquareDesk
Icon=/home/tj/code/SquareDesk/SquareDesk-DEV/test123/desk1d.icns
Type=Application
Name[en_US]=New SqDesk
```

When I double click on the icon, it opens the terminal window, and then runs the program.
