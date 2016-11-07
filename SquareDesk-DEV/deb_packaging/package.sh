#!/bin/bash

export VERSION=0.1
export PACKAGE=squaredesk
export SRCDIR=${PACKAGE}_${VERSION}
rm -r ${SRCDIR} ${SRCDIR}.orig.tar.gz ${SRCDIR}-1_amd64.build
mkdir -p ${SRCDIR}
cp ../test123/*.{ui,h,cpp,pro,qrc,ico} ${SRCDIR}
cp ../test123/Makefile ${SRCDIR}
tar czvf ${SRCDIR}.orig.tar.gz ${SRCDIR}
cp -r debian ${SRCDIR}
pushd ${SRCDIR}
debuild -us -uc

