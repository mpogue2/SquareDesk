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


#ifndef CALLLISTDATEDELEGATE_H
#define CALLLISTDATEDELEGATE_H

#include <QDate>
#include <QDateEdit>
#include <QStyledItemDelegate>
#include <functional>

// Editor for the Completed (taught-on) column of the Dance Programs call list (#1766).
//   Double-clicking a date opens a QDateEdit with a calendar popup. Rows with no date
//   (call not checked) get no editor. The chosen date is handed to onDateEdited along
//   with the call name, which saves it and returns the text to display.
class CallListDateDelegate : public QStyledItemDelegate
{
public:
    CallListDateDelegate(int nameColumn,
                         std::function<QString(const QString &callName, const QDate &date)> onDateEdited,
                         QObject *parent = nullptr)
        : QStyledItemDelegate(parent), nameColumn(nameColumn), onDateEdited(std::move(onDateEdited)) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const override
    {
        if (!QDate::fromString(index.data().toString(), kDateFormat).isValid())
            return nullptr;  // unchecked call: nothing to edit

        QDateEdit *editor = new QDateEdit(parent);
        editor->setDisplayFormat(kDateFormat);
        editor->setCalendarPopup(true);
        editor->setMaximumDate(QDate::currentDate());  // can't have taught it in the future
        editor->setAlignment(Qt::AlignCenter);
        return editor;
    }

    // The column is sized to fit the date text, which is narrower than a QDateEdit with its
    //   popup button, so let the editor extend to the right while it's open.
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        QRect r = option.rect;
        r.setWidth(qMax(r.width(), editor->sizeHint().width()));
        editor->setGeometry(r);
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        static_cast<QDateEdit *>(editor)->setDate(QDate::fromString(index.data().toString(), kDateFormat));
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        QDate date = static_cast<QDateEdit *>(editor)->date();
        if (date.toString(kDateFormat) == index.data().toString())
            return;  // unchanged

        // read the call name BEFORE setData, because with sorting on the row can move
        QString callName = index.sibling(index.row(), nameColumn).data().toString();
        model->setData(index, onDateEdited(callName, date));
    }

private:
    static constexpr const char *kDateFormat = "yyyy-MM-dd";
    int nameColumn;
    std::function<QString(const QString &, const QDate &)> onDateEdited;
};

#endif // CALLLISTDATEDELEGATE_H
