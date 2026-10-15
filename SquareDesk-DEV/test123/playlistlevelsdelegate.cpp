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


#include "playlistlevelsdelegate.h"

#include <QApplication>
#include <QPainter>

PlaylistLevelsDelegate::PlaylistLevelsDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void PlaylistLevelsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QString current = index.data(PLAYLIST_CURRENTLEVEL_ROLE).toString();
    QColor currentColor = index.data(PLAYLIST_CURRENTLEVEL_COLOR_ROLE).value<QColor>();
    int where = current.isEmpty() ? -1 : opt.text.indexOf(current);

    const QWidget *widget = opt.widget;
    QStyle *style = (widget != nullptr) ? widget->style() : QApplication::style();

    // Same text margins that QCommonStyle uses when it draws an item's text itself
    int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, widget) + 1;
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget)
                         .adjusted(textMargin, 0, -textMargin, 0);
    QFontMetrics fm(opt.font);
    int textWidth = fm.horizontalAdvance(opt.text);

    // Nothing to highlight, or it would not fit without eliding: paint it the normal way
    if (where < 0 || !currentColor.isValid() || textWidth > textRect.width()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // Background, selection and focus: everything except the text
    QString text = opt.text;
    opt.text.clear();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    QPalette::ColorGroup cg = !(opt.state & QStyle::State_Enabled) ? QPalette::Disabled :
                              (opt.state & QStyle::State_Active) ? QPalette::Normal : QPalette::Inactive;
    QColor normalColor = opt.palette.color(cg, (opt.state & QStyle::State_Selected) ? QPalette::HighlightedText
                                                                                     : QPalette::Text);

    // Draw it in three pieces (before, the current letter, after), centered as a whole
    painter->save();
    painter->setFont(opt.font);
    int x = textRect.left() + (textRect.width() - textWidth) / 2;
    const QString pieces[3] = { text.left(where), text.mid(where, current.length()), text.mid(where + current.length()) };
    for (int i = 0; i < 3; i++) {
        if (pieces[i].isEmpty()) {
            continue;
        }
        painter->setPen(i == 1 ? currentColor : normalColor);
        int w = fm.horizontalAdvance(pieces[i]);
        painter->drawText(QRect(x, textRect.top(), w, textRect.height()), Qt::AlignLeft | Qt::AlignVCenter, pieces[i]);
        x += w;
    }
    painter->restore();
}
