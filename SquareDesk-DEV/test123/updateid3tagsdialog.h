/****************************************************************************
**
** Copyright (C) 2016-2026 Mike Pogue, Dan Lyke
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

#ifndef UPDATEID3TAGSDIALOG_H
#define UPDATEID3TAGSDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QDebug>

namespace Ui
{
class MainWindow;
}

class MainWindow;

namespace Ui
{
class updateID3TagsDialog;
}

class updateID3TagsDialog : public QDialog
{
    Q_OBJECT

public:
    updateID3TagsDialog(QWidget *parent = nullptr); // 0);
    ~updateID3TagsDialog();

    Ui::updateID3TagsDialog *ui;

    double currentBPM;
    double currentTBPM;
    int    newTBPM;
    bool newTBPMValid;

    uint32_t currentLoopStart;
    uint32_t newLoopStart;
    bool newLoopStartValid;

    uint32_t currentLoopLength;
    uint32_t newLoopLength;
    bool newLoopLengthValid;

private slots:
    void on_newTBPMEditBox_textChanged(const QString &arg1);

    void on_newLoopStartEditBox_textChanged(const QString &arg1);

    void on_newLoopLengthEditBox_textChanged(const QString &arg1);

private:
    MainWindow *mw;  // so we can access MainWindow members directly
};

#endif // UPDATEID3TAGSDIALOG_H
