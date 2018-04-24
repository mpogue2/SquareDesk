Building on Linux, courtesy of Tim Schares:

## Libraries

You can't do this step utill after you have cloned the repo,
 but the instructions are here to keep the other steps more linear.

From the SquareDesk-DEV/ directory in the repo: (you should already be
 there if you got here from the main instructions)
`sudo cp deb_packaging/debian/libbass*.so /usr/local/lib`

You can also use LD_LIBRARY_PATH, but this is simplest.

NOTE: some distros (like Ubuntu) don't have /usr/local/lib in
 their default LD_LIBRARY_PATH. For Ubuntu in particular, you can
 add a custom `.conf` file to `/etc/ld.so.conf.d/` e.g.:

 `echo /usr/local/lib | sudo tee /etc/ld.so.conf.d/squaredesk.conf`

## Prerequisites

First, install the additional packages we're now using (one at a time)

`sudo apt-get install build-essential git g++ qt5-default libqt5svg5-dev libqt5webkit5-dev libtag1-dev cmake libsqlite3-dev sqlite3 xsltproc qtbase5-dev`

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
# Copy the shared libraries into the right place
# See the "Libraries" section above ^^^^^^
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
