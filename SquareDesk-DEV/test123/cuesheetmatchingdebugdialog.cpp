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

#include "cuesheetmatchingdebugdialog.h"
#include "mainwindow.h"
#include <QDebug>
#include <QApplication>
#include <QRegularExpression>

CuesheetMatchingDebugDialog::CuesheetMatchingDebugDialog(MainWindow *parent)
    : QDialog(parent), mainWindow(parent)
{
    setWindowTitle("Explore Cuesheet Matching");
    setModal(false);
    resize(600, 400);
    
    setupUI();
    connectSignals();
}

void CuesheetMatchingDebugDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create input fields layout
    QGridLayout *inputLayout = new QGridLayout();
    
    // Song filename
    QLabel *songLabel = new QLabel("Song filename:");
    songFilenameEdit = new QLineEdit();
    inputLayout->addWidget(songLabel, 0, 0);
    inputLayout->addWidget(songFilenameEdit, 0, 1);
    songFilenameEdit->setPlaceholderText("type song name here, do not put .mp3 on the end");
    
    // Cuesheet filename
    QLabel *cuesheetLabel = new QLabel("Cuesheet filename:");
    cuesheetFilenameEdit = new QLineEdit();
    inputLayout->addWidget(cuesheetLabel, 1, 0);
    inputLayout->addWidget(cuesheetFilenameEdit, 1, 1);
    cuesheetFilenameEdit->setPlaceholderText("type cuesheet name here, do not put .html on the end");

    mainLayout->addLayout(inputLayout);
    
    // Debug output area
    QLabel *debugLabel = new QLabel("Debug Output:");
    debugOutputEdit = new QTextEdit();
    debugOutputEdit->setReadOnly(true);
    debugOutputEdit->setFont(QFont("Courier", 12)); // Monospace font for debug output
    
    mainLayout->addWidget(debugLabel);
    mainLayout->addWidget(debugOutputEdit);
    
    setLayout(mainLayout);
}

void CuesheetMatchingDebugDialog::connectSignals()
{
    connect(songFilenameEdit, &QLineEdit::textChanged, this, &CuesheetMatchingDebugDialog::onTextChanged);
    connect(cuesheetFilenameEdit, &QLineEdit::textChanged, this, &CuesheetMatchingDebugDialog::onTextChanged);
}

void CuesheetMatchingDebugDialog::onTextChanged()
{
    QString songFilename = songFilenameEdit->text();
    QString cuesheetFilename = cuesheetFilenameEdit->text();
    
    // Only proceed if both fields have content
    if (songFilename.isEmpty() || cuesheetFilename.isEmpty()) {
        debugOutputEdit->clear();
        debugOutputEdit->append("Enter both song filename and cuesheet filename to see debug output...");
        return;
    }
    
    // Clear previous output
    debugOutputEdit->clear();
    
    // Call the actual scoring function with debug output
    if (mainWindow) {
        mainWindow->MP3FilenameVsCuesheetnameScore(songFilename, cuesheetFilename, debugOutputEdit);
        // The debug output is now generated directly by the real function
    }
}
