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

#include "addcommentdialog.h"
#include "ui_addcommentdialog.h"
#include "QFileDialog"
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QValidator>


AddCommentDialog::AddCommentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddCommentDialog)
{
    ui->setupUi(this);
}


// ----------------------------------------------------------------
AddCommentDialog::~AddCommentDialog()
{
    delete ui;
}

void AddCommentDialog::populateFields(QString call) {
//    qDebug() << "populateFields: " << call;
    ui->existingCall->setText(call + " (");
    ui->theComment->setPlaceholderText("type new comment here");
    ui->theComment->setFocus();
}

QString AddCommentDialog::getComment() {
    QString newComment = ui->theComment->text();
//    qDebug() << "getComment(): " << newComment;
    return newComment;
}
