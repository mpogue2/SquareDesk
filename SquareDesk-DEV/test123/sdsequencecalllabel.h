/****************************************************************************
**
** Copyright (C) 2016-2024 Mike Pogue, Dan Lyke
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

#ifndef SDSEQUENCECALLLABEL_H_INCLUDED
#define SDSEQUENCECALLLABEL_H_INCLUDED

#include <QLabel>
class MainWindow;

class SDSequenceCallLabel : public QLabel {
private:
    MainWindow *mw;

    QString theText;
    QString theHtml;

    QPixmap thePixmap;

    QString theFormation;

public:
    SDSequenceCallLabel(MainWindow *mw) : QLabel(), mw(mw) { }
    void mouseDoubleClickEvent(QMouseEvent *) override;

//    void setSelected(bool b);  // these are inherited from QLabel
//    bool isSelected();

    void setData(int role, const QVariant &value);
    QVariant data(int role);

    // replacements for data/setData ------
    void setFormation(QString formation);
    QString formation();

    void setPixmap(QPixmap pixmap);
    QPixmap pixmap();

    // -----------
    void setPlainText(QString s);  // plain text only (HTML tags removed)
    QString plainText();

    void setHtml(QString s);  // rich text
    QString html();
};

#endif /* ifndef SDSEQUENCECALLLABEL_H_INCLUDED */
