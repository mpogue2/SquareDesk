#!/bin/bash

export DEBFULLNAME="Dan Lyke"
export DEBEMAIL="danlyke@flutterby.com"
export VERSION=0.1
export PACKAGE=squaredesk
export SRCDIR=${PACKAGE}_${VERSION}

rm -r ${SRCDIR} ${SRCDIR}.orig.tar.gz ${PACKAGE}_${VERSION}.orig.tar.xz ${SRCDIR}-1_amd64.build
mkdir -p ${SRCDIR}
cp ../test123/*.{ui,h,cpp,pro,qrc,ico,mm} ${SRCDIR}
cp ./debian/libbass{,_fx,mix}.so ${SRCDIR}
cp ../test123/Makefile ${SRCDIR}
cp -r ../test123/{images,graphics,startupwizardimages} ${SRCDIR}
cp -r debian ${SRCDIR}
pushd ${SRCDIR}


echo "images.path = /usr/share/${PACKAGE}/images" >> test123.pro
echo "images.files = images/*" >> test123.pro
echo "INSTALLS += images" >> test123.pro

echo "startupwizardimages.path = /usr/share/${PACKAGE}/startupwizardimages" >> test123.pro
echo "startupwizardimages.files = startupwizardimages/*" >> test123.pro
echo "INSTALLS += startupwizardimages" >> test123.pro

echo "graphics.path = /usr/share/${PACKAGE}/graphics" >> test123.pro
echo "graphics.files = graphics/*" >> test123.pro
echo "INSTALLS += graphics" >> test123.pro

echo "target.path=/usr/bin" >> test123.pro
echo "INSTALLS += target" >> test123.pro

popd
rm -f ${SRCDIR}.orig.tar.gz
tar czvf ${SRCDIR}.orig.tar.gz ${SRCDIR}
pushd ${SRCDIR}

dh_make --yes -s -p ${SRCDIR} -c gpl2 --createorig
rm -f debian/*.ex debian/*.EX
rm -f debian/README.Debian
rm -f qdebian/README.source

debuild

#debuild -us -uc

