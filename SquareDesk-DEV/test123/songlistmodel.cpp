/* THIS IS A PLACEHOLDER FOR EVENTUALLY MOVING TO A MODEL VIEW FOR THE
 * SONG LIST - IGNORE FOR NOW - Dan 2018-06-29
 */


/****************************************************************************
**
** Copyright (C) 2016-2020 Mike Pogue, Dan Lyke
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

#include "songlistmodel.h"

SongListModel::SongListModel()
    : QAbstractItemModel()
{
}

QModelIndex SongListModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex(row, column, Q_NULLPTR);
}

QModelIndex SongListModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return(QModelIndex());  // to remove warning
}

int SongListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return songRows.length();
}

int SongListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 0;
}

QVariant SongListModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
    Q_UNUSED(index)
//    auto & songRow(songRows[index.row()]);
#if 0
    switch (index.column())
    {
    case kNumberCol :
        int playlist_position = (static_cast<SongSetting*>(index.data()))->playlist_position;
        return QVariant( playlist_position < 0 ? QString("") : QString("%1").arg(playlist_position, 3) );
        
    case kTypeCol :
        return ...;
        
    case kLabelCol :
        return ...;
        
    case kTitleCol :
        return ...;
        
    case kRecentCol :
        return ...;
        
    case kAgeCol   :
        return ;
        
    case kPitchCol :
        return ...;
        
    case kTempoCol :
        return ...;
        
    }
#endif
    return QVariant("<unknown row>");
}

Qt::ItemFlags SongListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flag = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    
    switch (index.column())
    {
    case kNumberCol :
        flag |= Qt::ItemIsEditable;
        break;
    default :
        break;
    }
    return flag;
}

bool SongListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role)
    Q_UNUSED(value)
    bool set = false;
    switch (index.column())
    {
    case kNumberCol :
        // ZZZZZZZ
        set = true;
        break;
    default:
        break;
    }
    return set;
}
 
