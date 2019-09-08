/****************************************************************************
**
** Copyright (C) 2016-2019 Mike Pogue, Dan Lyke
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
#include "sdredostack.h"
#include <QRegularExpression>

SDRedoStack::SDRedoStack()
    : command_stack(), doing_user_input(false)
{
    initialize();
}


void SDRedoStack::set_doing_user_input()
{
    doing_user_input = true;
}

void SDRedoStack::clear_doing_user_input()
{
    doing_user_input = false;
}


void SDRedoStack::initialize()
{
    doing_user_input = false;
    command_stack.clear();
    last_seen_formation.clear();
    last_seen_row = -1;
    last_seen_formation_line = -1;
    redo_stack.clear();
}

bool SDRedoStack::can_redo()
{
    return !command_stack.empty();
}


void SDRedoStack::add_command(int row, const QString &cmd)
{
    if (doing_user_input)
    {
        if (row != last_seen_row)
        {
            QString str(QString("<row %1>").arg(row));
            command_stack.push_back(str);
            last_seen_row = row;
        }
        command_stack.push_back(cmd);
    }
}

static bool areFormationsEqual(const QString &a, const QString &b) {
    if (a == b)
        return true;
    static QRegularExpression regexFormation("\\<formation: (\\d+)\\: (.*)\n(.*)$");
    QRegularExpressionMatch matchA(regexFormation.match(a));
    QRegularExpressionMatch matchB(regexFormation.match(b));

    if (matchA.hasMatch() && matchB.hasMatch())
    {
        if (matchA.captured(2) == matchB.captured(2) && matchA.captured(3) == matchB.captured(3))
            return true;
    }
    return false;
}

void SDRedoStack::checkpoint_formation(int last_line, const QString &formation)
{
    if (formation != last_seen_formation && last_line >= last_seen_formation_line)
    {
        last_seen_formation = formation;
        last_seen_formation_line = last_line;
        QString str(QString("<formation: %1: %2\n>").arg(last_line).arg(formation));
        command_stack.push_back(str);

        QStringList redo_list;
        build_redo_stack_from_command_tail(redo_list, &last_seen_formation_line, &last_seen_row);
        if (!redo_stack.empty() && !redo_stack.front().empty() && areFormationsEqual(str,redo_stack.front().front()))
        {
        }
        else
        {
            if (!redo_stack.empty())
            {
                redo_stack.clear();
            }
        }
    }
}

const QStringList &SDRedoStack::get_redo_commands()
{
    if (!redo_stack.isEmpty())
        return redo_stack.front();
    static QStringList empty_list;
    return empty_list;
}

void SDRedoStack::discard_redo_commands() {
    if (!redo_stack.isEmpty())
        redo_stack.pop_front();
}



void SDRedoStack::did_an_undo()
{
    redo_stack.push_front(QStringList());
    int undo_pos = build_redo_stack_from_command_tail(redo_stack.front());
    command_stack.erase(command_stack.begin() + undo_pos, command_stack.end());
}

int SDRedoStack::build_redo_stack_from_command_tail(QStringList &redo_stack_front, int *p_last_seen_formation_line, int *p_last_seen_row)
{
    if (command_stack.isEmpty())
        return 0;
    int command_stack_count = command_stack.count();
    int undo_pos = command_stack_count - 1;
    while (undo_pos >= 0 && command_stack[undo_pos].startsWith("<"))
        --undo_pos;
    while (undo_pos > 0 && !command_stack[undo_pos - 1].startsWith("<"))
        --undo_pos;

    if (undo_pos < 0)
        return 0;
    
    int initial_formation = undo_pos - 1;
    while (initial_formation > 0 &&
           command_stack[initial_formation].startsWith("<") &&
           !command_stack[initial_formation].startsWith("<formation: "))
        initial_formation--;

    if (initial_formation >= 0 && command_stack[initial_formation].startsWith("<formation: "))
    {
        while (initial_formation < undo_pos)
        {
            QString s = command_stack[initial_formation++];
            static QRegularExpression regexFormationNumber("\\<formation: (\\d+)\\:");
            static QRegularExpression regexRowNumber("\\<row: (\\d+)\\>");
            QRegularExpressionMatch formationMatch(regexFormationNumber.match(s));
            QRegularExpressionMatch rowMatch(regexRowNumber.match(s));

            if (formationMatch.hasMatch())
            {
                bool ok = true;
                int i = formationMatch.captured(1).toInt(&ok);
                if (ok)
                {
                    if (p_last_seen_formation_line)
                        *p_last_seen_formation_line = i;
                }
            }

            if (rowMatch.hasMatch())
            {
                bool ok = true;
                int i = rowMatch.captured(1).toInt(&ok);
                if (ok)
                {
                    if (p_last_seen_row)
                        *p_last_seen_row = i;
                }
            }

            redo_stack_front.push_back(s);
        }
    }
    
    for (int i = undo_pos; i < command_stack_count; ++i)
    {
        QString s(command_stack[i]);
        redo_stack_front.push_back(s);
    }
    return undo_pos;
}
