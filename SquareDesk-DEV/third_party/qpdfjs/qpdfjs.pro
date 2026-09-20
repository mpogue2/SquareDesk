#-------------------------------------------------
#
# Project created by QtCreator 2017-04-09T11:08:13
#
#-------------------------------------------------

QT += core gui widgets
QT += webenginewidgets

TARGET = qpdfjs
TEMPLATE = app

SOURCES +=\
        src/qpdfjs.cpp \
        src/qpdfjswindow.cpp \
        src/communicator.cpp

HEADERS  += \
        src/qpdfjswindow.h \
        src/communicator.h

DISTFILES += \
        minified/web/pdf.viewer.js \
        minified/web/qwebchannel.js \
        minified/web/viewer.html \
        minified/web/viewer.css #\
    #../pdf.js/web/viewer.js \
    #../pdf.js/web/viewer.css \
    #../pdf.js/web/viewer.html

copydata1.commands = $(MKDIR) $$OUT_PWD/qpdfjs.app/Contents/MacOS/minified
copydata2.commands = $(COPY_DIR) $$PWD/minified/web   $$OUT_PWD/qpdfjs.app/Contents/MacOS/minified
copydata3.commands = $(COPY_DIR) $$PWD/minified/build $$OUT_PWD/qpdfjs.app/Contents/MacOS/minified
copydata4.commands = $(RM) $$OUT_PWD/qpdfjs.app/Contents/MacOS/minified/web/compressed.*.pdf

first.depends = $(first) copydata1 copydata2 copydata3 copydata4
export(first.depends)
export(copydata1.commands)
export(copydata2.commands)
export(copydata3.commands)
export(copydata4.commands)
QMAKE_EXTRA_TARGETS += first copydata1 copydata2 copydata3 copydata4

CONFIG += debug_and_release
!debug_and_release|build_pass {
        CONFIG(debug, debug|release) {
                        TARGET = qpdfjs
                        #DESTDIR = $$PWD
         } else {
                        TARGET = qpdfjs
                        #DESTDIR = $$PWD
         }
}
