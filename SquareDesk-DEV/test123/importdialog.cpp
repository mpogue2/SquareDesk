/****************************************************************************
**
** Copyright (C) 2016, 2017 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk/SquareDeskPlayer application.
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

#include "importdialog.h"
#include "ui_importdialog.h"
#include "QFileDialog"

ImportDialog::ImportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportDialog)
{
    ui->setupUi(this);
}


// ----------------------------------------------------------------
ImportDialog::~ImportDialog()
{
    delete ui;
}


QStringList readLine(QFile &file, bool tab_separator)
{
    QStringList list;
    QString line;

    while (!file.atEnd() && line.isEmpty())
    {
        line = file.readLine().simplified();
    }

    if (!file.atEnd())
    {
        list = line.split(tab_separator ? '\t' : ',');
    }       
    return list;
}

static void setComboBoxColumn(QComboBox *comboBox, const QStringList &fields, int which, bool excludeNone = false )
{
    comboBox->clear();
    if (!excludeNone)
        comboBox->addItem("None", -1);
    else
        which--;
            
    for (int i = 0; i < fields.length(); ++i)
    {
        comboBox->addItem("Column " + QString("%1").arg(i + 1) + " - " + fields[i], i);
    }
    if (which < comboBox->count())
    {
        comboBox->setCurrentIndex(which);
    }
}


void ImportDialog::on_pushButtonChooseFile_clicked()
{
    QString filename =
        QFileDialog::getOpenFileName(this, tr("Select Import File"),
                                     QDir::homePath(),
                                     tr("Data Files (*.csv *.tsv *.txt)"));

    if (filename.isNull()) {
        return;  // user cancelled the "Select Base Directory for Music" dialog...so don't do anything, just return
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open " << filename;
        return;
    }
    QString line;
    while (!file.atEnd() && line.isEmpty())
    {
        line = file.readLine().trimmed();
    }
    QStringList fieldsTabs;
    QStringList fieldsCommas;
    
    if (!file.atEnd())
    {
        fieldsTabs = line.split((char)(9));
        fieldsCommas = line.split(',');
    }

    bool usesTabs = (fieldsTabs.length() > fieldsCommas.length());
    qDebug() << "usesTabs " << usesTabs << " " << fieldsTabs.length() << "/" << fieldsCommas.length();
    ui->comboBoxTabOrCSV->setCurrentIndex(usesTabs ? 0 : 1);
    QStringList &fields(usesTabs ? fieldsTabs : fieldsCommas);

    QRegularExpression regexNumbers("^\\d+\\%?$");
    bool firstRowHeaders = true;
    QListIterator<QString> iter(fields);
    while (iter.hasNext())
    {
        QString s = iter.next();
        QRegularExpressionMatch match(regexNumbers.match(s));
        if (match.hasMatch())
            firstRowHeaders = false;
    }

    ui->comboBoxFirstRow->setCurrentIndex(firstRowHeaders ? 0 : 1);
        
    setComboBoxColumn(ui->comboBoxColumnSongName, fields, 1, true);
    setComboBoxColumn(ui->comboBoxColumnPitch, fields, 2);
    setComboBoxColumn(ui->comboBoxColumnTempo, fields, 3);
    setComboBoxColumn(ui->comboBoxColumnIntro, fields, 4);
    setComboBoxColumn(ui->comboBoxColumnOutro, fields, 5);
    setComboBoxColumn(ui->comboBoxColumnVolume, fields, 6);
    setComboBoxColumn(ui->comboBoxColumnCuesheet, fields, 7);
    
    
    ui->labelFileName->setText(filename);
}

