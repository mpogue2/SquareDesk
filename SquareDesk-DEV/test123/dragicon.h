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

// dragicon.h - Helper functions for creating drag-and-drop icons
#ifndef DRAGICON_H
#define DRAGICON_H

#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

class DragIcon
{
public:
    // Create a drag icon for songs
    // If count <= 1, returns plain music icon
    // If count > 1, adds black rectangle badge with white count number in lower right
    static QPixmap createDragIcon(int count = 1)
    {
        // Load the base music icon
        QPixmap baseIcon(":/graphics/dragicon_music.png");
        if (baseIcon.isNull()) {
            // Fallback: try direct path if resource not found
            baseIcon.load("graphics/dragicon_music.png");
        }

        if (baseIcon.isNull()) {
            // Last resort: create a simple fallback icon
            baseIcon = QPixmap(48, 48);
            baseIcon.fill(Qt::transparent);
            QPainter p(&baseIcon);
            p.setPen(QPen(QColor(70, 130, 180), 2));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(10, 10, 28, 28);
            p.drawLine(32, 15, 32, 38);
        }

        // If only one item, return the base icon
        if (count <= 1) {
            return baseIcon;
        }

        // Create a copy to draw the badge on
        QPixmap iconWithBadge = baseIcon.copy();
        QPainter painter(&iconWithBadge);
        painter.setRenderHint(QPainter::Antialiasing);

        int width = iconWithBadge.width();
        int height = iconWithBadge.height();

        // Prepare count text
        QString countText = QString::number(count);

        // Set up font for badge
        QFont font = painter.font();
        int fontSize = qMax(12, int(height * 0.35));
        font.setPixelSize(fontSize);
        font.setBold(true);
        painter.setFont(font);

        // Calculate badge dimensions based on text size
        QFontMetrics fm(font);
        QRect textBounds = fm.boundingRect(countText);
        int textWidth = textBounds.width();
        int textHeight = textBounds.height();

        // Add padding
        int paddingX = qMax(6, int(textWidth * 0.3));
        int paddingY = qMax(4, int(textHeight * 0.2));
        int badgeWidth = textWidth + paddingX * 2;
        int badgeHeight = textHeight + paddingY * 2;

        // Position in lower right corner (overlapping)
        int badgeX = width - badgeWidth;
        int badgeY = height - badgeHeight;

        // Draw black filled rectangle
        painter.fillRect(badgeX, badgeY, badgeWidth, badgeHeight, Qt::black);

        // Draw white text centered in badge
        painter.setPen(Qt::white);
        int textX = badgeX + (badgeWidth - textWidth) / 2;
        int textY = badgeY + (badgeHeight - textHeight) / 2 + fm.ascent() - 2;
        painter.drawText(textX, textY, countText);

        return iconWithBadge;
    }
};

#endif // DRAGICON_H
