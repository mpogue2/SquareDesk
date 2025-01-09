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

#include "songhistoryexportdialog.h"
#include "ui_songhistoryexportdialog.h"
#include "QFileDialog"
#include "songsettings.h"
#include "sessioninfo.h"
//#include "utility.h"

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
    ui->dateTimeEditEnd->setDisplayFormat("MM/dd/yyyy HH:mm ap"); // work around bug: changing 25 to 24 results in 1924

    ui->dateTimeEditStart->setDateTime(yesterday);
    ui->dateTimeEditStart->setDisplayFormat("MM/dd/yyyy HH:mm ap"); // work around bug: changing 25 to 24 results in 1924
    
    QList<SessionInfo> sessions(songSettings.getSessionInfo());
    ui->comboBoxSession->clear();
    ui->comboBoxSession->addItem("<all sessions>", 0);
    for (const auto &session : sessions)
    {
        ui->comboBoxSession->addItem(session.name, session.id);
    }

    ui->checkBoxOpenAfterExport->setChecked(true);
}



class FileExportSongPlayEvent : public SongPlayEvent {
    QTextStream &stream;
public:
    FileExportSongPlayEvent(QTextStream &stream) : stream(stream) {}
    virtual void operator() (const QString &name,
                             const QString &playedOnUTC,
                             const QString &playedOnLocal,
                             const QString &playedOnFilename,
                             const QString &playedOnPitch,
                             const QString &playedOnTempo,
                             const QString &playedOnLastCuesheet
                             )
    {
        outputString(stream, name, true);
        stream << ",";
        outputString(stream, playedOnLocal, true);
        stream << ",";
        outputString(stream, playedOnUTC, true);
        stream << ",";
        outputString(stream, playedOnFilename, true);
        stream << ",";
        outputString(stream, playedOnPitch, true);
        stream << ",";
        outputString(stream, playedOnTempo, true);
        stream << ",";
        outputString(stream, playedOnLastCuesheet, true);
        stream << "\n";
    }
    virtual ~FileExportSongPlayEvent() {}
};


QString SongHistoryExportDialog::exportSongPlayData(SongSettings &settings, QString lastSaveDir)
{
    QString dateFormat1("yyyy-MM-dd");
    QString startDate1 = ui->dateTimeEditStart->dateTime().toString(dateFormat1);

    QString saveDir = lastSaveDir;
    if (saveDir == "") {
        saveDir = QDir::homePath();  // use HOME, if one was not set previously
    }
    QString defaultFilename = saveDir + "/songsPlayedOn_" + startDate1 + ".txt";

    QString filename =
        QFileDialog::getSaveFileName(this, tr("Select Export File"),
                                     defaultFilename);

    if (filename.isNull()) {
        return("");
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
        int session_id = ui->comboBoxSession->currentData().toInt();

        stream << "\"Song\",\"when played (local)\",\"when played (UTC)\",\"filename\",\"pitch\",\"tempo\",\"last_cuesheet\"\n"; // CSV HEADER
        settings.getSongPlayHistory(fespe, session_id,
                                    omitStartDate,
                                    startDate,
                                    omitEndDate,
                                    endDate);

        if (ui->checkBoxOpenAfterExport->isChecked()) {
#if defined(Q_OS_MAC)
            QStringList args;
            args << "-e";
            args << "tell application \"TextEdit\"";
            args << "-e";
            args << "activate";
            args << "-e";
            args << "open POSIX file \"" + filename + "\"";
            args << "-e";
            args << "end tell";

            //    QProcess::startDetached("osascript", args);

            // same as startDetached, but suppresses output from osascript to console
            //   as per: https://www.qt.io/blog/2017/08/25/a-new-qprocessstartdetached
            QProcess process;
            process.setProgram("osascript");
            process.setArguments(args);
            process.setStandardOutputFile(QProcess::nullDevice());
            process.setStandardErrorFile(QProcess::nullDevice());
            qint64 pid;
            process.startDetached(&pid);
#endif
        }
        // get path to directory of file user selected
        QFileInfo fileInfo(filename);
        QString directoryPath = fileInfo.absolutePath();
        return(directoryPath); // successful return, update the lastExportSongHistoryDir in preferences
    } // end of successful open
    return(""); // error return
}

void SongHistoryExportDialog::on_checkBoxOmitStart_stateChanged(int newState)
{
    ui->dateTimeEditStart->setEnabled(newState == Qt::Unchecked);
}

void SongHistoryExportDialog::on_checkBoxOmitEnd_stateChanged(int newState)
{
    ui->dateTimeEditEnd->setEnabled(newState == Qt::Unchecked);
}
