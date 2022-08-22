#ifndef MAKEFLASHDRIVEWIZARD_H
#define MAKEFLASHDRIVEWIZARD_H
/****************************************************************************
**
** Copyright (C) 2016-2022 Mike Pogue, Dan Lyke
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

#include <QWizard>
#include <QDir>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QRadioButton;
QT_END_NAMESPACE

// --------------------------------------
class MakeFlashDriveWizard : public QWizard
{
    Q_OBJECT

public:
    MakeFlashDriveWizard(QWidget *parent = 0);
    void accept() Q_DECL_OVERRIDE;
};

// --------------------------------------
class IntroMakeFlashDrivePage : public QWizardPage
{
    Q_OBJECT

public:
    IntroMakeFlashDrivePage(QWidget *parent = 0);

private:
    QLabel *label;
};


// --------------------------------------
class VolumeMakeFlashDrivePage : public QWizardPage
{
    Q_OBJECT

public:
    VolumeMakeFlashDrivePage(QWidget *parent = 0);

protected:
    void initializePage() Q_DECL_OVERRIDE;

private:
    QLabel *label;
    QComboBox *combobox;
};


// --------------------------------------
class ConclusionMakeFlashDrivePage : public QWizardPage
{
    Q_OBJECT

public:
    ConclusionMakeFlashDrivePage(QWidget *parent = 0);

protected:
    void initializePage() Q_DECL_OVERRIDE;

private:
    QLabel *label;
};

#endif // MAKEFLASHDRIVEWIZARD_H
