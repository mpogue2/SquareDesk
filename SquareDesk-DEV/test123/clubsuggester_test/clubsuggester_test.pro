# Standalone test harness for suggesting Clubs from play history (#1814).
#   Suggests clubs from a read-only copy of SquareDesk.sqlite3 and compares them with its real clubs.
#   Not part of the SquareDesk build, though clubsuggester.cpp is (Clubs > Suggest Clubs from Play History...).

QT       += core sql
QT       -= gui
CONFIG   += console c++17
CONFIG   -= app_bundle
TARGET    = clubsuggester_test

INCLUDEPATH += ..
SOURCES += main.cpp ../clubsuggester.cpp ../clubinfo.cpp
HEADERS += ../clubsuggester.h ../clubinfo.h ../sessioninfo.h
