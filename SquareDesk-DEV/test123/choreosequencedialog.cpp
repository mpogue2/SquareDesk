/****************************************************************************
**
** Copyright (C) 2016-2020 Mike Pogue, Dan Lyke
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

#include "choreosequencedialog.h"
#include "ui_choreosequencedialog.h"
#include <QMessageBox>
#include <QKeySequenceEdit>
#include <QTimeEdit>
#include "sessioninfo.h"
#include <algorithm>
#include "mainwindow.h"

// -------------------------------------------------------------------
ChoreoSequenceDialog::ChoreoSequenceDialog(QString sequenceIdentifier, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChoreoSequenceDialog)
{
    Q_UNUSED(sequenceIdentifier)
    mw = reinterpret_cast<MainWindow *>(parent);
}


// ----------------------------------------------------------------
ChoreoSequenceDialog::~ChoreoSequenceDialog()
{
    delete ui;
}

