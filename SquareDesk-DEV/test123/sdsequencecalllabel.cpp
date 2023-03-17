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

#include "mainwindow.h"
#include "sdsequencecalllabel.h"

void SDSequenceCallLabel::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
#ifndef NO_TIMING_INFO
    mw->sdSequenceCallLabelDoubleClicked(e);
#endif
}

// *********************
// INCOMPLETE CODE:  this is not going to work, because setSelected is not a member of QLabel
//   we really need to use a delegate for this, so I am abandoning this code.
//   I'm leaving it here to remind myself later that using setCellWidget() is not going to
//   work, because it appears that we cannot implement setSelected on a QTableWidget cell that
//   has a QLabel in it.  I think both Dan and I tried to do something like this.

// inherited from QLabel -----
//void setSelected(bool b) {
//}
//bool isSelected() {
//}

void SDSequenceCallLabel::setData(int role, const QVariant &value) {
    qDebug() << "SDSequenceCallLabel::setData" << role << value;
}
QVariant SDSequenceCallLabel::data(int role) {
    qDebug() << "SDSequenceCallLabel::data" << role;
    return QVariant();
}

// replacements for data/setData ------
void SDSequenceCallLabel::setFormation(QString f) {
    theFormation = f;
}
QString SDSequenceCallLabel::formation() {
    return theFormation;
}

void SDSequenceCallLabel::setPixmap(QPixmap pixmap) {
    thePixmap = pixmap;
}
QPixmap SDSequenceCallLabel::pixmap() {
    return thePixmap;
}

// -----------
// plain text only (HTML tags removed)
void SDSequenceCallLabel::setPlainText(QString s) {
    theText = s;    // cache the text
    theHtml = s;    // rich text version is the same as plain text version
    setText(s);     // set the actual text on the Qlabel
}
QString SDSequenceCallLabel::plainText() {
    return theText;
}

// rich text version
void SDSequenceCallLabel::setHtml(QString s) {
    theHtml = s;
    static QRegularExpression htmlTags("<.*>", QRegularExpression::InvertedGreedinessOption);
    theText = s.replace(htmlTags, ""); // deletes all HTML tags
    setText(s);     // set the actual text to be the RICH TEXT version
}
QString SDSequenceCallLabel::html() {
    return theHtml;
}
