/****************************************************************************
**
** Copyright (C) 2016-2026 Mike Pogue, Dan Lyke
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

/* THIS IS A PLACEHOLDER FOR EVENTUALLY MOVING TO A MODEL VIEW FOR THE
 * SONG LIST - IGNORE FOR NOW - Dan 2018-06-29
 */

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
#define kLevelsCol 4
#define kRecentCol 5
#define kAgeCol   6
#define kPitchCol 7
#define kTempoCol 8

// Apple Music metadata columns (issue #1740, item 2).  These are populated only for tracks that
//   came from an Apple Music playlist; a song in the Music Directory keeps this metadata in its
//   own file tags, which nothing reads today, so those rows are blank here.
// NOTE: new columns MUST be appended.  The user's sort order is persisted as column INDICES
//   (see MyTableWidget::setOrderFromString, "c:2,so:0;c:1,so:0"), so inserting one in the middle
//   would silently re-point everybody's saved sort at a different column.  Left-to-right order
//   is a matter for QHeaderView::moveSection(), not for these numbers.
#define kAlbumCol       9
#define kAlbumArtistCol 10
#define kComposerCol    11
#define kCommentsCol    12
#define kYearCol        13
#define kDurationCol    14

// Issue #1744.  Artist was already imported for the square dance filter and the Type mapping; it
//   just never had a column.  Rating and Date Added are new imports.
#define kArtistCol      15
#define kRatingCol      16
#define kDateAddedCol   17

#define kNumSongTableCols 18

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
