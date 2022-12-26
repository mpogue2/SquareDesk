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

#ifndef DANCEPROGRAMSMODEL_H_INCLUDED
#define DANCEPROGRAMSMODEL_H_INCLUDED
#include <QAbstractItemModel>
#include <QList>
#include <QHash>
#include "songsettings.h"


#define kDanceProgramTeachOrderCol     0
#define kDanceProgramTaughtCheckboxCol 1
#define kDanceProgramMoveName          2
#define kDanceProgramCompletedDate     3
#define kDanceProgramTiming            4


class DanceProgramMoveRow
{
public:
    QString teachOrder;
    QString name;
    QString completed;
    QString timing;
};

class DanceProgramsModel : public QAbstractItemModel
{
    Q_OBJECT
private:
    SongSettings *songSettings;
    QList<DanceProgramMoveRow> moveRows;
    
public:
    DanceProgramsModel();
    void SetSongSettings(SongSettings *songSettings)
    {
        this->songSettings = songSettings;
    }
    void loadCallList(const QString &danceProgram, const QString&filename);
    QString callNameForOriginalRow(int clickRow);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex() ) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
};

#endif /* ifndef DANCEPROGRAMSMODEL_H_INCLUDED */
