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

#ifndef SDWEBVIEW_H
#define SDWEBVIEW_H

// SDWebView is the web view used by the Reference tab and the Taminations tab. (Issue #1791)
//
//   USE_QWEBENGINE defined:  SDWebView is Qt's QWebEngineView (bundled Chromium, ~300 MB).
//                            This is the Linux build, or a macOS build with CONFIG+=webengine.
//   otherwise (macOS):       SDWebView is SquareDeskWebView, a thin wrapper around the system
//                            WKWebView (the engine Safari uses), which ships with macOS.
//
// SquareDeskWebView implements only the parts of the QWebEngineView API that SquareDesk calls,
//   so that the call sites are the same lines in both builds.

#if defined(USE_QWEBENGINE)

#include <QtWebEngineWidgets/QtWebEngineWidgets>
using SDWebView = QWebEngineView;

#else

#include <QWidget>
#include <QUrl>
#include <QString>

class SquareDeskWebView : public QWidget
{
    Q_OBJECT

public:
    explicit SquareDeskWebView(QWidget *parent = nullptr);
    ~SquareDeskWebView() override;

    // Local .htm/.html/.txt files are read and handed to WebKit with an explicit encoding,
    //   because WebKit (unlike Chromium) assumes Latin-1 for files that don't declare a charset,
    //   and many cuesheets are UTF-8 with no charset. Other local files (e.g. PDFs, which WebKit
    //   displays natively) and http(s) URLs are loaded as-is.
    void setUrl(const QUrl &url);
    void load(const QUrl &url) { setUrl(url); }

    void setHtml(const QString &html, const QUrl &baseUrl = QUrl());
    void setZoomFactor(qreal factor);

    // Hides QWidget::hasFocus(). Keyboard focus lives in the native WKWebView, which Qt's focus
    //   tracking does not see, so this asks AppKit whether the first responder is inside it.
    bool hasFocus() const;

protected:
    void showEvent(QShowEvent *event) override;

private:
    struct Private;
    Private *d;
};

using SDWebView = SquareDeskWebView;

#endif // USE_QWEBENGINE

#endif // SDWEBVIEW_H
