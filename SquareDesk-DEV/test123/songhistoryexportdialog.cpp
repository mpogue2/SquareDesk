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

#include "songhistoryexportdialog.h"
#include "ui_songhistoryexportdialog.h"
#include "QFileDialog"
#include "songsettings.h"
#include "sessioninfo.h"
#include "utility.h"

SongHistoryExportDialog::SongHistoryExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SongHistoryExportDialog)
{
    ui->setupUi(this);
}


// ----------------------------------------------------------------
SongHistoryExportDialog::~SongHistoryExportDialog()
{
    delete ui;
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
        quotedString.replace("\"", "\"\"");
        stream << "\"" << quotedString << "\"";
    }
}

void SongHistoryExportDialog::populateOptions(SongSettings &songSettings)
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime yesterday = now.addDays(-1);
    
    ui->dateTimeEditEnd->setDateTime(now);
    ui->dateTimeEditStart->setDateTime(yesterday);
    
    QList<SessionInfo> sessions(songSettings.getSessionInfo());
    ui->comboBoxSession->clear();
    ui->comboBoxSession->addItem("<all sessions>", 0);
    for (auto session : sessions)
    {
        ui->comboBoxSession->addItem(session.name, session.id);
    }
}



class FileExportSongPlayEvent : public SongPlayEvent {
    QTextStream &stream;
public:
    FileExportSongPlayEvent(QTextStream &stream) : stream(stream) {}
    virtual void operator() (const QString &name, const QString &playedOnUTC, const QString &playedOnLocal)
    {
        outputString(stream, name, true);
        stream << ",";
        outputString(stream, playedOnLocal, true);
        stream << ",";
        outputString(stream, playedOnUTC, true);
        stream << "\n";
    }
    virtual ~FileExportSongPlayEvent() {}
};


void SongHistoryExportDialog::exportSongPlayData(SongSettings &settings)
{
    QString filename =
        QFileDialog::getSaveFileName(this, tr("Select Export File"),
                                     QDir::homePath());

    if (filename.isNull()) {
        return; 
    }

    QFile file( filename );
    if ( file.open(QIODevice::WriteOnly) )
    {
        
        QTextStream stream( &file );
        FileExportSongPlayEvent fespe(stream);
        QString dateFormat("yyyy-MM-dd HH:mm:ss.sss");
        
        QString startDate = ui->dateTimeEditStart->dateTime().toUTC().toString(dateFormat);
        QString endDate = ui->dateTimeEditEnd->dateTime().toUTC().toString(dateFormat);

        bool omitStartDate = ui->checkBoxOmitStart->isChecked();
        bool omitEndDate = ui->checkBoxOmitEnd->isChecked();
        int session_id = ui->comboBoxSession->currentIndex();

        stream << "\"Song\",\"when played (local)\",\"when played (UTC)\"\n";
        settings.getSongPlayHistory(fespe, session_id,
                                    omitStartDate,
                                    startDate,
                                    omitEndDate,
                                    endDate);
    } // end of successful open
}

void SongHistoryExportDialog::on_checkBoxOmitStart_stateChanged(int newState)
{
    ui->dateTimeEditStart->setEnabled(newState == Qt::Unchecked);
}

void SongHistoryExportDialog::on_checkBoxOmitEnd_stateChanged(int newState)
{
    ui->dateTimeEditEnd->setEnabled(newState == Qt::Unchecked);
}
