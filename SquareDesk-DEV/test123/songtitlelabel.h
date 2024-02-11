/****************************************************************************
**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

#ifndef SONGTITLELABEL_H_INCLUDED
#define SONGTITLELABEL_H_INCLUDED

#include <QLabel>
#include "mainwindow.h"
#include "mytablewidget.h"

//class MainWindow;

class SongTitleLabel : public QLabel {
private:
    MainWindow *mw;
public:
    SongTitleLabel(MainWindow *mw) : QLabel(), mw(mw) {}
    void mouseDoubleClickEvent(QMouseEvent *) override;
    QString textColor;  // saved so that we can restore it when not selected
};

#ifdef DARKMODE
class darkSongTitleLabel : public QLabel {
private:
    MainWindow *mw;
public:
    darkSongTitleLabel(MainWindow *mw) : QLabel(), mw(mw) {}
    void mouseDoubleClickEvent(QMouseEvent *) override;
    QString textColor;  // saved so that we can restore it when not selected
};

// when a MyTableWidget is in a palette slot, we need different handling for double-clicking
class darkPaletteSongTitleLabel : public QLabel {
private:
    MainWindow *mw;
    // MyTableWidget *mtw;
public:
    darkPaletteSongTitleLabel(MainWindow *mw) : QLabel(), mw(mw) {}
    // darkPaletteSongTitleLabel(int slot) : QLabel(), slotNumber(slot) {}
    void mouseDoubleClickEvent(QMouseEvent *) override;
//    QString textColor;  // saved so that we can restore it when not selected
};
#endif

#endif /* ifndef SONGTITLELABEL_H_INCLUDED */
