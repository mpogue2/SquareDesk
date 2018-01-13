/* Copyright (C) 2016, 2017, 2018 Mike Pogue, Dan Lyke
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

#include "importdialog.h"
#include "ui_importdialog.h"
#include "QFileDialog"
#include "utility.h"

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


QStringList splitCSV(const QString &str)
{
    QStringList fields;
    int pos = 0;
    QString result;

    while (pos < str.length())
    {
        while (pos < str.length() && str[pos].isSpace())
            pos++;
        if (str[pos] == '"')
        {
            pos++;
            int startPos = pos;
            while (pos < str.length() && str[pos] != '"')
            {
                if (str[pos] == '\\')
                {
                    result = result + str.mid(startPos, pos - startPos - 1);
                    ++pos;
                    if (pos < str.length())
                    {
                        if (str[pos] == 'r')
                            result = result + '\r';
                        else if (str[pos] == 'n')
                            result = result + '\n';
                        else if (str[pos] == 't')
                            result = result + '\t';
                        else
                            result = result + str[pos];
                        pos++;
                    }
                    startPos = pos;
                }
                ++pos;
            }
            if (startPos < pos)
                result = result + str.mid(startPos, pos - startPos);
        }
        else
        {
            int startPos = pos;
            pos = str.indexOf(',', pos);
            if (pos == -1)
                pos = str.length();
            
            if (startPos < pos)
                result = result + str.mid(startPos, pos - startPos);
         }
        
        if (str[pos] == ',')
        {
            fields << result.trimmed();
            result.clear();
        }
            
        pos++;
    }
    fields << result;
    return fields;
}



QStringList readLine(QFile &file, bool tab_separator)
{
    QStringList list;
    QString line;

    while (!file.atEnd() && line.isEmpty())
    {
        line = file.readLine().trimmed();
    }

    if (!file.atEnd())
    {
        if (tab_separator)
            list = line.split((char)(9));
        else
            list = splitCSV(line);
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
        fieldsCommas = splitCSV(line);
    }

    bool usesTabs = (fieldsTabs.length() > fieldsCommas.length());
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

void ImportDialog::readImportFile(SongSettings &settings,
                                  QFile &file,
                                  QMap<QString, SongSetting> &songSettingsByFilename)
{
    while (!file.atEnd())
    {
        QStringList fields = readLine(file, ui->comboBoxTabOrCSV->currentIndex() == 0);

        QString filename;
        SongSetting setting;

        if (fields.size() > ui->comboBoxColumnSongName->currentIndex())
            filename = fields[ui->comboBoxColumnSongName->currentIndex()];

        if (fields.size() > ui->comboBoxColumnTempo->currentIndex())
        {
            QString v(fields[ui->comboBoxColumnTempo->currentIndex()]);
            // qDebug() << "Setting " << filename << " Tempo " << " to " << v;
            QString v1 = v.replace("%","");

            if (v1 != v) setting.setTempoIsPercent(true);
            
            int t = v1.toInt();
            if (t)
                setting.setTempo(t);
            switch (ui->comboBoxFormatTempo->currentIndex())
            {
            case 1:
                setting.setTempoIsPercent(true);
                break;
            case 2:
                setting.setTempoIsPercent(false);
                break;
            }
        }
        if (fields.size() > ui->comboBoxColumnPitch->currentIndex())
        {
            QString v(fields[ui->comboBoxColumnPitch->currentIndex()]);
            int t = v.toInt();
            // qDebug() << "Setting " << filename << " Pitch " << " to " << t;
            if (t)
                setting.setPitch(t);
        }
        if (fields.size() > ui->comboBoxColumnIntro->currentIndex())
        {
            QString v(fields[ui->comboBoxColumnIntro->currentIndex()]);
            double d = timeToDouble(v);
            // qDebug() << "Setting " << filename << " Intro " << " to " << d;
            setting.setIntroPos(d);
            setting.setIntroOutroIsTimeBased(true);
        }
        if (fields.size() > ui->comboBoxColumnOutro->currentIndex())
        {
            QString v(fields[ui->comboBoxColumnOutro->currentIndex()]);
            double d = timeToDouble(v);
            setting.setOutroPos(d);
            // qDebug() << "Setting " << filename << " Outro " << " to " << d;
            setting.setIntroOutroIsTimeBased(true);
        }
        if (fields.size() > ui->comboBoxColumnVolume->currentIndex())
        {
            QString v(fields[ui->comboBoxColumnVolume->currentIndex()]);
            // qDebug() << "Setting " << filename << " Volume " << " to " << v;
            int t = v.toInt();
            if (t)
                setting.setVolume(t);
        }
        if (fields.size() > ui->comboBoxColumnCuesheet->currentIndex())
        {
            QString v(fields[ui->comboBoxColumnCuesheet->currentIndex()]);
            // qDebug() << "Setting " << filename << " Cuesheet " << " to " << v;
            if (!v.isEmpty())
            {
                if (ui->comboBoxFormatCuesheet->currentIndex() == 0)
                    v = settings.primaryRootDir() + v;
                setting.setCuesheetName(v);
            }
        }
        if (!filename.isNull() && !filename.isEmpty())
            songSettingsByFilename[filename] = setting;
    }
}


void ImportDialog::importSongs(SongSettings &settings, QList<QString>* musicFilenames)
{
    QString filename(ui->labelFileName->text());

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open " << filename;
        return;
    }

    
    QString line;

    if (ui->comboBoxFirstRow->currentIndex() == 0)
    {
        while (!file.atEnd() && line.isEmpty())
        {
            line = file.readLine().trimmed();
        }
    }

    QMap<QString, SongSetting> songSettingsByFilename;
    readImportFile(settings, file, songSettingsByFilename);

    QListIterator<QString> iter(*musicFilenames);
    while (iter.hasNext())
    {
        QString s(iter.next());
        QStringList sl1 = s.split("#!#");
        QString filename_with_path(sl1[1]);
        QString altfilename(settings.removeRootDirs(filename_with_path));
        QFileInfo fi(filename_with_path);
        QString basename = fi.completeBaseName();
        QString filename = fi.fileName();

        if (songSettingsByFilename.contains(filename_with_path))
        {
            settings.saveSettings(filename,songSettingsByFilename[filename_with_path]);
        }
        else if (songSettingsByFilename.contains(altfilename))
        {
            settings.saveSettings(filename,songSettingsByFilename[altfilename]);
        }
        else if (songSettingsByFilename.contains(filename))
        {
            settings.saveSettings(filename,songSettingsByFilename[filename]);
        }
        else if (songSettingsByFilename.contains(basename))
        {
            settings.saveSettings(filename,songSettingsByFilename[basename]);
        }
    } // end of iterating through passed in filenames
}
