/****************************************************************************
**
** Copyright (C) 2016-2024 Mike Pogue, Dan Lyke
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

//#include "calllistcheckbox.h"
#include "mainwindow.h"
#include "sdlineedit.h"
#include "ui_mainwindow.h"

bool SDLineEdit::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->matches(QKeySequence::Undo)) {
            mainWindow->undo_last_sd_action();
            return true;
        }
        if (ke->matches(QKeySequence::Redo)) {
            mainWindow->redo_last_sd_action();
            return true;
        }
        if (ke->matches(QKeySequence::SelectAll)) {
            mainWindow->select_all_sd_current_sequence();
            return true;
        }
        if (ke->matches(QKeySequence::Copy)) {
            mainWindow->copy_selection_from_tableWidgetCurrentSequence();
            return true;
        }
    }
    return QLineEdit::event(event);
}

void SDLineEdit::focusInEvent(QFocusEvent *e)
{
  QLineEdit::focusInEvent(e);

//  // AND, if there's anything in the Current Sequence, render the last item and clear any selection
//  //  as we're going back to entering calls (we want to see the last know state of the square for this case)

  if (mainWindow != NULL) {
      int c = mainWindow->ui->tableWidgetCurrentSequence->rowCount();

//      qDebug() << "***** c: " << c;
      if (c > 0) {
          mainWindow->ui->tableWidgetCurrentSequence->clearSelection();
          mainWindow->render_sd_item_data(mainWindow->ui->tableWidgetCurrentSequence->item(c-1, 1)); // last row, col 1
      }
  } else {
      qDebug() << "mainWindow was NULL!";
  }
}
