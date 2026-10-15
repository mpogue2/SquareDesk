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

#include "playlistprinter.h"

#include <QFontMetricsF>
#include <QPagedPaintDevice>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>

// ============================================================================================================
// Playlist printing (issue #1804)
//
// Markers print as tinted bands with their tip number in a circle, songs print indented beneath them
//   with their tags as pills, and each tip is kept on one page when it fits.  All the layout is done in
//   points and converted to device pixels by P(), so a printer and a 72 dpi PDF writer come out the same.

QString cleanPlaylistMarkerTitle(const QString &title) {
    static QRegularExpression separatorRun("[=\\-_+]{3,}");
    QStringList parts;
    for (QString part : title.split(" - ")) {
        part.replace(separatorRun, "");
        part = part.trimmed();
        if (!part.isEmpty()) {
            parts << part;
        }
    }
    return parts.isEmpty() ? title.trimmed() : parts.join(QString("  —  "));
}

// The background colors the SD tab uses for calls at each level (mainwindow_sd.cpp), picked by the
//   marker's first word.  SSD has become the new Mainstream, so it gets the Mainstream color.
static QColor markerBandColor(const QString &markerTitle) {
    QString level = markerTitle.trimmed().section(' ', 0, 0).toUpper();
    if (level == "SSD" || level == "MS" || level == "MAINSTREAM" || level == "BASIC") {
        return QColor("#E0E0FF");
    }
    if (level == "PLUS") {
        return QColor("#BFFFC0");
    }
    if (level == "A1" || level == "A2" || level.startsWith("ADV")) {
        return QColor("#FFF0C0");
    }
    static QRegularExpression challengeLevel("^C[1-4]$|^CHALL");
    if (challengeLevel.match(level).hasMatch()) {
        return QColor("#FEE0E0");
    }
    return QColor("#E8E8E8");  // EXTRA, BREAK, etc.
}

// A darker, more saturated version of the band's hue, for the accent bar, tip circle and marker title
static QColor markerInkColor(const QColor &band) {
    if (band.hslSaturation() < 20) {
        return QColor("#606060");  // the gray band
    }
    return QColor::fromHsl(band.hslHue(), 160, 85);
}

// "2" --> "+2", "0" stays "0"
static QString signedPitch(const QString &pitch) {
    bool ok = false;
    int p = pitch.toInt(&ok);
    return (ok && p > 0) ? "+" + pitch.trimmed() : pitch.trimmed();
}

namespace {
// Sizes in points
constexpr qreal kHeaderH       = 54;   // title block on page 1, down to the heavy rule
constexpr qreal kContHeaderH   = 26;   // shorter title block on later pages
constexpr qreal kHeadingsH     = 22;   // TIP / TITLE / LEVELS / PITCH / TEMPO
constexpr qreal kFooterH       = 24;
constexpr qreal kMarkerH       = 22;
constexpr qreal kMarkerGap     = 6;    // space above a marker that isn't first on its page
constexpr qreal kSongH         = 18;
constexpr qreal kTitleX        = 34;
constexpr qreal kIndentX       = 50;

struct Placement { int page; qreal y; };
}

void printPlaylist(QPagedPaintDevice *device, const PlaylistPrintJob &job) {
    QPainter painter(device);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const qreal u = device->logicalDpiY() / 72.0;     // device pixels per point
    auto P = [u](qreal points) { return points * u; };
    const qreal W = device->width() / u;               // printable area, in points
    const qreal H = device->height() / u;

    auto font = [](qreal pointSize, QFont::Weight weight = QFont::Normal) {
        QFont f("Helvetica Neue");
        f.setPointSizeF(pointSize);
        f.setWeight(weight);
        return f;
    };
    auto textWidth = [device, u](const QFont &f, const QString &s) {
        return QFontMetricsF(f, device).horizontalAdvance(s) / u;  // in points
    };
    auto drawText = [&](qreal x, qreal y, qreal w, qreal h, int flags, const QString &s) {
        painter.drawText(QRectF(P(x), P(y), P(w), P(h)), flags, s);
    };

    const QColor ink("#222222"), muted("#8a8a8a"), rule("#d8d8d8"), missingColor("#c00000");
    const qreal xLevels = W - 130, xPitch = W - 70, xTempo = W;   // column centers / right edges

    int nTips = 0, nSongs = 0;
    for (const auto &row : job.rows) {
        row.isMarker ? nTips++ : nSongs++;
    }
    const bool hasMarkers = (nTips > 0);

    // ---- pass 1: decide which page each row lands on, keeping each tip together when it fits ----
    const qreal firstTop = kHeaderH + kHeadingsH;
    const qreal laterTop = kContHeaderH + kHeadingsH;
    const qreal bottom   = H - kFooterH;

    QList<Placement> where(job.rows.size());
    int page = 0;
    qreal top = firstTop, y = top;

    auto rowHeight = [&](int i, qreal atY, qreal pageTop) {
        if (!job.rows[i].isMarker) {
            return kSongH;
        }
        return kMarkerH + (atY > pageTop ? kMarkerGap : 0);
    };
    auto newPage = [&]() { page++; top = laterTop; y = top; };

    for (int i = 0; i < job.rows.size(); i++) {
        if (job.rows[i].isMarker || i == 0) {
            // start of a tip (or the songs before the first marker): move it to a fresh page if it
            //   doesn't fit here but would fit there
            qreal blockH = rowHeight(i, y, top);
            for (int j = i + 1; j < job.rows.size() && !job.rows[j].isMarker; j++) {
                blockH += kSongH;
            }
            if (y > top && y + blockH > bottom && blockH <= bottom - laterTop) {
                newPage();
            }
        }
        qreal h = rowHeight(i, y, top);
        qreal needed = h;
        if (job.rows[i].isMarker && i + 1 < job.rows.size() && !job.rows[i + 1].isMarker) {
            needed += kSongH;  // never leave a marker alone at the bottom of a page
        }
        if (y > top && y + needed > bottom) {
            newPage();
            h = rowHeight(i, y, top);
        }
        where[i] = { page, y + (h - (job.rows[i].isMarker ? kMarkerH : kSongH)) };
        y += h;
    }
    const int pageCount = page + 1;

    // ---- pass 2: draw ----
    auto drawPageFrame = [&](int pageNum) {
        qreal headingsY;
        if (pageNum == 0) {
            painter.setPen(ink);
            painter.setFont(font(22, QFont::Bold));
            drawText(0, 0, W * 0.65, 30, Qt::AlignLeft | Qt::AlignVCenter,
                     QFontMetricsF(painter.font(), device).elidedText(job.name, Qt::ElideRight, P(W * 0.65)));
            painter.setPen(muted);
            painter.setFont(font(10));
            drawText(0, 30, W * 0.65, 16, Qt::AlignLeft | Qt::AlignVCenter, job.folders.join(QString("  ›  ")));
            QString counts = hasMarkers ? QString("%1 tips  ·  %2 songs").arg(nTips).arg(nSongs)
                                        : QString("%1 songs").arg(nSongs);
            drawText(0, 4, W, 16, Qt::AlignRight | Qt::AlignVCenter, counts);
            drawText(0, 20, W, 16, Qt::AlignRight | Qt::AlignVCenter, job.dateLine);
            headingsY = kHeaderH;
        } else {
            painter.setPen(ink);
            painter.setFont(font(12, QFont::Bold));
            drawText(0, 0, W * 0.75, 20, Qt::AlignLeft | Qt::AlignVCenter, job.name);
            painter.setPen(muted);
            painter.setFont(font(9));
            drawText(0, 0, W, 20, Qt::AlignRight | Qt::AlignVCenter, "continued");
            headingsY = kContHeaderH;
        }
        painter.setPen(QPen(ink, P(pageNum == 0 ? 1.2 : 0.8)));
        painter.drawLine(QPointF(0, P(headingsY)), QPointF(P(W), P(headingsY)));

        qreal hy = headingsY + 6;
        painter.setPen(muted);
        painter.setFont(font(7.5, QFont::DemiBold));
        drawText(0, hy, 26, 12, Qt::AlignCenter, job.tipNumbers ? "TIP" : "#");
        drawText(kTitleX, hy, 200, 12, Qt::AlignLeft | Qt::AlignVCenter, "TITLE");
        drawText(xLevels - 30, hy, 60, 12, Qt::AlignCenter, "LEVELS");
        drawText(xPitch - 40, hy, 40, 12, Qt::AlignRight | Qt::AlignVCenter, "PITCH");
        drawText(xTempo - 40, hy, 40, 12, Qt::AlignRight | Qt::AlignVCenter, "TEMPO");

        painter.setPen(QPen(rule, P(0.5)));
        painter.drawLine(QPointF(0, P(H - kFooterH + 6)), QPointF(P(W), P(H - kFooterH + 6)));
        painter.setPen(muted);
        painter.setFont(font(8));
        drawText(0, H - kFooterH + 8, W, 14, Qt::AlignLeft | Qt::AlignVCenter, "Printed from SquareDesk");
        drawText(0, H - kFooterH + 8, W, 14, Qt::AlignRight | Qt::AlignVCenter,
                 QString("Page %1 of %2").arg(pageNum + 1).arg(pageCount));
    };

    int currentPage = -1;
    bool underMarker = false;
    for (int i = 0; i < job.rows.size(); i++) {
        const PlaylistPrintRow &row = job.rows[i];
        if (where[i].page != currentPage) {
            if (currentPage >= 0) {
                device->newPage();
            }
            currentPage = where[i].page;
            drawPageFrame(currentPage);
        }
        const qreal ry = where[i].y;

        if (row.isMarker) {
            underMarker = true;
            const QColor band = markerBandColor(row.title);
            const QColor accent = markerInkColor(band);

            QPainterPath bandPath;
            bandPath.addRoundedRect(QRectF(0, P(ry), P(W), P(kMarkerH)), P(3), P(3));
            painter.fillPath(bandPath, band);
            painter.fillRect(QRectF(0, P(ry), P(4), P(kMarkerH)), accent);

            if (!row.number.isEmpty()) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(accent);
                painter.drawEllipse(QPointF(P(17), P(ry + kMarkerH / 2)), P(8.5), P(8.5));
                painter.setBrush(Qt::NoBrush);
                painter.setPen(Qt::white);
                painter.setFont(font(row.number.size() > 2 ? 7 : 9.5, QFont::Bold));
                drawText(8.5, ry, 17, kMarkerH, Qt::AlignCenter, row.number);
            }

            QFont mf = font(11.5, QFont::Bold);
            painter.setFont(mf);
            painter.setPen(accent);
            QString markerText = cleanPlaylistMarkerTitle(row.title);
            drawText(kTitleX, ry, W - kTitleX - 6, kMarkerH, Qt::AlignLeft | Qt::AlignVCenter,
                     QFontMetricsF(mf, device).elidedText(markerText, Qt::ElideRight, P(W - kTitleX - 6)));
            continue;
        }

        // ---- a song ----
        if (!row.number.isEmpty()) {
            painter.setPen(muted);
            painter.setFont(font(8.5));
            drawText(0, ry, 26, kSongH, Qt::AlignCenter, row.number);
        }

        const qreal x0 = (hasMarkers && underMarker) ? kIndentX : kTitleX;

        QFont tagFont = font(7.5, QFont::Bold);
        qreal tagsW = 0;
        for (const auto &tag : row.tags) {
            tagsW += textWidth(tagFont, tag.text) + 10 + 4;
        }

        QFont tf = font(10.5);
        tf.setStrikeOut(row.missing);
        const qreal titleRoom = qMax<qreal>(60, xLevels - 34 - x0 - (tagsW > 0 ? tagsW + 7 : 0));
        QString titleText = QFontMetricsF(tf, device).elidedText(row.title, Qt::ElideRight, P(titleRoom));
        painter.setFont(tf);
        painter.setPen(row.missing ? missingColor : (row.titleColor.isValid() ? row.titleColor : ink));
        drawText(x0, ry, titleRoom, kSongH, Qt::AlignLeft | Qt::AlignVCenter, titleText);

        qreal x = x0 + textWidth(tf, titleText) + 7;
        for (const auto &tag : row.tags) {
            qreal w = textWidth(tagFont, tag.text) + 10;
            QRectF pill(P(x), P(ry + 4), P(w), P(kSongH - 8));
            QPainterPath pillPath;
            pillPath.addRoundedRect(pill, P(5), P(5));
            painter.fillPath(pillPath, tag.background);
            if (tag.background.lightness() > 230) {
                // a white or near-white pill needs an outline to be seen on paper
                painter.setPen(QPen(tag.foreground, P(0.6)));
                painter.setBrush(Qt::NoBrush);
                painter.drawPath(pillPath);
            }
            painter.setPen(tag.foreground);
            painter.setFont(tagFont);
            painter.drawText(pill, Qt::AlignCenter, tag.text);
            x += w + 4;
        }

        painter.setPen(ink);
        painter.setFont(font(9.5));
        drawText(xLevels - 30, ry, 60, kSongH, Qt::AlignCenter, row.levels);

        painter.setFont(font(10));
        QString pitchText = signedPitch(row.pitch);
        painter.setPen((pitchText == "0" || pitchText.isEmpty()) ? muted : ink);
        drawText(xPitch - 40, ry, 40, kSongH, Qt::AlignRight | Qt::AlignVCenter, pitchText);
        painter.setPen(ink);
        drawText(xTempo - 40, ry, 40, kSongH, Qt::AlignRight | Qt::AlignVCenter, row.tempo.trimmed());

        painter.setPen(QPen(rule, P(0.5)));
        painter.drawLine(QPointF(P(x0), P(ry + kSongH)), QPointF(P(W), P(ry + kSongH)));
    }

    if (job.rows.isEmpty()) {
        drawPageFrame(0);
    }
}
