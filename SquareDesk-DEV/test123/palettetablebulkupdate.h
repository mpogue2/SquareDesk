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

#ifndef PALETTETABLEBULKUPDATE_H_INCLUDED
#define PALETTETABLEBULKUPDATE_H_INCLUDED

#include <QHeaderView>
#include <QTableWidget>

// ============================================================================================================
// RAII guard for any loop that touches a palette slot table one row (or one item) at a time.
//
// WHY THIS EXISTS (issue #1695):
//   The palette slot tables use QHeaderView::ResizeToContents on their VERTICAL header, so each
//   row's height tracks its contents.  That is fine when it happens once, but it makes any
//   per-item update of a *visible* table quadratic:
//
//     QTableWidgetItem::setFont()  -> setData(Qt::FontRole) -> emits dataChanged
//       -> QHeaderView::dataChanged()        posts a delayed resizeSections()
//       -> QAbstractItemView::dataChanged()  calls update(index)
//            -> QTableView::visualRect() -> rowHeight() -> QHeaderView::sectionSize()
//            -> executePostedResize()        runs that posted resize RIGHT NOW, synchronously
//       -> QHeaderViewPrivate::resizeSections() re-measures EVERY row via sizeHintForRow(),
//            which includes editor->sizeHint() on each row's rich-text QLabel cell widget.
//
//   So N items touched x N rows re-measured = O(N^2).  Loading a 511-row Track Filter into a
//   palette slot spent 12.8 s of its 13.0 s inside exactly one such loop (the per-row setFont
//   loop in adjustFontSizes()), while building all 511 rows took only 176 ms.
//
// WHAT THIS DOES:
//   Switching the vertical header to Fixed for the duration of the loop makes
//   QHeaderViewPrivate::hasAutoResizeSections() return false, so no resize is ever posted and
//   executePostedResize() becomes a no-op -- update(index) drops to O(1).  Restoring
//   ResizeToContents afterwards costs exactly one O(N) pass, so the whole loop becomes O(N).
//   Row heights end up identical to what they were before; they are just computed once at the
//   end instead of once per touched item.
//
// USAGE:
//   {
//       PaletteTableBulkUpdate bulk(theTableWidget);
//       ... loop that adds rows, or sets fonts/items on existing rows ...
//   }   // <-- heights recomputed once here
//
//   Hiding the table around the loop happens to dodge the same blowup (dataChanged() skips
//   update() when the view is not visible), but that is incidental and easy to break.  Prefer
//   this guard: it is explicit about the invariant and works whether the table is shown or not.
// ============================================================================================================
class PaletteTableBulkUpdate {
public:
    explicit PaletteTableBulkUpdate(QTableWidget *table) : table(table) {
        table->setUpdatesEnabled(false);
        table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    }

    ~PaletteTableBulkUpdate() {
        table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        table->resizeRowsToContents();  // the single O(N) pass, instead of one per touched item
        table->setUpdatesEnabled(true);
    }

    PaletteTableBulkUpdate(const PaletteTableBulkUpdate &) = delete;
    PaletteTableBulkUpdate &operator=(const PaletteTableBulkUpdate &) = delete;

private:
    QTableWidget *table;
};

#endif /* ifndef PALETTETABLEBULKUPDATE_H_INCLUDED */
