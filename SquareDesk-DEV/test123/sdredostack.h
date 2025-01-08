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
#ifndef SD_REDO_STACK_INCLUDED
#define SD_REDO_STACK_INCLUDED 1
#include <QStringList>

class SDRedoStack {
public:
    QStringList command_stack;
    QList<QStringList> redo_stack;
    QString last_seen_formation;
    int last_seen_row;
    int last_seen_formation_line;
    bool doing_user_input;
    
    SDRedoStack();

    void initialize();
    
    void add_command(int row, const QString &cmd);
    void checkpoint_formation(int last_line, const QString &formation);

    const QStringList &get_redo_commands();
    void discard_redo_commands();

    void did_an_undo();
    void set_doing_user_input();
    void clear_doing_user_input();

    bool can_redo();

private:
    int build_redo_stack_from_command_tail(QStringList &redo_stack_front, int *p_last_seen_formation_line = NULL, int *p_last_seen_row = NULL);
};

#endif /* ifndef COMMAND_STACK_INCLUDED */
