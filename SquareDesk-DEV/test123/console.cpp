/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

/****************************************************************************
**
** Copyright (C) 2016-2021 Mike Pogue, Dan Lyke
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

#include "console.h"

#include <QScrollBar>
#include <QtCore/QDebug>
#include <QApplication>
#include <QRegularExpression>

Console::Console(QWidget *parent)
    : QPlainTextEdit(parent)
    , localEchoEnabled(false)
{
    document()->setMaximumBlockCount(100);
    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::Text, Qt::black);
    setPalette(p);
#if defined(Q_OS_MAC)
    // temporarily commented out for Win32 because of runtime errors
    QFont theFont("courier");
    this->setFont(theFont);
#endif
}

void Console::putData(const QByteArray &data)
{
    QByteArray data2 = data; // mutable
    QString data2s(data);

//    qDebug() << "console data:" << data2 << "length:" << data.length();

    if (data2s.contains('\x07')) {
            QApplication::beep();
//            qDebug() << "Console::putData -- BEEP";
            data2s.replace('\x07',"");
//            qDebug() << "post-beep console data:" << data2 << "length:" << data.length();
    }

    bool done = false;
    while (!done) {
        int beforeLength = data2s.length();
        data2s.replace(QRegularExpression(".\b \b"),""); // if coming in as bulk text, delete one char
        int afterLength = data2s.length();
        done = (beforeLength == afterLength);
    }
//    if (data2s.length()==3 && data2s[0]=='\b' && data2s[1]==' ' && data2s[2] == '\b') {
    if (data2s == "\b \b") {
        // backspace: get rid of one character on the screen (the hard way)
        textCursor().deletePreviousChar();  // the easy way
    } else {
        // normal text
        insertPlainText(QString(data2s));
    }

    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());
}

void Console::setLocalEchoEnabled(bool set)
{
    localEchoEnabled = set;
}

void Console::keyPressEvent(QKeyEvent *e)
{
//    qDebug() << "Console:keyPressEvent:" << e << e->key() << Qt::Key_Escape;
    switch (e->key()) {
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
        break;
    default:
        if (localEchoEnabled) {
            QPlainTextEdit::keyPressEvent(e);
        }
        if (e->text() == "\u001B") {
//            qDebug() << "ESC detected.";
//            qDebug() << "ESC detected:" << e->key() << e->text() << e->text().toLocal8Bit();

#if defined(Q_OS_MAC)
            // map ESC to Ctrl-U (delete entire line), MAC OS ONLY
            emit getData(QByteArray("\025")); // Ctrl-U
#endif
#if defined(Q_OS_WIN32) | defined(Q_OS_LINUX)
            emit getData(QByteArray("\u001B")); // ESC is passed thru to sd, translated there to ClearToEOL
#endif

        } else {
//            qDebug() << "Normal key detected:" << e->key() << e->text() << e->text().toLocal8Bit();
            emit getData(e->text().toLocal8Bit());
        }
    }
}

void Console::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    setFocus();
}

void Console::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
}

void Console::contextMenuEvent(QContextMenuEvent *e)
{
    Q_UNUSED(e)
}

QString Console::getAllText() {
    return this->document()->toPlainText();
}
