# Standalone test harness for the cuesheet converter (#1789).
#   Converts a folder of cuesheets and writes an index.html for side-by-side review.
#   Not part of the SquareDesk build.

QT       += core gui
CONFIG   += console c++17
CONFIG   -= app_bundle
TARGET    = cuesheetconverter_test

INCLUDEPATH += ..
SOURCES += main.cpp ../cuesheetconverter.cpp
HEADERS += ../cuesheetconverter.h
