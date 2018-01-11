#!/bin/bash

rm -rf SquareDeskPlayer
mkdir -p SquareDeskPlayer/usr/bin
mkdir -p SquareDeskPlayer/usr/lib
mkdir -p SquareDeskPlayer/usr/share/SquareDeskPlayer
mkdir -p SquareDeskPlayer/DEBIAN

echo 'Package: squaredesk' > SquareDeskPlayer/DEBIAN/control
echo 'Version: 0.1' >> SquareDeskPlayer/DEBIAN/control
echo 'Maintainer: Dan Lyke <danlyke@flutterby.com>' >> SquareDeskPlayer/DEBIAN/control
echo 'Architecture: all' >> SquareDeskPlayer/DEBIAN/control
echo 'Description: music player and choreography management for square dance callers' >> SquareDeskPlayer/DEBIAN/control

cp foo/libbass*.so SquareDeskPlayer/usr/lib
pushd ..
qmake
make
popd
cp ../test123/SquareDeskPlayer SquareDeskPlayer/usr/bin

pushd ../sdlib
./mkcalls
popd
cp ../sdlib/sd_calls.dat SquareDeskPlayer/usr/share/SquareDeskPlayer
cp ../sdlib/sd_calls.dat SquareDeskPlayer/usr/share/SquareDeskPlayer
cp ../test123/{cuesheet2.css,patter.template.html,lyrics.template.html} \
   SquareDeskPlayer/usr/share/SquareDeskPlayer


cp -r ../test123/{soundfx,images,graphics,startupwizardimages} SquareDeskPlayer/usr/share/SquareDeskPlayer

dpkg-deb --build SquareDeskPlayer


