/****************************************************************************
**
** Copyright (C) 2019 Mike Pogue, Dan Lyke
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

#ifndef CHOREOSEQUENCEDIALOG_H
#define CHOREOSEQUENCEDIALOG_H

#include <QDialog>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QValidator>

#include "common_enums.h"
#include "default_colors.h"
#include "keybindings.h"
#include <QPushButton>

class SessionInfo;

namespace Ui
{
class ChoreoSequenceDialog;
}



class ChoreoSequenceDialog : public QDialog
{
    Q_OBJECT

public:
    ChoreoSequenceDialog(QString sequenceIdentifier, QWidget *parent = 0);
    ~ChoreoSequenceDialog();
private slots:
//    void on_chooseMusicPathButton_clicked();


private:
    Ui::ChoreoSequenceDialog *ui;
    MainWindow *mw;  // so we can play soundFX from within the dialog
};


#endif // CHOREOSEQUENCEDIALOG_H
