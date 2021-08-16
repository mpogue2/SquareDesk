/****************************************************************************
**
** Copyright (C) 2016-2021 Mike Pogue, Dan Lyke
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

#include "danceprogramsmodel.h"
#include "calllistcheckbox.h"

DanceProgramsModel::DanceProgramsModel()
    : QAbstractItemModel()
{
}

void DanceProgramsModel::loadCallList(const QString &danceProgram, const QString &filename)
{
    static QRegularExpression regex_numberCommaName(QRegularExpression("^((\\s*\\d+)(\\.\\w+)?)\\,?\\s+(.*)$"));

    moveRows.clear();
    QFile inputFile(filename);
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        int line_number = 0;
        while (!in.atEnd())
        {
            line_number++;
            QString line = in.readLine();

            QString number(QString("%1").arg(line_number, 2));
            QString name(line);

            QRegularExpressionMatch match = regex_numberCommaName.match(line);
            if (match.hasMatch())
            {
                QString prefix("");
                if (match.captured(2).length() < 2)
                {
                    prefix = " ";
                }
                number = prefix + match.captured(1);
                name = match.captured(4);
            }
            QString taughtOn = songSettings->getCallTaughtOn(danceProgram, name);
            CallListCheckBox *checkbox = AddItemToCallList(tableWidget, number, name, taughtOn, findTimingForCall(danceProgram, name));
            callListOriginalOrder.append(name);
            checkbox->setMainWindow(this);

        }
        inputFile.close();
    }
    
}


QModelIndex DanceProgramsModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex(row, column, Q_NULLPTR);
}

QModelIndex DanceProgramsModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return(QModelIndex());  // to remove warning
}

int DanceProgramsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return moveRows.length();
}

int DanceProgramsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 0;
}

QVariant DanceProgramsModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
    auto & songRow(moveRows[index.row()]);

    switch (index.column())
    {
    case kDanceProgramTeachOrderCol :
        return QVariant(songRow.teachOrder);
        
    case kDanceProgramTaughtCheckboxCol :
        return QVariant( !songRow.completed.empty() );
        
    case kDanceProgramMoveName :
        return QVariant(songRow.name);
        
    case kDanceProgramCompletedDate :
        return QVariant(songRow.completed);
        
    case kDanceProgramTiming :
        return QVariant(songRow.timing);
    }

    return QVariant("<unknown row>");
}


Qt::ItemFlags DanceProgramsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flag = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    
    switch (index.column())
    {
    case kDanceProgramTaughtCheckboxCol :
    case kDanceProgramCompletedDate :
        flag |= Qt::ItemIsEditable;
    default :
        break;
    }
    return flag;
}


bool DanceProgramsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role)
    Q_UNUSED(value)
    bool set = false;
    auto & songRow(moveRows[index.row()]);
    switch (index.column())
    {
    case kDanceProgramTaughtCheckboxCol :
        songRow.completed = ...; 
        break;
    case kNumberCol :
        songRow.completed = value.string().
        set = true;
        break;
    default:
        break;
    }
    return set;
}
 
