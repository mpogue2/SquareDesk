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

// SquareDeskWebView: a QWidget hosting the system WKWebView. See sdwebview.h. (Issue #1791)
//
// NOTE: this file is compiled WITHOUT ARC (qmake's default), so the WKWebView is retained and
//   released by hand.

#import <WebKit/WebKit.h>
#import <AppKit/AppKit.h>

#include "sdwebview.h"

#include <QFileInfo>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#if __has_feature(objc_arc)
#error "sdwebview.mm does manual retain/release; build it without -fobjc-arc"
#endif

struct SquareDeskWebView::Private {
    WKWebView *webView  = nil;      // retained
    QWidget   *container = nullptr; // owns the QWindow that wraps webView
    QUrl       pendingFileUrl;      // a PDF waiting for this view to be shown (see setUrl)
};

SquareDeskWebView::SquareDeskWebView(QWidget *parent)
    : QWidget(parent), d(new Private)
{
    @autoreleasepool {
        WKWebViewConfiguration *config = [[[WKWebViewConfiguration alloc] init] autorelease];
        // The default website data store is persistent (per app), so localStorage survives
        //   restarts -- Taminations keeps its settings there.
        d->webView = [[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:config];

#ifndef QT_NO_DEBUG
        // Debug builds: allow Safari's Develop menu > Web Inspector to attach.
        if (@available(macOS 13.3, *)) {
            d->webView.inspectable = YES;
        }
#endif
    }

    QWindow *window = QWindow::fromWinId(reinterpret_cast<WId>(d->webView));
    d->container = QWidget::createWindowContainer(window, this);  // takes ownership of window

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(d->container);
}

SquareDeskWebView::~SquareDeskWebView()
{
    @autoreleasepool {
        [d->webView stopLoading];
        delete d->container;        // destroys the QWindow, detaching the NSView from our window
        d->container = nullptr;
        [d->webView release];
        d->webView = nil;
    }
    delete d;
}

void SquareDeskWebView::setUrl(const QUrl &url)
{
    @autoreleasepool {
        if (!url.isLocalFile()) {
            [d->webView loadRequest:[NSURLRequest requestWithURL:url.toNSURL()]];
            return;
        }

        QFileInfo info(url.toLocalFile());
        NSURL *fileURL   = url.toNSURL();
        NSURL *folderURL = [NSURL fileURLWithPath:info.absolutePath().toNSString() isDirectory:YES];
        const QString suffix = info.suffix().toLower();

        if (suffix == "htm" || suffix == "html" || suffix == "txt") {
            NSData *data = [NSData dataWithContentsOfURL:fileURL];
            if (data != nil) {
                // Valid UTF-8 (which includes plain ASCII) is loaded as UTF-8; anything else is
                //   assumed to be Windows-1252, the usual encoding of older cuesheets.
                NSString *asUTF8 = [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease];
                NSString *encoding = (asUTF8 != nil) ? @"UTF-8" : @"windows-1252";
                NSString *mimeType = (suffix == "txt") ? @"text/plain" : @"text/html";

                // Base URL = the file's folder, so relative images and CSS still load.
                [d->webView loadData:data MIMEType:mimeType characterEncodingName:encoding baseURL:folderURL];
                return;
            }
        }

        // Everything else (PDFs, or a text file we couldn't read): let WebKit load it directly,
        //   with read access to its folder.
        //
        // WebKit's PDF viewer picks its fit-to-width scale ONCE, from the view's size when the
        //   document finishes loading, and doesn't redo it when the view is resized later. The
        //   Reference tab creates its views while hidden, with no real size yet, so a PDF that
        //   finished loading before its tab was first shown came up as tiny pages (which PDF won
        //   that race was just timing). So a hidden view waits until it is shown. (#1791)
        if (!isVisible()) {
            d->pendingFileUrl = url;
            return;
        }
        d->pendingFileUrl.clear();
        [d->webView loadFileURL:fileURL allowingReadAccessToURL:folderURL];
    }
}

void SquareDeskWebView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (!d->pendingFileUrl.isEmpty()) {
        // Queued, so the layout has given the native view its real size before WebKit loads.
        QTimer::singleShot(0, this, [this]() {
            if (!d->pendingFileUrl.isEmpty() && isVisible()) {
                setUrl(d->pendingFileUrl);
            }
        });
    }
}

void SquareDeskWebView::setHtml(const QString &html, const QUrl &baseUrl)
{
    @autoreleasepool {
        [d->webView loadHTMLString:html.toNSString() baseURL:(baseUrl.isEmpty() ? nil : baseUrl.toNSURL())];
    }
}

void SquareDeskWebView::setZoomFactor(qreal factor)
{
    d->webView.pageZoom = factor;
}

bool SquareDeskWebView::hasFocus() const
{
    NSResponder *responder = d->webView.window.firstResponder;
    return [responder isKindOfClass:[NSView class]] && [(NSView *)responder isDescendantOf:d->webView];
}
