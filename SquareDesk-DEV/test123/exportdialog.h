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

#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QValidator>

#include "common_enums.h"

namespace Ui
{
class ExportDialog;
}


class SongSettings;

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(QWidget *parent = 0);
    ~ExportDialog();
    void exportSongs(SongSettings &settings, QList<QString> *musicFilenames);

private slots:
    void on_pushButtonChooseFile_clicked();
     
private:
    Ui::ExportDialog *ui;
};

QString removeCuesheetDirs(QString s, QStringList songTypeNames);

void exportSongList(QTextStream &stream, SongSettings &settings, QList<QString> *musicFilenames,
                    int outputFieldCount, enum ColumnExportData outputFields[],
                    char separator,
                    bool includeHeaderNames,
                    bool relativePathNames,
                    QStringList songTypeNamesForSinging);

#endif // EXPORTDIALOG_H
