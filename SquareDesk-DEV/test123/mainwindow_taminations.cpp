/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
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

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "embeddedserver.h"
#include "ui_mainwindow.h"
#include <QWebEngineView>
#include <QVBoxLayout>
#include <QTimer>

void MainWindow::startTaminationsServer() {
    // qDebug() << "startTaminationsServer";

    QWebEngineProfile* profile = new QWebEngineProfile();
    // profile->setPersistentStoragePath(musicRootPath + "/taminationsData"); // Or other path
    profile->setPersistentStoragePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    // qDebug() << "writableLocation:" << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    // ...
    // Debug: Print all tabs in the tabWidget
    // qDebug() << "Total tabs in tabWidget:" << ui->tabWidget->count();
    // for (int i = 0; i < ui->tabWidget->count(); ++i) {
    //     QWidget *tab = ui->tabWidget->widget(i);
    //     QString tabName = tab->objectName();
    //     QString tabText = ui->tabWidget->tabText(i);
    //     qDebug() << "Tab" << i << ":" << tabName << "(" << tabText << ")";
    // }
    
    // Check if TaminationsTab exists, if not, create it
    QWidget *taminationsTab = ui->tabWidget->findChild<QWidget*>("TaminationsTab");
    if (!taminationsTab) {
        // qDebug() << "TaminationsTab not found, creating it programmatically";
        
        // Create the TaminationsTab
        taminationsTab = new QWidget();
        taminationsTab->setObjectName("TaminationsTab");
        
        // Add it to the tabWidget (insert it after SD tab, before Dance Programs tab)
        int insertIndex = 3;  // After SD tab (index 2), before Dance Programs tab
        ui->tabWidget->insertTab(insertIndex, taminationsTab, "Taminations");
        
        // qDebug() << "TaminationsTab created and added at index:" << insertIndex;
    } else {
        // qDebug() << "TaminationsTab found!";
        
        // Make sure the tab is visible (not hidden)
        int tabIndex = ui->tabWidget->indexOf(taminationsTab);
        if (tabIndex >= 0) {
            // qDebug() << "TaminationsTab is at index:" << tabIndex;
            ui->tabWidget->setTabVisible(tabIndex, true);  // Ensure it's visible
        } else {
            // qDebug() << "TaminationsTab not found in tabWidget!";
        }
    }
    
    taminationsServer = new EmbeddedServer();

    if (taminationsServer->start()) {
        // Give the server a moment to fully start
        QTimer::singleShot(100, [this]() {
            QString url = taminationsServer->getServerUrl();
            url += "?Speed=Fast";  // default the sequencer dancer speed to "Fast", rather than "Normal"
            if (!url.isEmpty()) {
                // qDebug() << "Loading URL:" << url;
                
                // Create and set up the webview for the TaminationsTab
                QWebEngineView *taminationsWebView = new QWebEngineView();

                QWebEngineSettings* settings = taminationsWebView->page()->settings();
                settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);

                // Find the TaminationsTab widget (should exist now)
                QWidget *taminationsTab = nullptr;
                
                // Try finding by object name first
                taminationsTab = ui->tabWidget->findChild<QWidget*>("TaminationsTab");
                
                // If not found by name, try finding by tab text
                if (!taminationsTab) {
                    for (int i = 0; i < ui->tabWidget->count(); ++i) {
                        if (ui->tabWidget->tabText(i) == "Taminations") {
                            taminationsTab = ui->tabWidget->widget(i);
                            break;
                        }
                    }
                }
                
                if (taminationsTab) {
                    // Clear any existing layout/widgets
                    if (taminationsTab->layout()) {
                        delete taminationsTab->layout();
                    }
                    
                    // Create a new layout and add the webview
                    QVBoxLayout *layout = new QVBoxLayout(taminationsTab);
                    layout->setContentsMargins(0, 0, 0, 0);  // Remove margins for full-screen effect
                    layout->addWidget(taminationsWebView);
                    
                    // Load the Flutter web app
                    taminationsWebView->load(QUrl(url));
                    
                    // Store reference to the webview (optional, for later access)
                    webViews.append(taminationsWebView);
                    
                    // qDebug() << "Taminations webview created and URL loaded";
                } else {
                    qWarning() << "Could not find TaminationsTab widget even after creating it!";
                    delete taminationsWebView;  // Clean up if tab not found
                }
            }
        });
    } else {
        qCritical() << "Failed to start embedded Taminations server";
    }

}
