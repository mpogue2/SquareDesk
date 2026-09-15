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

// SplashScreen.h
#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QScreen>
#include <QGuiApplication>
#include <QVBoxLayout>

class SplashScreen : public QWidget {
    Q_OBJECT
public:
    explicit SplashScreen(const QString& version = "", QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

public slots:
    void setProgress(int value, const QString& message);

private:
    QProgressBar* progressBar;
    QLabel* statusLabel;
    QLabel* versionLabel;
    QLabel* imageLabel;
    QPixmap roundedPixmap(const QPixmap& input, int radius);
};

#endif // SPLASHSCREEN_H
