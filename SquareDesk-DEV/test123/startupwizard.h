/****************************************************************************
**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
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

#ifndef STARTUPWIZARD_H
#define STARTUPWIZARD_H

#include <QWizard>
#include <QDir>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QRadioButton;
QT_END_NAMESPACE

// --------------------------------------
class StartupWizard : public QWizard
{
    Q_OBJECT

public:
    StartupWizard(QWidget *parent = 0);

    void accept() Q_DECL_OVERRIDE;
};

// --------------------------------------
class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = 0);

private:
    QLabel *label;
};


// --------------------------------------
class OutputFilesPage : public QWizardPage
{
    Q_OBJECT

public:
    OutputFilesPage(QWidget *parent = 0);

protected:
    void initializePage() Q_DECL_OVERRIDE;

private:
    QLabel *outputDirLabel;
    QLabel *patterLabel;
    QLabel *singingCallLabel;
    QLabel *vocalsLabel;
    QLabel *extrasLabel;

    QLineEdit *outputDirLineEdit;
    QLineEdit *patterLineEdit;
    QLineEdit *singingCallLineEdit;
    QLineEdit *vocalsLineEdit;
    QLineEdit *extrasLineEdit;
};

// GLOBALS
int countMP3FilesInDir(QDir rootDir);
void copyMP3FilesFromTo(QDir fromDir, QDir toDir);

// --------------------------------------
class CopyFilesPage : public QWizardPage
{
    Q_OBJECT

public:
    CopyFilesPage(QWidget *parent = 0);

protected:
    void initializePage() Q_DECL_OVERRIDE;

private:
    QCheckBox *patterCheckBox;
    QCheckBox *singingCallCheckBox;
    QCheckBox *vocalsCheckBox;
    QCheckBox *extrasCheckBox;

    QLabel *patterLabel;
    QLabel *singingCallLabel;
    QLabel *vocalsLabel;
    QLabel *extrasLabel;

    QLineEdit *patterLineEdit;
    QLineEdit *singingCallLineEdit;
    QLineEdit *vocalsLineEdit;
    QLineEdit *extrasLineEdit;

    QPushButton *patterPushButton;
    QPushButton *singingCallPushButton;
    QPushButton *vocalsPushButton;
    QPushButton *extrasPushButton;
};

// --------------------------------------
class ConclusionPage : public QWizardPage
{
    Q_OBJECT

public:
    ConclusionPage(QWidget *parent = 0);

protected:
    void initializePage() Q_DECL_OVERRIDE;

private:
    QLabel *label;
};

#endif
