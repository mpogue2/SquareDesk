/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
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

#ifndef NEWDANCEDIALOG_H
#define NEWDANCEDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QValidator>

namespace Ui
{
    class newDanceDialog;
}

class newDanceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit newDanceDialog(QWidget *parent = 0);
    ~newDanceDialog();
    void populateFields();
    QString getNewDanceName();

private:
    Ui::newDanceDialog *ui;
};

#endif // NEWDANCEDIALOG_H
