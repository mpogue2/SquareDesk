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

#include "exportdialog.h"
#include "ui_exportdialog.h"
#include "QFileDialog"
#include "songsettings.h"
#include "utility.h"

ExportDialog::ExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportDialog)
{
    ui->setupUi(this);
}


// ----------------------------------------------------------------
ExportDialog::~ExportDialog()
{
    delete ui;
}

void ExportDialog::on_pushButtonChooseFile_clicked()
{
    QString filename =
        QFileDialog::getSaveFileName(this, tr("Select Export File"),
                                     QDir::homePath());

    if (filename.isNull()) {
        return;  // user cancelled the "Select Base Directory for Music" dialog...so don't do anything, just return
    }

    ui->labelFileName->setText(filename);
}

static void outputString(QTextStream &stream, const QString &str, bool quote)
{
    if (!quote)
    {
        stream << str;
    }
    else
    {
        QString quotedString(str);
        quotedString.replace("\"", "\\\"");
        stream << "\"" << str << "\"";
    }
}

void exportSongList(QTextStream &stream, SongSettings &settings, QList<QString> *musicFilenames,
                    int outputFieldCount, enum ColumnExportData outputFields[],
                    char separator,
                    bool includeHeaderNames,
                    bool relativePathNames)
{
    const char *headerNames[] = { "Filename", "Pitch", "Tempo", "Intro", "Outro", "Volume", "Cuesheet", "None" };
    if (includeHeaderNames)
    {
        for (int i = 0; i < outputFieldCount; ++i)
        {
            if (i > 0) stream << separator;
            stream << headerNames[outputFields[i]];
        }
        stream << ENDL;
    }
    
    QListIterator<QString> musicFilenameIter(*musicFilenames);
    while (musicFilenameIter.hasNext())
    {
        QString s(musicFilenameIter.next());
        QStringList sl1 = s.split("#!#");
        QString filename = sl1[1];  // everything else

        SongSetting setting;
        if (settings.loadSettings(filename, setting))
        {
            for (int i = 0; i < outputFieldCount; ++i)
            {
                if (i > 0) stream << separator;
                
                switch (outputFields[i])
                {
                case ExportDataFileName :
                    if (relativePathNames)
                    {
                        outputString(stream, settings.removeRootDirs(filename), ',' == separator);
                    }
                    else
                    {
                        outputString(stream, filename, ',' == separator);
                    }
                    break;
                case ExportDataPitch :
                    if (setting.isSetPitch())
                        stream << setting.getPitch();
                    break;
                case ExportDataTempo :
                    if (setting.isSetTempo())
                        stream << setting.getTempo();
                    if (setting.getTempoIsPercent())
                        stream << '%';
                    break;
                case ExportDataIntro :
                    if (setting.isSetIntroOutroIsTimeBased())
                    {
                        if (setting.getIntroOutroIsTimeBased())
                        {
                            stream << doubleToTime(setting.getIntroPos());
                        }
                        else if (setting.isSetSongLength())
                        {
                            stream << doubleToTime(setting.getIntroPos() * setting.getSongLength());
                        }
                    }
                    break;
                case ExportDataOutro :
                    if (setting.isSetIntroOutroIsTimeBased())
                    {
                        if (setting.getIntroOutroIsTimeBased())
                        {
                            stream << doubleToTime(setting.getOutroPos());
                        }
                        else if (setting.isSetSongLength())
                        {
                            stream << doubleToTime(setting.getOutroPos() * setting.getSongLength());
                        }
                    }
                    break;
                case ExportDataVolume :
                    if (setting.isSetVolume())
                        stream << setting.getVolume();
                    break;
                case ExportDataCuesheetPath :
                    if (relativePathNames)
                    {
                        outputString(stream, settings.removeRootDirs(setting.getCuesheetName()),  ',' == separator);
                    }
                    else
                    {
                        outputString(stream, setting.getCuesheetName(), ',' == separator);
                    }
                    break;
                case ExportDataNone :
                    break;
                }
            } // end of iterating through fields
            stream << ENDL;
        } // end of if we could load settings for this file
    } // end of while iterating through filenames
}

void ExportDialog::exportSongs(SongSettings &settings, QList<QString> *musicFilenames)
{
    QString filename(ui->labelFileName->text());
    QFile file( filename );
    if ( file.open(QIODevice::WriteOnly) )
    {
        QTextStream stream( &file );

        enum ColumnExportData outputFields[7];
        int outputFieldCount = sizeof(outputFields) / sizeof(*outputFields);
        char separator = (ui->comboBoxTabOrCSV->currentIndex() == 0) ? '\t' : ',';

        outputFields[0] = (ColumnExportData)(ui->comboBoxColumn1->currentIndex());
        outputFields[1] = (ColumnExportData)(ui->comboBoxColumn2->currentIndex());
        outputFields[2] = (ColumnExportData)(ui->comboBoxColumn3->currentIndex());
        outputFields[3] = (ColumnExportData)(ui->comboBoxColumn4->currentIndex());
        outputFields[4] = (ColumnExportData)(ui->comboBoxColumn5->currentIndex());
        outputFields[5] = (ColumnExportData)(ui->comboBoxColumn6->currentIndex());
        outputFields[6] = (ColumnExportData)(ui->comboBoxColumn7->currentIndex());

        while (outputFieldCount > 0 && outputFields[outputFieldCount - 1] == ExportDataNone)
        {
            outputFieldCount--;
        }
        exportSongList(stream, settings, musicFilenames,
                       outputFieldCount, outputFields,
                       separator,
                       0 == ui->comboBoxFirstRow->currentIndex(),
                       0 == ui->comboBoxFormatSongName->currentIndex());

    } // end of successful open
}
