# Probe for the Suggest Clubs review dialog (#1814).
#   Shows the dialog for the suggestions from a read-only copy of SquareDesk.sqlite3, styled with
#   the real Themes.qss, and shows what OK would save.  Never writes to the database.
#   Not part of the SquareDesk build, though the dialog is (Clubs > Suggest Clubs from Play History...).

QT       += core gui widgets sql
CONFIG   += c++17 sdk_no_version_check
CONFIG   -= app_bundle
TARGET    = clubsuggester_probe

DEFINES += THEMES_QSS=\\\"$$PWD/../themes/Themes.qss\\\"

INCLUDEPATH += ..
SOURCES += main.cpp ../suggestclubsdialog.cpp \
           ../clubsuggester.cpp ../clubinfo.cpp ../clubstab.cpp
HEADERS += ../suggestclubsdialog.h \
           ../clubsuggester.h ../clubinfo.h ../clubstab.h ../sessioninfo.h
