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

#ifndef MYTEXTEDIT_H
#define MYTEXTEDIT_H

#include <QTextEdit>
#include <QMimeData>
#include <QImage>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>
#include <QImageReader>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QKeyEvent>

class myTextEdit : public QTextEdit {

public:
    explicit myTextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {}
    void smartPaste();  // Public method for Smart Paste (GitHub issue #1552)

protected:
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    // Smart Paste helpers for GitHub issue #1552
    QString performSmartPaste(const QString &clipboardText);
    int findNearestFigureNumber();
    QString transformHeadsSides(const QString &text, int figureNumber, bool isHead);
    bool firstLineContainsHeadOrSides(const QString &text, bool &isHead);
};

#endif // MYTEXTEDIT_H
