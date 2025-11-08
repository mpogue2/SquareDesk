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
#include <QLocale>

#include "songhistoryexportdialog.h"
#include "ui_songhistoryexportdialog.h"
#include "QFileDialog"
#include "songsettings.h"
#include "sessioninfo.h"
#include "globaldefines.h"
#include "mainwindow.h"

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
    // ui->dateTimeEditEnd->setDisplayFormat("MM/dd/yyyy HH:mm ap"); // work around bug: changing 25 to 24 results in 1924

    ui->dateTimeEditStart->setDateTime(yesterday);
    // ui->dateTimeEditStart->setDisplayFormat("MM/dd/yyyy HH:mm ap"); // work around bug: changing 25 to 24 results in 1924

    // -------------------
    // Get the current system locale
    QLocale locale = QLocale::system();

    // Set the display format based on the locale's date and time formats
    QString dateFormat = locale.dateFormat(QLocale::ShortFormat);
    QString timeFormat = locale.timeFormat(QLocale::ShortFormat);
    QString dateTimeFormat = dateFormat + " " + timeFormat;

    // Apply the format to the widget
    ui->dateTimeEditStart->setDisplayFormat(dateTimeFormat);
    ui->dateTimeEditEnd->setDisplayFormat(dateTimeFormat);
    // -------------------

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
    MainWindow *mainWindow;
public:
    FileExportSongPlayEvent(QTextStream &stream, MainWindow *mw) : stream(stream), mainWindow(mw) {}
    virtual void operator() (const QString &name,
                             const QString &playedOnUTC,
                             const QString &playedOnLocal,
                             const QString &playedOnFilename,
                             const QString &playedOnPitch,
                             const QString &playedOnTempo,
                             const QString &playedOnLastCuesheet
                             )
    {
        // Extract labelID from filename
        QString label = "";
        QString labelnum = "";
        QString labelnum_extra = "";
        QString title = "";
        QString shortTitle = "";

        QFileInfo fi(playedOnFilename);
        QString baseName = fi.completeBaseName();

        if (mainWindow) {
            mainWindow->parseFilenameIntoParts(baseName, label, labelnum, labelnum_extra, title, shortTitle);
        }

        // Combine label parts and capitalize
        QString labelID = "";
        if (!label.isEmpty()) {
            labelID = label.toUpper();
            if (!labelnum.isEmpty()) {
                labelID += " " + labelnum.toUpper();
            }
            if (!labelnum_extra.isEmpty()) {
                labelID += labelnum_extra.toUpper();
            }
        }

        // Output labelID as first field
        outputString(stream, labelID, true);
        stream << ",";
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


QString SongHistoryExportDialog::exportSongPlayData(SongSettings &settings, QString lastSaveDir, QString &lastSaveExt, MainWindow *mainWindow)
{
    QString dateFormat1("yyyy.MM.dd");
    QString startDate1 = ui->dateTimeEditStart->dateTime().toString(dateFormat1);

    QString saveDir = lastSaveDir;
    if (saveDir == "") {
        saveDir = QDir::homePath();  // use HOME, if one was not set previously
    }
    // Use remembered extension, default to ".csv" if not set
    QString extension = lastSaveExt.isEmpty() ? ".csv" : lastSaveExt;
    QString defaultFilename = saveDir + "/songsPlayedOn_" + startDate1 + extension;

    QString filename =
        QFileDialog::getSaveFileName(this,
                                     tr("Select Export File"),
                                     defaultFilename);

    if (filename.isNull()) {
        return("");
    }

    QFile file( filename );
    if ( file.open(QIODevice::WriteOnly) )
    {        
        QTextStream stream( &file );
        FileExportSongPlayEvent fespe(stream, mainWindow);
        
        QString dateFormat("yyyy-MM-dd HH:mm:ss.sss");
        QString startDate = ui->dateTimeEditStart->dateTime().toUTC().toString(dateFormat);
        QString endDate = ui->dateTimeEditEnd->dateTime().toUTC().toString(dateFormat);

        // DDD(startDate)
        // DDD(endDate)

        bool omitStartDate = ui->checkBoxOmitStart->isChecked();
        bool omitEndDate = ui->checkBoxOmitEnd->isChecked();
        int session_id = ui->comboBoxSession->currentData().toInt();

        stream << "\"labelID\",\"Song\",\"when played (local)\",\"when played (UTC)\",\"filename\",\"pitch\",\"tempo\",\"last_cuesheet\"\n"; // CSV HEADER
        settings.getSongPlayHistory(fespe, session_id,
                                    omitStartDate,
                                    startDate,
                                    omitEndDate,
                                    endDate);

        if (ui->checkBoxOpenAfterExport->isChecked()) {
                // NOWADAYS DO THIS FOR ALL FILES/ALL PLATFORMS:
                //   e.g. MacOS: (".txt" -> TextEdit), (".csv" -> Numbers/Excel/LibreOffice)
                QProcess::startDetached("open", QStringList() << filename);
        }
        // get path to directory and extension of file user selected
        QFileInfo fileInfo(filename);
        QString directoryPath = fileInfo.absolutePath();
        lastSaveExt = "." + fileInfo.suffix();  // Update the extension (e.g., ".csv" or ".txt")
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
