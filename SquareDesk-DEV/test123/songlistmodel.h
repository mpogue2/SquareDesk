/* THIS IS A PLACEHOLDER FOR EVENTUALLY MOVING TO A MODEL VIEW FOR THE
 * SONG LIST - IGNORE FOR NOW - Dan 2018-06-29
 */

/****************************************************************************
**
** Copyright (C) 2016, 2017, 2018 Mike Pogue, Dan Lyke
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

#ifndef SONGLISTMODEL_H_INCLUDED
#define SONGLISTMODEL_H_INCLUDED
#include <QAbstractItemModel>
#include <QList>
#include <QHash>
#include "songsettings.h"


// columns in songTable
#define kNumberCol 0
#define kTypeCol 1
#define kPathCol 1
// path is stored in the userData portion of the Type column...
#define kLabelCol 2
#define kTitleCol 3

// POSSIBLY hidden columns:
#define kRecentCol 4
#define kAgeCol   5
#define kPitchCol 6
#define kTempoCol 7

class SongRow : public SongSetting
{
public:
SongRow() : SongSetting() { }

    int playlistNum;
};

class SongListModel : public QAbstractItemModel
{
    Q_OBJECT
private:
    SongSettings *songSettings;
    QList<SongRow> songRows;
    QHash<QString, int> songListByFilename;
    
public:
    SongListModel();
    void SetSongSettings(SongSettings *songSettings)
    {
        this->songSettings = songSettings;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex() ) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
};

#endif /* ifndef SONGLISTMODEL_H_INCLUDED */
